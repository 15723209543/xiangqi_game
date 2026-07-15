#pragma once

#include "board.h"
#include <string>

// robotsetting 保存本局机器人配置。
struct robotsetting
{
    int mode;        // mode 表示本局选择的对战模式。
    bool redrobot;   // redrobot 表示红方是否由机器人控制。
    bool blackrobot; // blackrobot 表示黑方是否由机器人控制。
};

// robotmove 保存机器人选择的一步棋。
struct robotmove
{
    int pieceid;   // pieceid 表示机器人选择的棋子编号。
    int torow;     // torow 表示机器人落点行号。
    int tocol;     // tocol 表示机器人落点列号。
    double score;  // score 表示机器人评估分。
    bool valid;    // valid 表示是否找到可用走法。
};

// robotruntime 保存机器人当前可视化走棋阶段。
struct robotruntime
{
    int phase;                 // phase 表示机器人当前阶段。
    robotmove move;            // move 表示机器人本回合选择的走法。
    unsigned long long tick;   // tick 表示当前阶段开始的毫秒时间。
    int delayms;               // delayms 表示当前阶段需要等待的毫秒数。
};

const int robot_phase_none = 0;        // robot_phase_none 表示机器人没有正在执行动作。
const int robot_phase_searching = 1;   // robot_phase_searching 表示机器人正在后台思考走法。
const int robot_phase_wait_select = 2; // robot_phase_wait_select 表示等待后选中棋子。
const int robot_phase_wait_move = 3;   // robot_phase_wait_move 表示等待后执行落子。
const int robot_mode_players = 0;      // robot_mode_players 表示红黑双方都由玩家控制。
const int robot_mode_red = 1;          // robot_mode_red 表示红方由机器人控制。
const int robot_mode_black = 2;        // robot_mode_black 表示黑方由机器人控制。
const int robot_mode_both = 3;         // robot_mode_both 表示红黑双方都由机器人控制。

void setrobotmode(robotsetting& setting, int mode);
bool isrobotcolor(const robotsetting& setting, int color);
bool initrobotengine(const std::wstring& enginefolder);
void closerobotengine();
bool isrobotengineready();
std::wstring getrobotengineerror();
robotmove getbestrobotmove(const gamestate& state, int color, bool balancedmode, int movetimems);
bool startrobotsearch(const gamestate& state, int color, bool balancedmode, int movetimems);
bool pollrobotsearch(robotmove& move);
void cancelrobotsearch();
void resetrobotruntime(robotruntime& runtime);
