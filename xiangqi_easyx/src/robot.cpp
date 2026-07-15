#include "robot.h"

#include <windows.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <future>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// enginecandidate 保存 Pikafish 返回的一个高质量候选走法。
struct enginecandidate
{
    std::string movetext; // movetext 表示 UCI 格式的走法文字。
    int score;            // score 表示引擎站在当前行棋方角度计算的分数。
    int depth;            // depth 表示该候选的搜索深度。
    bool valid;           // valid 表示候选走法是否已被有效解析。
};

// robotenginestate 保存 Pikafish 子进程和 UCI 管道状态。
struct robotenginestate
{
    HANDLE process = nullptr;      // process 表示 Pikafish 子进程句柄。
    HANDLE thread = nullptr;       // thread 表示 Pikafish 主线程句柄。
    HANDLE inputwrite = nullptr;   // inputwrite 表示向引擎发送 UCI 命令的管道。
    HANDLE outputread = nullptr;   // outputread 表示读取引擎 UCI 输出的管道。
    std::string buffer;            // buffer 保存尚未组成完整行的引擎输出。
    std::wstring folder;           // folder 表示引擎与 NNUE 文件所在目录。
    std::wstring error;            // error 保存最近一次引擎错误说明。
    int multipv = 0;               // multipv 表示引擎当前输出的候选线数量。
    bool ready = false;            // ready 表示引擎是否已完成 UCI 初始化。
};

static robotenginestate engine; // engine 保存本次游戏共用的 Pikafish 引擎实例。
static std::future<robotmove> robotsearchfuture;       // robotsearchfuture 保存后台搜索任务及其返回结果。
static std::mutex enginewritemutex;                   // enginewritemutex 保护不同线程向引擎管道写入命令。
static std::atomic<bool> robotsearchcancelled(false); // robotsearchcancelled 表示当前后台搜索已被请求取消。
static std::atomic<bool> enginesearching(false);      // enginesearching 表示 Pikafish 当前正在执行 go 命令。
static bool robotsearchactive = false;                // robotsearchactive 表示后台搜索任务尚未被主线程回收。

// 这个函数根据对战模式补全红黑双方控制配置。
static void applyrobotmode(robotsetting& setting)
{
    setting.redrobot = setting.mode == robot_mode_red || setting.mode == robot_mode_both;
    setting.blackrobot = setting.mode == robot_mode_black || setting.mode == robot_mode_both;
}

// 这个函数设置对战模式，并通过范围保护同步双方控制配置。
void setrobotmode(robotsetting& setting, int mode)
{
    int safemode = mode < robot_mode_players ? robot_mode_players : mode; // safemode 表示经过下限保护的对战模式。
    setting.mode = safemode > robot_mode_both ? robot_mode_both : safemode;
    applyrobotmode(setting);
}

// 这个函数判断指定颜色是否由机器人控制。
bool isrobotcolor(const robotsetting& setting, int color)
{
    if (color == color_red)
    {
        return setting.redrobot;
    }
    if (color == color_black)
    {
        return setting.blackrobot;
    }
    return false;
}

// 这个函数安全关闭一个 Windows 句柄并清空它。
static void closehandle(HANDLE& handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle);
    }
    handle = nullptr;
}

// 这个函数停止 Pikafish 子进程并释放全部管道。
void closerobotengine()
{
    cancelrobotsearch();

    if (engine.inputwrite != nullptr)
    {
        const char quitcommand[] = "quit\n"; // quitcommand 表示请求引擎正常退出的 UCI 命令。
        DWORD writtencount = 0;               // writtencount 表示实际写入管道的字节数。
        WriteFile(engine.inputwrite, quitcommand, static_cast<DWORD>(sizeof(quitcommand) - 1), &writtencount, nullptr);
    }

    if (engine.process != nullptr)
    {
        DWORD waitresult = WaitForSingleObject(engine.process, 1200); // waitresult 表示等待引擎正常退出的结果。
        if (waitresult == WAIT_TIMEOUT)
        {
            TerminateProcess(engine.process, 0);
            WaitForSingleObject(engine.process, 500);
        }
    }

    closehandle(engine.inputwrite);
    closehandle(engine.outputread);
    closehandle(engine.thread);
    closehandle(engine.process);
    engine.buffer.clear();
    engine.folder.clear();
    engine.multipv = 0;
    engine.ready = false;
}

// 这个函数记录引擎错误、关闭引擎并返回失败状态。
static bool failrobotengine(const std::wstring& message)
{
    closerobotengine();
    engine.error = message;
    return false;
}

// 这个函数向 Pikafish 发送一行 ASCII UCI 命令。
static bool sendenginecommand(const std::string& command)
{
    std::lock_guard<std::mutex> writelock(enginewritemutex); // writelock 保证每条 UCI 命令完整写入管道。
    if (engine.inputwrite == nullptr)
    {
        engine.error = L"Pikafish 输入管道未就绪。";
        return false;
    }

    std::string line = command + "\n"; // line 表示补全换行符后的 UCI 命令。
    DWORD writtencount = 0;             // writtencount 表示实际写入引擎的字节数。
    BOOL result = WriteFile(engine.inputwrite, line.data(), static_cast<DWORD>(line.size()), &writtencount, nullptr); // result 表示管道写入是否成功。
    if (!result || writtencount != line.size())
    {
        engine.error = L"无法向 Pikafish 发送 UCI 命令。";
        return false;
    }
    return true;
}

// 这个函数请求停止后台搜索，并等待工作线程退出后回收结果。
void cancelrobotsearch()
{
    if (!robotsearchactive)
    {
        robotsearchcancelled = false;
        enginesearching = false;
        return;
    }

    robotsearchcancelled = true;
    if (enginesearching)
    {
        sendenginecommand("stop");
    }
    if (robotsearchfuture.valid())
    {
        robotsearchfuture.wait();
        try
        {
            robotsearchfuture.get();
        }
        catch (...)
        {
        }
    }
    robotsearchactive = false;
    robotsearchcancelled = false;
    enginesearching = false;
}

// 这个函数在指定超时时间内读取 Pikafish 的一行输出。
static bool readengineline(std::string& line, int timeoutms)
{
    unsigned long starttick = GetTickCount(); // starttick 表示本次读取开始时的毫秒时间。
    while (static_cast<unsigned long>(GetTickCount() - starttick) <= static_cast<unsigned long>(timeoutms))
    {
        std::size_t newline = engine.buffer.find('\n'); // newline 表示缓冲区中第一个换行符位置。
        if (newline != std::string::npos)
        {
            line = engine.buffer.substr(0, newline);
            engine.buffer.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            return true;
        }

        DWORD available = 0; // available 表示引擎输出管道中待读的字节数。
        if (!PeekNamedPipe(engine.outputread, nullptr, 0, nullptr, &available, nullptr))
        {
            engine.error = L"Pikafish 输出管道已断开。";
            return false;
        }

        if (available > 0)
        {
            char data[1024] = {}; // data 保存一次从引擎管道读取的字节。
            DWORD readcount = 0;  // readcount 表示本次实际读取的字节数。
            DWORD requestcount = available < sizeof(data) ? available : static_cast<DWORD>(sizeof(data)); // requestcount 表示本次请求读取的字节数。
            if (!ReadFile(engine.outputread, data, requestcount, &readcount, nullptr))
            {
                engine.error = L"读取 Pikafish 输出失败。";
                return false;
            }
            engine.buffer.append(data, readcount);
            continue;
        }

        if (engine.process == nullptr || WaitForSingleObject(engine.process, 0) == WAIT_OBJECT_0)
        {
            engine.error = L"Pikafish 子进程已提前退出。";
            return false;
        }
        Sleep(2);
    }

    engine.error = L"等待 Pikafish 输出超时。";
    return false;
}

// 这个函数持续读取引擎输出，直到收到指定 UCI 同步标记。
static bool waitengineword(const std::string& word, int timeoutms)
{
    unsigned long starttick = GetTickCount(); // starttick 表示开始等待同步标记的毫秒时间。
    while (static_cast<unsigned long>(GetTickCount() - starttick) <= static_cast<unsigned long>(timeoutms))
    {
        int remainms = timeoutms - static_cast<int>(GetTickCount() - starttick); // remainms 表示尚可等待的毫秒数。
        std::string line; // line 保存一行 Pikafish 输出。
        if (!readengineline(line, remainms > 0 ? remainms : 0))
        {
            return false;
        }
        if (line == word)
        {
            return true;
        }
    }
    engine.error = L"Pikafish 未返回预期的 UCI 同步标记。";
    return false;
}

// 这个函数启动 Pikafish，完成 UCI、NNUE、线程和缓存初始化。
bool initrobotengine(const std::wstring& enginefolder)
{
    closerobotengine();
    engine.error.clear();

    std::filesystem::path folderpath(enginefolder);                 // folderpath 表示引擎文件夹路径。
    std::filesystem::path executablepath = folderpath / L"pikafish.exe"; // executablepath 表示 Pikafish 可执行文件路径。
    std::filesystem::path networkpath = folderpath / L"pikafish.nnue";   // networkpath 表示 Pikafish NNUE 网络文件路径。
    if (!std::filesystem::exists(executablepath) || !std::filesystem::exists(networkpath))
    {
        return failrobotengine(L"未找到 engine 文件夹中的 pikafish.exe 或 pikafish.nnue。");
    }

    SECURITY_ATTRIBUTES security = {}; // security 表示允许子进程继承管道句柄的安全属性。
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

    HANDLE childoutputwrite = nullptr; // childoutputwrite 表示子进程写入标准输出的管道端。
    HANDLE childinputread = nullptr;   // childinputread 表示子进程读取标准输入的管道端。
    if (!CreatePipe(&engine.outputread, &childoutputwrite, &security, 0) ||
        !SetHandleInformation(engine.outputread, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&childinputread, &engine.inputwrite, &security, 0) ||
        !SetHandleInformation(engine.inputwrite, HANDLE_FLAG_INHERIT, 0))
    {
        closehandle(childoutputwrite);
        closehandle(childinputread);
        return failrobotengine(L"创建 Pikafish UCI 通信管道失败。");
    }

    STARTUPINFOW startup = {}; // startup 保存 Pikafish 子进程的标准输入输出配置。
    startup.cb = sizeof(STARTUPINFOW);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childinputread;
    startup.hStdOutput = childoutputwrite;
    startup.hStdError = childoutputwrite;

    PROCESS_INFORMATION processinfo = {}; // processinfo 接收新建 Pikafish 进程及线程句柄。
    std::wstring commandline = L"\"" + executablepath.wstring() + L"\""; // commandline 表示创建引擎进程的命令行。
    BOOL created = CreateProcessW(executablepath.c_str(), commandline.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
        nullptr, folderpath.c_str(), &startup, &processinfo); // created 表示 Pikafish 子进程是否创建成功。
    closehandle(childoutputwrite);
    closehandle(childinputread);
    if (!created)
    {
        return failrobotengine(L"启动 Pikafish 子进程失败，请检查 CPU 是否支持 SSE4.1 和 POPCNT。");
    }

    engine.process = processinfo.hProcess;
    engine.thread = processinfo.hThread;
    engine.folder = enginefolder;
    if (!sendenginecommand("uci") || !waitengineword("uciok", 8000))
    {
        return failrobotengine(L"Pikafish UCI 初始化失败。");
    }

    SYSTEM_INFO systeminfo = {}; // systeminfo 接收当前电脑的逻辑处理器数量。
    GetSystemInfo(&systeminfo);
    int processorcount = static_cast<int>(systeminfo.dwNumberOfProcessors); // processorcount 表示系统可用的逻辑处理器数量。
    int threadcount = processorcount > 4 ? processorcount - 2 : (processorcount >= 2 ? 2 : 1); // threadcount 表示为 Pikafish 保留界面余量后的搜索线程数。
    if (threadcount > 8)
    {
        threadcount = 8;
    }

    if (!sendenginecommand("setoption name Threads value " + std::to_string(threadcount)) ||
        !sendenginecommand("setoption name Hash value 256") ||
        !sendenginecommand("setoption name EvalFile value pikafish.nnue") ||
        !sendenginecommand("setoption name MultiPV value 1") ||
        !sendenginecommand("isready") || !waitengineword("readyok", 12000))
    {
        return failrobotengine(L"Pikafish NNUE 网络或搜索参数初始化失败。");
    }

    engine.multipv = 1;
    if (!sendenginecommand("ucinewgame") || !sendenginecommand("isready") || !waitengineword("readyok", 5000))
    {
        return failrobotengine(L"Pikafish 新对局初始化失败。");
    }
    engine.ready = true;
    return true;
}

// 这个函数返回 Pikafish 引擎是否已可用。
bool isrobotengineready()
{
    return engine.ready;
}

// 这个函数返回最近一次 Pikafish 引擎错误说明。
std::wstring getrobotengineerror()
{
    return engine.error;
}

// 这个函数把棋盘行列转换为 Pikafish 使用的 UCI 坐标。
static std::string getucisquare(int row, int col)
{
    std::string square; // square 保存由字母列和数字行组成的 UCI 坐标。
    square.push_back(static_cast<char>('a' + col));
    square.push_back(static_cast<char>('0' + board_rows - 1 - row));
    return square;
}

// 这个函数把历史走法转换为 Pikafish position startpos moves 命令。
static std::string getpositioncommand(const gamestate& state)
{
    std::string command = "position startpos"; // command 保存设置当前局面的完整 UCI 命令。
    if (!state.history.empty())
    {
        command += " moves";
        for (const moveinfo& move : state.history)
        {
            command += " " + getucisquare(move.fromrow, move.fromcol) + getucisquare(move.torow, move.tocol);
        }
    }
    return command;
}

// 这个函数按空格拆分一行 UCI 输出。
static std::vector<std::string> splitenginetokens(const std::string& line)
{
    std::istringstream stream(line); // stream 表示用于分割引擎输出的字符串流。
    std::vector<std::string> tokens; // tokens 保存拆分后的 UCI 单词。
    std::string token;               // token 表示正在读取的一个 UCI 单词。
    while (stream >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

// 这个函数解析 Pikafish info 行中的深度、排名、分数和主变首步。
static void parseengineinfo(const std::string& line, std::vector<enginecandidate>& candidates)
{
    if (line.rfind("info ", 0) != 0)
    {
        return;
    }

    std::vector<std::string> tokens = splitenginetokens(line); // tokens 保存当前 info 行的所有 UCI 单词。
    int depth = -1;                                           // depth 表示当前 info 行的搜索深度。
    int multipv = 1;                                         // multipv 表示当前 info 行的候选排名。
    int score = 0;                                           // score 表示当前 info 行的引擎评分。
    bool hasscore = false;                                   // hasscore 表示当前 info 行是否包含评分。
    std::string movetext;                                    // movetext 表示主要变化中的第一步走法。

    for (int index = 0; index < static_cast<int>(tokens.size()); ++index)
    {
        if (tokens[index] == "depth" && index + 1 < static_cast<int>(tokens.size()))
        {
            depth = std::stoi(tokens[++index]);
        }
        else if (tokens[index] == "multipv" && index + 1 < static_cast<int>(tokens.size()))
        {
            multipv = std::stoi(tokens[++index]);
        }
        else if (tokens[index] == "score" && index + 2 < static_cast<int>(tokens.size()))
        {
            std::string scoretype = tokens[++index]; // scoretype 表示评分是普通分还是将死步数。
            int scorevalue = std::stoi(tokens[++index]); // scorevalue 表示引擎输出的原始评分数值。
            score = scoretype == "mate" ? (scorevalue > 0 ? 100000 - scorevalue : -100000 - scorevalue) : scorevalue;
            hasscore = true;
        }
        else if (tokens[index] == "pv" && index + 1 < static_cast<int>(tokens.size()))
        {
            movetext = tokens[index + 1];
            break;
        }
    }

    if (depth < 0 || !hasscore || movetext.size() < 4 || multipv < 1 || multipv > static_cast<int>(candidates.size()))
    {
        return;
    }

    enginecandidate& candidate = candidates[multipv - 1]; // candidate 表示当前排名对应的候选走法。
    if (!candidate.valid || depth >= candidate.depth)
    {
        candidate.movetext = movetext;
        candidate.score = score;
        candidate.depth = depth;
        candidate.valid = true;
    }
}

// 这个函数设置 Pikafish 需要输出的主要变化数量。
static bool setenginemultipv(int value)
{
    if (engine.multipv == value)
    {
        return true;
    }
    if (!sendenginecommand("setoption name MultiPV value " + std::to_string(value)) ||
        !sendenginecommand("isready") || !waitengineword("readyok", 3000))
    {
        return false;
    }
    engine.multipv = value;
    return true;
}

// 这个函数请求 Pikafish 在指定毫秒内搜索当前局面。
static bool searchengine(const gamestate& state, int movetimems, int multipv, std::vector<enginecandidate>& candidates, std::string& bestmovetext)
{
    if (!engine.ready)
    {
        engine.error = L"Pikafish 引擎尚未初始化。";
        return false;
    }

    if (robotsearchcancelled)
    {
        return false;
    }

    candidates.assign(multipv, enginecandidate{ "", 0, -1, false });
    if (!setenginemultipv(multipv) || !sendenginecommand(getpositioncommand(state)))
    {
        return false;
    }

    if (robotsearchcancelled || !sendenginecommand("go movetime " + std::to_string(movetimems)))
    {
        return false;
    }
    enginesearching = true;
    if (robotsearchcancelled)
    {
        sendenginecommand("stop");
    }

    unsigned long starttick = GetTickCount(); // starttick 表示本次引擎搜索开始时的毫秒时间。
    int timeoutms = movetimems + 6000;         // timeoutms 表示搜索输出的硬超时上限。
    while (static_cast<unsigned long>(GetTickCount() - starttick) <= static_cast<unsigned long>(timeoutms))
    {
        int remainms = timeoutms - static_cast<int>(GetTickCount() - starttick); // remainms 表示尚可等待搜索输出的毫秒数。
        std::string line; // line 保存一行 Pikafish 搜索输出。
        if (!readengineline(line, remainms > 0 ? remainms : 0))
        {
            enginesearching = false;
            return false;
        }
        parseengineinfo(line, candidates);
        if (line.rfind("bestmove ", 0) == 0)
        {
            std::vector<std::string> tokens = splitenginetokens(line); // tokens 保存 bestmove 行拆分后的 UCI 单词。
            if (tokens.size() >= 2 && tokens[1] != "(none)")
            {
                bestmovetext = tokens[1];
                if (!candidates.empty() && !candidates[0].valid)
                {
                    candidates[0] = enginecandidate{ bestmovetext, 0, 0, true };
                }
                enginesearching = false;
                return true;
            }
            enginesearching = false;
            engine.error = L"Pikafish 认为当前局面没有可走步。";
            return false;
        }
    }
    enginesearching = false;
    engine.error = L"Pikafish 搜索超时。";
    return false;
}

// 这个函数返回用于双机器人开局变化的随机数引擎。
static std::mt19937& getrobotrandomengine()
{
    static std::mt19937 randomengine(std::random_device{}()); // randomengine 表示每次程序启动都不同的随机数引擎。
    return randomengine;
}

// 这个函数仅在开局从评分非常接近的 Pikafish 候选中选择一步。
static std::string chooseenginemove(const std::vector<enginecandidate>& candidates, const std::string& bestmovetext, bool balancedmode, int historysize)
{
    if (!balancedmode || historysize >= 16 || candidates.empty() || !candidates[0].valid || std::abs(candidates[0].score) > 50000)
    {
        return bestmovetext;
    }

    int bestscore = candidates[0].score; // bestscore 表示 Pikafish 第一推荐走法的评分。
    std::vector<std::string> movechoices; // movechoices 保存与最优走法分差不超过 8 的开局候选。
    std::vector<double> weights;          // weights 保存候选走法的引擎质量权重。
    for (const enginecandidate& candidate : candidates)
    {
        if (!candidate.valid || candidate.score < bestscore - 8)
        {
            continue;
        }
        if (std::find(movechoices.begin(), movechoices.end(), candidate.movetext) != movechoices.end())
        {
            continue;
        }
        movechoices.push_back(candidate.movetext);
        weights.push_back(std::exp(static_cast<double>(candidate.score - bestscore) / 3.0));
    }

    if (movechoices.size() <= 1)
    {
        return bestmovetext;
    }
    std::discrete_distribution<int> distribution(weights.begin(), weights.end()); // distribution 表示按 Pikafish 分数加权的候选抽取分布。
    int selectedindex = distribution(getrobotrandomengine()); // selectedindex 表示本次抽中的高质量候选索引。
    return movechoices[selectedindex];
}

// 这个函数把 UCI 走法转换为工程内部走法，并用现有规则再次校验合法性。
static robotmove convertenginemove(const gamestate& state, const std::string& movetext, int enginescore)
{
    robotmove move = { empty_id, -1, -1, static_cast<double>(enginescore), false }; // move 表示初始为无效的内部机器人走法。
    if (movetext.size() < 4)
    {
        return move;
    }

    int fromcol = movetext[0] - 'a';                // fromcol 表示 UCI 走法起点列号。
    int fromrow = board_rows - 1 - (movetext[1] - '0'); // fromrow 表示 UCI 走法起点行号。
    int tocol = movetext[2] - 'a';                  // tocol 表示 UCI 走法终点列号。
    int torow = board_rows - 1 - (movetext[3] - '0');   // torow 表示 UCI 走法终点行号。
    if (!isinsideboard(fromrow, fromcol) || !isinsideboard(torow, tocol))
    {
        return move;
    }

    int pieceid = getpieceidat(state, fromrow, fromcol); // pieceid 表示 UCI 走法起点上的棋子编号。
    if (pieceid == empty_id || state.pieces[pieceid].color != state.currentcolor)
    {
        return move;
    }

    std::vector<point> legalmoves = getlegalmoves(state, pieceid); // legalmoves 保存本工程规则层计算的全部合法落点。
    for (const point& target : legalmoves)
    {
        if (target.row == torow && target.col == tocol)
        {
            move.pieceid = pieceid;
            move.torow = torow;
            move.tocol = tocol;
            move.valid = true;
            return move;
        }
    }
    return move;
}

// 这个函数调用 Pikafish NNUE 搜索当前局面的高质量走法。
robotmove getbestrobotmove(const gamestate& state, int color, bool balancedmode, int movetimems)
{
    robotmove invalidmove = { empty_id, -1, -1, 0.0, false }; // invalidmove 表示搜索失败时返回的无效走法。
    if (state.gameover || state.currentcolor != color || !engine.ready || robotsearchcancelled)
    {
        return invalidmove;
    }

    int safetime = movetimems < 300 ? 300 : movetimems; // safetime 表示经过搜索时间下限保护的毫秒数。
    int multipv = balancedmode && state.history.size() < 16 ? 3 : 1; // multipv 表示本回合需要 Pikafish 输出的高质量候选数。
    std::vector<enginecandidate> candidates; // candidates 接收 Pikafish 返回的排名候选。
    std::string bestmovetext;                // bestmovetext 保存 Pikafish bestmove 行的第一推荐。
    if (!searchengine(state, safetime, multipv, candidates, bestmovetext))
    {
        return invalidmove;
    }

    std::string selectedtext = chooseenginemove(candidates, bestmovetext, balancedmode, static_cast<int>(state.history.size())); // selectedtext 表示经过优质开局变化后选中的 UCI 走法。
    int selectedscore = candidates.empty() ? 0 : candidates[0].score; // selectedscore 表示选中走法对应的 Pikafish 分数。
    for (const enginecandidate& candidate : candidates)
    {
        if (candidate.valid && candidate.movetext == selectedtext)
        {
            selectedscore = candidate.score;
            break;
        }
    }

    robotmove move = convertenginemove(state, selectedtext, selectedscore); // move 表示已转换并校验的内部机器人走法。
    if (!move.valid && selectedtext != bestmovetext)
    {
        move = convertenginemove(state, bestmovetext, candidates.empty() ? 0 : candidates[0].score);
    }
    if (!move.valid)
    {
        engine.error = L"Pikafish 返回的走法未通过本工程象棋规则校验。";
    }
    return move;
}

// 这个函数复制当前局面并在后台线程启动 Pikafish 搜索，避免阻塞 EasyX 主循环。
bool startrobotsearch(const gamestate& state, int color, bool balancedmode, int movetimems)
{
    if (robotsearchactive)
    {
        engine.error = L"机器人已有一个搜索任务正在运行。";
        return false;
    }
    if (!engine.ready)
    {
        engine.error = L"Pikafish 引擎尚未初始化。";
        return false;
    }

    gamestate statecopy = state; // statecopy 保存后台线程独立使用的只读局面副本。
    robotsearchcancelled = false;
    enginesearching = false;
    try
    {
        robotsearchfuture = std::async(std::launch::async, [statecopy, color, balancedmode, movetimems]()
        {
            return getbestrobotmove(statecopy, color, balancedmode, movetimems);
        });
        robotsearchactive = true;
    }
    catch (...)
    {
        engine.error = L"无法创建机器人后台搜索线程。";
        robotsearchactive = false;
        return false;
    }
    return true;
}

// 这个函数无阻塞检查后台搜索是否完成，并在完成时取回机器人走法。
bool pollrobotsearch(robotmove& move)
{
    if (!robotsearchactive || !robotsearchfuture.valid())
    {
        return false;
    }
    if (robotsearchfuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    {
        return false;
    }

    try
    {
        move = robotsearchfuture.get();
    }
    catch (...)
    {
        move = { empty_id, -1, -1, 0.0, false };
        engine.error = L"机器人后台搜索线程异常退出。";
    }
    robotsearchactive = false;
    enginesearching = false;
    return true;
}

// 这个函数重置机器人可视化走棋阶段。
void resetrobotruntime(robotruntime& runtime)
{
    cancelrobotsearch();
    runtime.phase = robot_phase_none;
    runtime.move.pieceid = empty_id;
    runtime.move.torow = -1;
    runtime.move.tocol = -1;
    runtime.move.score = 0.0;
    runtime.move.valid = false;
    runtime.tick = 0;
    runtime.delayms = 0;
}
