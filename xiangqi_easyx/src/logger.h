#pragma once

#include "board.h"
#include <filesystem>
#include <fstream>
#include <string>

// loggerstate 保存日志文件状态。
struct loggerstate
{
    std::ofstream file;       // file 表示正在写入的日志文件。
    std::filesystem::path path; // path 表示日志文件完整路径。
    int count;                // count 表示已经记录的有效步数。
    bool opened;              // opened 表示日志是否成功打开。
};

bool initlogger(loggerstate& logger, const std::wstring& folder);
void closelogger(loggerstate& logger);
void logstart(loggerstate& logger, const gamestate& state);
void logmove(loggerstate& logger, const gamestate& state, const moveinfo& info);
void logundo(loggerstate& logger, const gamestate& state, const moveinfo& info);
void logmessage(loggerstate& logger, const std::wstring& text);
std::wstring getlogpath(const loggerstate& logger);
