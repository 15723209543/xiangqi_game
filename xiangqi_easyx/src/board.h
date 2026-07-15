#pragma once

#include "piece.h"
#include <vector>

// point 保存棋盘上的一个落点。
struct point
{
    int row; // row 表示落点行号。
    int col; // col 表示落点列号。
};

// moveinfo 保存一步棋的完整信息，便于悔棋和写日志。
struct moveinfo
{
    int pieceid;     // pieceid 表示移动棋子的编号。
    int fromrow;     // fromrow 表示起点行号。
    int fromcol;     // fromcol 表示起点列号。
    int torow;       // torow 表示终点行号。
    int tocol;       // tocol 表示终点列号。
    int captureid;   // captureid 表示被吃棋子的编号，没有则为 empty_id。
    int playercolor; // playercolor 表示执行这步棋的玩家颜色。
};

// gamestate 保存整局游戏的当前状态。
struct gamestate
{
    int board[board_rows][board_cols]; // board 保存每个交叉点上的棋子编号。
    std::vector<piece> pieces;         // pieces 保存全部棋子的动态状态。
    std::vector<moveinfo> history;     // history 保存历史走法，用于悔棋。
    int currentcolor;                  // currentcolor 表示当前轮到的颜色。
    bool gameover;                     // gameover 表示游戏是否结束。
    int winnercolor;                   // winnercolor 表示获胜方颜色。
};

void initgame(gamestate& state);
int getoppositecolor(int color);
int getpieceidat(const gamestate& state, int row, int col);
bool isinsideboard(int row, int col);
bool isownpiece(const gamestate& state, int row, int col, int color);
bool iskingincheck(const gamestate& state, int color);
bool haslegalmove(const gamestate& state, int color);
std::vector<point> getlegalmoves(const gamestate& state, int pieceid);
bool movepiece(gamestate& state, int pieceid, int torow, int tocol, moveinfo* outmove);
bool undomove(gamestate& state, moveinfo* outmove);
