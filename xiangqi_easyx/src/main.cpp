#include "board.h"
#include "logger.h"
#include "ui.h"
#include "analysis.h"
#include "robot.h"
#include "time.h"

#include <graphics.h>
#include <filesystem>
#include <vector>
#include <windows.h>

// 这个函数判断某个棋盘点是否在合法落点列表中。
static bool isinmoves(const std::vector<point>& moves, int row, int col)
{
    for (const point& target : moves)
    {
        if (target.row == row && target.col == col)
        {
            return true;
        }
    }
    return false;
}

// 这个函数清空当前选中的棋子和可落点。
static void clearselection(int& selectedid, std::vector<point>& moves)
{
    selectedid = empty_id;
    moves.clear();
}

// 这个函数选中一枚棋子，并计算它的合法落点。
static void selectpiece(const gamestate& state, int pieceid, int& selectedid, std::vector<point>& moves)
{
    selectedid = pieceid;
    moves = getlegalmoves(state, pieceid);
}

// 这个函数处理悔棋按钮点击。
static bool handleundo(gamestate& state, timestate& timer, loggerstate& logger, int& selectedid, std::vector<point>& moves)
{
    moveinfo info; // info 表示被撤回的走法。
    if (undomove(state, &info))
    {
        undotimer(timer);
        logundo(logger, state, info);
        clearselection(selectedid, moves);
        return true;
    }
    return false;
}

// 这个函数处理棋盘区域点击。
static bool handleboardclick(gamestate& state, timestate& timer, loggerstate& logger, int x, int y, int& selectedid, std::vector<point>& moves)
{
    int row = 0; // row 表示鼠标点击对应的棋盘行号。
    int col = 0; // col 表示鼠标点击对应的棋盘列号。
    if (!getboardrowcol(x, y, row, col))
    {
        return false;
    }

    if (state.gameover)
    {
        return false;
    }

    int clickedid = getpieceidat(state, row, col); // clickedid 表示被点击位置上的棋子编号。

    if (selectedid == empty_id)
    {
        if (clickedid != empty_id && state.pieces[clickedid].color == state.currentcolor)
        {
            selectpiece(state, clickedid, selectedid, moves);
        }
        return false;
    }

    if (isinmoves(moves, row, col))
    {
        moveinfo info; // info 表示成功走出的走法。
        if (movepiece(state, selectedid, row, col, &info))
        {
            switchtimer(timer, state.currentcolor);
            logmove(logger, state, info);
            clearselection(selectedid, moves);
            return true;
        }
        return false;
    }

    if (clickedid != empty_id && state.pieces[clickedid].color == state.currentcolor)
    {
        selectpiece(state, clickedid, selectedid, moves);
    }
    return false;
}

// 这个函数判断目录是否是本工程目录。
static bool isprojectfolder(const std::filesystem::path& folder)
{
    return std::filesystem::exists(folder / L"xiangqi_easyx.vcxproj");
}

// 这个函数从指定目录向上寻找工程目录。
static std::filesystem::path findprojectfolder(std::filesystem::path folder)
{
    for (int count = 0; count < 8 && !folder.empty(); ++count)
    {
        if (isprojectfolder(folder))
        {
            return folder;
        }

        std::filesystem::path childproject = folder / L"xiangqi_easyx"; // childproject 表示可能的工程子目录。
        if (isprojectfolder(childproject))
        {
            return childproject;
        }

        std::filesystem::path parent = folder.parent_path(); // parent 表示上一层目录。
        if (parent == folder)
        {
            break;
        }
        folder = parent;
    }
    return std::filesystem::path();
}

// 这个函数返回工程目录下的 result 文件夹路径。
static std::wstring getresultfolder()
{
    std::filesystem::path projectpath = findprojectfolder(std::filesystem::current_path()); // projectpath 表示工程目录。
    if (!projectpath.empty())
    {
        return (projectpath / L"result").wstring();
    }

    wchar_t modulepath[MAX_PATH] = {}; // modulepath 表示当前程序的宽字符完整路径。
    DWORD length = GetModuleFileNameW(nullptr, modulepath, MAX_PATH); // length 表示读取到的路径长度。
    if (length > 0)
    {
        std::filesystem::path exefolder = std::filesystem::path(modulepath).parent_path(); // exefolder 表示程序所在目录。
        projectpath = findprojectfolder(exefolder);
        if (!projectpath.empty())
        {
            return (projectpath / L"result").wstring();
        }
    }

    return (std::filesystem::current_path() / L"result").wstring();
}

// 这个函数返回工程目录下的 Pikafish engine 文件夹路径。
static std::wstring getenginefolder()
{
    std::filesystem::path projectpath = findprojectfolder(std::filesystem::current_path()); // projectpath 表示从当前目录找到的工程目录。
    if (!projectpath.empty())
    {
        return (projectpath / L"engine").wstring();
    }

    wchar_t modulepath[MAX_PATH] = {}; // modulepath 表示当前程序的宽字符完整路径。
    DWORD length = GetModuleFileNameW(nullptr, modulepath, MAX_PATH); // length 表示读取到的程序路径长度。
    if (length > 0)
    {
        std::filesystem::path exefolder = std::filesystem::path(modulepath).parent_path(); // exefolder 表示可执行文件所在目录。
        projectpath = findprojectfolder(exefolder);
        if (!projectpath.empty())
        {
            return (projectpath / L"engine").wstring();
        }
    }
    return (std::filesystem::current_path() / L"engine").wstring();
}

// 这个函数记录计时规则。
static void logtimesetting(loggerstate& logger, const timesetting& setting)
{
    std::wstring text = L"计时规则：每步" + std::to_wstring(setting.stepseconds) + L"秒，单方总时长" + std::to_wstring(setting.sideseconds / 60) + L"分钟。"; // text 表示计时规则日志。
    logmessage(logger, text);
}

// 这个函数记录机器人配置。
static void logrobotsetting(loggerstate& logger, const robotsetting& setting)
{
    std::wstring text = L"对战模式："; // text 表示机器人配置日志。
    if (setting.mode == robot_mode_red)
    {
        text += L"红方机器人，黑方玩家。";
    }
    else if (setting.mode == robot_mode_black)
    {
        text += L"红方玩家，黑方机器人。";
    }
    else if (setting.mode == robot_mode_both)
    {
        text += L"红黑双方都由 Pikafish 高质量机器人控制。";
    }
    else
    {
        text += L"红黑双方都由玩家控制。";
    }
    logmessage(logger, text);
}

// 这个函数处理超时判负。
static bool handletimeout(gamestate& state, timestate& timer, loggerstate& logger, int timeoutcolor, int& selectedid, std::vector<point>& moves)
{
    if (timeoutcolor == color_none || state.gameover)
    {
        return false;
    }

    state.gameover = true;
    state.winnercolor = getoppositecolor(timeoutcolor);
    stoptimer(timer);
    clearselection(selectedid, moves);

    std::wstring text = getplayertext(timeoutcolor) + L"时间用完，" + getplayertext(state.winnercolor) + L"获胜。"; // text 表示超时日志。
    logmessage(logger, text);
    return true;
}

// 这个函数返回机器人每个可视化动作需要等待的毫秒数。
static int getrobotdelayms(const timestate& timer)
{
    int maxdelay = timer.steplimit * 150; // maxdelay 表示保证两段机器人动作不超过步时长30%的单段上限。
    if (maxdelay < 200)
    {
        return maxdelay;
    }
    return maxdelay < 2000 ? maxdelay : 2000;
}

// 这个函数在机器人整步耗时不超过步时长百分之三十的前提下计算 Pikafish 搜索时间。
static int getrobotsearchms(const timestate& timer)
{
    int delayms = getrobotdelayms(timer); // delayms 表示选中棋子和执行落子前的单段等待时间。
    int maxsearchms = timer.steplimit * 300 - delayms * 2 - 800; // maxsearchms 表示扣除两段可视化等待和通信余量后的搜索上限。
    int preferredms = timer.steplimit <= 30 ? 4200 : (timer.steplimit <= 60 ? 7000 : 9000); // preferredms 表示不同步时长下的高强度 Pikafish 搜索时间。
    int searchms = preferredms < maxsearchms ? preferredms : maxsearchms; // searchms 表示最终交给 Pikafish 的搜索毫秒数。
    return searchms < 300 ? 300 : searchms;
}

// 这个函数判断当前是否应屏蔽玩家对棋盘和悔棋的操作。
static bool shouldblockhumaninput(const gamestate& state, const robotsetting& robots, const robotruntime& robotrun)
{
    if (state.gameover)
    {
        return false;
    }
    return robotrun.phase != robot_phase_none || isrobotcolor(robots, state.currentcolor);
}

// 这个函数推进机器人当前回合的可视化走棋过程。
static bool updaterobotturn(gamestate& state, timestate& timer, loggerstate& logger, const robotsetting& robots, robotruntime& robotrun, analysisresult& analysis, int& selectedid, std::vector<point>& moves)
{
    if (state.gameover || !isrobotcolor(robots, state.currentcolor))
    {
        if (robotrun.phase != robot_phase_none)
        {
            resetrobotruntime(robotrun);
        }
        return false;
    }

    unsigned long long now = static_cast<unsigned long long>(GetTickCount()); // now 表示当前毫秒时间。

    if (robotrun.phase == robot_phase_none)
    {
        bool balancedmode = robots.mode == robot_mode_both; // balancedmode 表示是否启用双机器人平衡对局策略。
        int searchms = getrobotsearchms(timer);              // searchms 表示本回合 Pikafish 可用的搜索毫秒数。
        if (!startrobotsearch(state, state.currentcolor, balancedmode, searchms))
        {
            std::wstring errortext = L"机器人引擎无法启动后台搜索：\n" + getrobotengineerror(); // errortext 表示向玩家显示并写入日志的引擎错误。
            logmessage(logger, errortext);
            state.gameover = true;
            state.winnercolor = getoppositecolor(state.currentcolor);
            stoptimer(timer);
            clearselection(selectedid, moves);
            MessageBoxW(nullptr, errortext.c_str(), L"Pikafish 引擎错误", MB_OK | MB_ICONERROR);
            return true;
        }
        clearselection(selectedid, moves);
        robotrun.phase = robot_phase_searching;
        return false;
    }

    if (robotrun.phase == robot_phase_searching)
    {
        if (!pollrobotsearch(robotrun.move))
        {
            return false;
        }
        if (!robotrun.move.valid)
        {
            std::wstring errortext = L"机器人引擎无法给出合法走法：\n" + getrobotengineerror(); // errortext 表示向玩家显示并写入日志的引擎错误。
            logmessage(logger, errortext);
            state.gameover = true;
            state.winnercolor = getoppositecolor(state.currentcolor);
            stoptimer(timer);
            clearselection(selectedid, moves);
            MessageBoxW(nullptr, errortext.c_str(), L"Pikafish 引擎错误", MB_OK | MB_ICONERROR);
            return true;
        }
        robotrun.phase = robot_phase_wait_select;
        robotrun.tick = static_cast<unsigned long long>(GetTickCount());
        robotrun.delayms = getrobotdelayms(timer);
        return false;
    }

    if (now - robotrun.tick < static_cast<unsigned long long>(robotrun.delayms))
    {
        return false;
    }

    if (robotrun.phase == robot_phase_wait_select)
    {
        selectedid = robotrun.move.pieceid;
        moves = getlegalmoves(state, selectedid);
        robotrun.phase = robot_phase_wait_move;
        robotrun.tick = now;
        robotrun.delayms = getrobotdelayms(timer);
        return true;
    }

    if (robotrun.phase == robot_phase_wait_move)
    {
        moveinfo info; // info 表示机器人实际走出的走法。
        bool moved = movepiece(state, robotrun.move.pieceid, robotrun.move.torow, robotrun.move.tocol, &info);
        resetrobotruntime(robotrun);
        if (moved)
        {
            switchtimer(timer, state.currentcolor);
            logmove(logger, state, info);
            clearselection(selectedid, moves);
            analysis = getanalysis(state);
            if (state.gameover)
            {
                stoptimer(timer);
            }
            return true;
        }

        clearselection(selectedid, moves);
        return true;
    }

    resetrobotruntime(robotrun);
    return true;
}

// 这个函数是程序公共入口，负责启动 EasyX 游戏窗口和主循环。
int main()
{
    gamestate state;              // state 保存整局游戏当前状态。
    timesetting setting;          // setting 保存玩家选择的计时规则。
    timestate timer;              // timer 保存整局游戏计时状态。
    analysisresult analysis;      // analysis 保存当前局势分析结果。
    robotsetting robots;          // robots 保存本局机器人配置。
    robotruntime robotrun;        // robotrun 保存机器人可视化走棋过程。
    loggerstate logger;           // logger 保存操作日志文件状态。
    int selectedid = empty_id;    // selectedid 表示当前选中棋子的编号。
    std::vector<point> moves;     // moves 保存当前选中棋子的可落点。
    buttonrect undobutton;        // undobutton 保存悔棋按钮区域。

    initgraph(window_width, window_height);
    setbkcolor(RGB(233, 236, 225));
    cleardevice();
    if (!choosegamesetting(setting, robots))
    {
        closegraph();
        return 0;
    }

    if ((robots.redrobot || robots.blackrobot) && !initrobotengine(getenginefolder()))
    {
        std::wstring errortext = L"无法启动 Pikafish 高质量象棋引擎：\n" + getrobotengineerror(); // errortext 表示引擎启动失败时的用户提示。
        MessageBoxW(nullptr, errortext.c_str(), L"Pikafish 引擎错误", MB_OK | MB_ICONERROR);
        closegraph();
        return 0;
    }

    initgame(state);
    inittimer(timer, setting);
    resetrobotruntime(robotrun);
    analysis = getanalysis(state);
    initlogger(logger, getresultfolder());
    logstart(logger, state);
    logtimesetting(logger, setting);
    logrobotsetting(logger, robots);

    undobutton.left = panel_left + 94;
    undobutton.top = 124;
    undobutton.right = panel_left + 252;
    undobutton.bottom = 160;

    drawgame(state, timer, analysis, selectedid, moves, undobutton);

    bool running = true; // running 表示游戏窗口主循环是否继续。
    bool needdraw = false; // needdraw 表示本轮循环是否需要重绘。
    unsigned long long lastdraw = static_cast<unsigned long long>(GetTickCount()); // lastdraw 表示上一次刷新画面的毫秒时间。
    while (running)
    {
        int timeoutcolor = updatetimer(timer); // timeoutcolor 表示刚刚超时的一方。
        if (handletimeout(state, timer, logger, timeoutcolor, selectedid, moves))
        {
            resetrobotruntime(robotrun);
            needdraw = true;
        }

        if (updaterobotturn(state, timer, logger, robots, robotrun, analysis, selectedid, moves))
        {
            needdraw = true;
        }

        ExMessage message; // message 表示 EasyX 取出的输入消息。
        if (peekmessage(&message, EM_MOUSE | EM_KEY))
        {
            if (message.message == WM_LBUTTONDOWN)
            {
                timeoutcolor = updatetimer(timer);
                if (handletimeout(state, timer, logger, timeoutcolor, selectedid, moves))
                {
                    resetrobotruntime(robotrun);
                    needdraw = true;
                }

                if (shouldblockhumaninput(state, robots, robotrun))
                {
                    needdraw = true;
                }
                else if (isinbutton(message.x, message.y, undobutton))
                {
                    if (handleundo(state, timer, logger, selectedid, moves))
                    {
                        resetrobotruntime(robotrun);
                        analysis = getanalysis(state);
                        needdraw = true;
                    }
                }
                else
                {
                    if (handleboardclick(state, timer, logger, message.x, message.y, selectedid, moves))
                    {
                        analysis = getanalysis(state);
                        if (state.gameover)
                        {
                            stoptimer(timer);
                        }
                        needdraw = true;
                    }
                    else
                    {
                        needdraw = true;
                    }
                }
            }
            else if (message.message == WM_KEYDOWN)
            {
                if (message.vkcode == VK_ESCAPE)
                {
                    running = false;
                }
                else if (message.vkcode == 'U')
                {
                    if (!shouldblockhumaninput(state, robots, robotrun) && handleundo(state, timer, logger, selectedid, moves))
                    {
                        resetrobotruntime(robotrun);
                        analysis = getanalysis(state);
                        needdraw = true;
                    }
                }
            }
        }

        unsigned long long now = static_cast<unsigned long long>(GetTickCount()); // now 表示当前毫秒时间。
        if (needdraw || (timer.running && now - lastdraw >= 180))
        {
            drawgame(state, timer, analysis, selectedid, moves, undobutton);
            lastdraw = now;
            needdraw = false;
        }
        Sleep(10);
    }

    logmessage(logger, L"游戏窗口关闭。");
    closelogger(logger);
    closerobotengine();
    closegraph();
    return 0;
}
