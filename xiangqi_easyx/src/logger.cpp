#include "logger.h"

#include <iomanip>
#include <sstream>
#include <ctime>
#include <windows.h>

// 这个函数把宽字符串转为 UTF-8，方便 txt 文件保存中文。
static std::string wstringtoutf8(const std::wstring& text)
{
    if (text.empty())
    {
        return std::string();
    }

    int length = static_cast<int>(text.size()); // length 表示不含结尾空字符的宽字符数量。
    int needed = WideCharToMultiByte(CP_UTF8, 0, text.data(), length, nullptr, 0, nullptr, nullptr); // needed 表示所需字节数。
    std::string result(needed, '\0'); // result 表示转换后的 UTF-8 内容。
    WideCharToMultiByte(CP_UTF8, 0, text.data(), length, result.data(), needed, nullptr, nullptr);
    return result;
}

// 这个函数生成当前时间文件名。
static std::wstring gettimestamp()
{
    std::time_t now = std::time(nullptr); // now 表示当前系统时间。
    std::tm localtimevalue{};             // localtimevalue 表示本地时间结构。
    localtime_s(&localtimevalue, &now);

    std::wstringstream stream; // stream 表示时间格式化输出流。
    stream << std::put_time(&localtimevalue, L"%Y%m%d_%H%M%S");
    return stream.str();
}

// 这个函数把棋盘坐标格式化为文本。
static std::wstring getpositiontext(int row, int col)
{
    return L"第" + std::to_wstring(row + 1) + L"行第" + std::to_wstring(col + 1) + L"列";
}

// 这个函数写入一行日志。
static void writeline(loggerstate& logger, const std::wstring& text)
{
    if (!logger.opened)
    {
        return;
    }

    std::string line = wstringtoutf8(text + L"\r\n"); // line 表示带换行的 UTF-8 文本。
    logger.file.write(line.data(), static_cast<std::streamsize>(line.size()));
    logger.file.flush();
}

// 这个函数打开日志文件，并在 result 文件夹中用生成时间命名。
bool initlogger(loggerstate& logger, const std::wstring& folder)
{
    logger.count = 0;
    logger.opened = false;

    std::filesystem::path folderpath(folder);       // folderpath 表示日志文件夹路径。
    std::filesystem::create_directories(folderpath);
    logger.path = folderpath / (gettimestamp() + L".txt");
    logger.file.open(logger.path, std::ios::binary);

    if (!logger.file.is_open())
    {
        return false;
    }

    const unsigned char bom[3] = { 0xef, 0xbb, 0xbf }; // bom 表示 UTF-8 文件头。
    logger.file.write(reinterpret_cast<const char*>(bom), 3);
    logger.opened = true;
    return true;
}

// 这个函数关闭日志文件。
void closelogger(loggerstate& logger)
{
    if (logger.file.is_open())
    {
        logger.file.close();
    }
    logger.opened = false;
}

// 这个函数记录游戏开始信息。
void logstart(loggerstate& logger, const gamestate& state)
{
    std::wstring text = L"游戏开始，当前是" + getplayertext(state.currentcolor) + L"。"; // text 表示开始日志内容。
    writeline(logger, text);
}

// 这个函数记录一次真实走棋。
void logmove(loggerstate& logger, const gamestate& state, const moveinfo& info)
{
    ++logger.count;
    const piece& moving = state.pieces[info.pieceid]; // moving 表示刚刚移动后的棋子。

    std::wstring text = L"第" + std::to_wstring(logger.count) + L"步："; // text 表示本步日志内容。
    text += getplayertext(info.playercolor);
    text += L"移动";
    text += getpiecename(moving.type, moving.color);
    text += L"，从";
    text += getpositiontext(info.fromrow, info.fromcol);
    text += L"到";
    text += getpositiontext(info.torow, info.tocol);

    if (info.captureid != empty_id)
    {
        const piece& captured = state.pieces[info.captureid]; // captured 表示本步被吃掉的棋子。
        text += L"，吃掉";
        text += getpiecename(captured.type, captured.color);
    }

    if (state.gameover)
    {
        text += L"，游戏结束，";
        text += getplayertext(state.winnercolor);
        text += L"获胜";
    }

    writeline(logger, text);
}

// 这个函数记录一次悔棋操作。
void logundo(loggerstate& logger, const gamestate& state, const moveinfo& info)
{
    const piece& moving = state.pieces[info.pieceid]; // moving 表示被撤回的棋子。
    std::wstring text = L"悔棋：撤回";               // text 表示悔棋日志内容。
    text += getplayertext(info.playercolor);
    text += L"的";
    text += getpiecename(moving.type, moving.color);
    text += L"，从";
    text += getpositiontext(info.torow, info.tocol);
    text += L"回到";
    text += getpositiontext(info.fromrow, info.fromcol);

    if (info.captureid != empty_id)
    {
        const piece& captured = state.pieces[info.captureid]; // captured 表示悔棋后恢复的棋子。
        text += L"，恢复";
        text += getpiecename(captured.type, captured.color);
    }

    writeline(logger, text);
}

// 这个函数记录普通说明信息。
void logmessage(loggerstate& logger, const std::wstring& text)
{
    writeline(logger, text);
}

// 这个函数返回日志路径。
std::wstring getlogpath(const loggerstate& logger)
{
    return logger.path.wstring();
}
