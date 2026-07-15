#pragma once

#include "gamedata.h"
#include <string>

// piece 保存一枚棋子的动态状态。
struct piece
{
    int id;     // id 表示棋子的唯一编号。
    int type;   // type 表示棋子类型。
    int color;  // color 表示棋子颜色。
    int row;    // row 表示棋子当前行号。
    int col;    // col 表示棋子当前列号。
    bool alive; // alive 表示棋子是否仍在棋盘上。
};

std::wstring getpiecename(int type, int color);
std::wstring getpieceword(int type, int color);
std::wstring getcolorname(int color);
std::wstring getplayertext(int color);
