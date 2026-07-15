#pragma once

#include "analysis.h"
#include "board.h"
#include "time.h"

// buttonrect 保存一个按钮的矩形区域。
struct buttonrect
{
    int left;   // left 表示按钮左边界。
    int top;    // top 表示按钮上边界。
    int right;  // right 表示按钮右边界。
    int bottom; // bottom 表示按钮下边界。
};

void drawgame(const gamestate& state, const timestate& timer, const analysisresult& analysis, int selectedid, const std::vector<point>& moves, const buttonrect& undobutton);
bool getboardrowcol(int x, int y, int& row, int& col);
bool isinbutton(int x, int y, const buttonrect& button);
