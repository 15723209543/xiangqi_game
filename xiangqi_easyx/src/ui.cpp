#include "ui.h"

#include <graphics.h>
#include <cmath>
#include <sstream>

// 这个函数把棋盘列号转换为窗口横坐标。
static int getx(int col)
{
    return board_left + col * cell_size;
}

// 这个函数把棋盘行号转换为窗口纵坐标。
static int gety(int row)
{
    return board_top + row * cell_size;
}

// 这个函数在矩形内居中绘制文字。
static void drawcentertext(int left, int top, int right, int bottom, const std::wstring& text, int color, int height)
{
    int width = right - left;             // width 表示矩形宽度。
    int heightbox = bottom - top;         // heightbox 表示矩形高度。
    setbkmode(TRANSPARENT);
    settextcolor(color);
    settextstyle(height, 0, L"Microsoft YaHei");
    int textx = left + (width - textwidth(text.c_str())) / 2;      // textx 表示文字横坐标。
    int texty = top + (heightbox - textheight(text.c_str())) / 2;  // texty 表示文字纵坐标。
    outtextxy(textx, texty, text.c_str());
}

// 这个函数绘制棋盘线、九宫和楚河汉界。
static void drawboardlines()
{
    setlinecolor(RGB(64, 49, 36));
    setlinestyle(PS_SOLID, 2);

    for (int row = 0; row < board_rows; ++row)
    {
        line(getx(0), gety(row), getx(board_cols - 1), gety(row));
    }

    for (int col = 0; col < board_cols; ++col)
    {
        if (col == 0 || col == board_cols - 1)
        {
            line(getx(col), gety(0), getx(col), gety(board_rows - 1));
        }
        else
        {
            line(getx(col), gety(0), getx(col), gety(4));
            line(getx(col), gety(5), getx(col), gety(board_rows - 1));
        }
    }

    line(getx(3), gety(0), getx(5), gety(2));
    line(getx(5), gety(0), getx(3), gety(2));
    line(getx(3), gety(7), getx(5), gety(9));
    line(getx(5), gety(7), getx(3), gety(9));

    drawcentertext(getx(1), gety(4), getx(3), gety(5), L"楚河", RGB(83, 90, 69), 30);
    drawcentertext(getx(5), gety(4), getx(7), gety(5), L"汉界", RGB(83, 90, 69), 30);
}

// 这个函数绘制棋盘上的一枚棋子。
static void drawpiececircle(const piece& item, bool selected)
{
    int x = getx(item.col);                       // x 表示棋子圆心横坐标。
    int y = gety(item.row);                       // y 表示棋子圆心纵坐标。
    int linecolor = item.color == color_red ? RGB(174, 38, 38) : RGB(30, 30, 30); // linecolor 表示棋子描边颜色。
    int textcolor = item.color == color_red ? RGB(174, 38, 38) : RGB(15, 15, 15); // textcolor 表示棋子文字颜色。

    if (selected)
    {
        setlinecolor(RGB(36, 130, 94));
        setlinestyle(PS_SOLID, 4);
        circle(x, y, piece_radius + 6);
    }

    setfillcolor(RGB(252, 247, 231));
    setlinecolor(linecolor);
    setlinestyle(PS_SOLID, 3);
    solidcircle(x, y, piece_radius);
    circle(x, y, piece_radius);

    setbkmode(TRANSPARENT);
    settextcolor(textcolor);
    settextstyle(30, 0, L"SimSun");
    std::wstring word = getpieceword(item.type, item.color); // word 表示棋子显示字。
    outtextxy(x - textwidth(word.c_str()) / 2, y - textheight(word.c_str()) / 2, word.c_str());
}

// 这个函数绘制所有仍然存活的棋子。
static void drawboardpieces(const gamestate& state, int selectedid)
{
    for (const piece& item : state.pieces)
    {
        if (item.alive)
        {
            drawpiececircle(item, item.id == selectedid);
        }
    }
}

// 这个函数绘制选中棋子的可落子位置。
static void drawmoves(const gamestate& state, const std::vector<point>& moves)
{
    for (const point& target : moves)
    {
        int x = getx(target.col);      // x 表示目标点横坐标。
        int y = gety(target.row);      // y 表示目标点纵坐标。
        int targetid = state.board[target.row][target.col]; // targetid 表示目标点棋子编号。

        if (targetid == empty_id)
        {
            setfillcolor(RGB(42, 151, 105));
            setlinecolor(RGB(42, 151, 105));
            solidcircle(x, y, 7);
        }
        else
        {
            setlinecolor(RGB(42, 151, 105));
            setlinestyle(PS_SOLID, 4);
            circle(x, y, piece_radius + 8);
        }
    }
}

// 这个函数绘制右侧信息区中的小棋子。
static void drawsmallpiece(const piece& item, int x, int y)
{
    int linecolor = item.color == color_red ? RGB(174, 38, 38) : RGB(30, 30, 30); // linecolor 表示小棋子线色。
    int textcolor = item.color == color_red ? RGB(174, 38, 38) : RGB(15, 15, 15); // textcolor 表示小棋子文字颜色。

    setfillcolor(RGB(253, 252, 246));
    setlinecolor(linecolor);
    setlinestyle(PS_SOLID, 2);
    solidcircle(x, y, 13);
    circle(x, y, 13);

    setbkmode(TRANSPARENT);
    settextcolor(textcolor);
    settextstyle(18, 0, L"SimSun");
    std::wstring word = getpieceword(item.type, item.color); // word 表示小棋子显示字。
    outtextxy(x - textwidth(word.c_str()) / 2, y - textheight(word.c_str()) / 2, word.c_str());

    if (!item.alive)
    {
        setlinecolor(RGB(196, 45, 45));
        setlinestyle(PS_SOLID, 3);
        line(x - 17, y - 17, x + 17, y + 17);
        line(x + 17, y - 17, x - 17, y + 17);
    }
}

// 这个函数绘制一条从进攻方指向被将军方的水平箭头。
static void drawcheckarrow(int fromx, int tox, int y, int color)
{
    int headsize = 12; // headsize 表示箭头头部大小。
    setlinecolor(color);
    setlinestyle(PS_SOLID, 5);
    line(fromx, y, tox, y);

    if (tox > fromx)
    {
        line(tox, y, tox - headsize, y - headsize);
        line(tox, y, tox - headsize, y + headsize);
    }
    else
    {
        line(tox, y, tox + headsize, y - headsize);
        line(tox, y, tox + headsize, y + headsize);
    }
}

// 这个函数在右侧两列棋子中间绘制将军提示和方向箭头。
static void drawcheckwarning(const gamestate& state)
{
    bool redcheck = !state.gameover && iskingincheck(state, color_red);     // redcheck 表示红方是否被将军。
    bool blackcheck = !state.gameover && iskingincheck(state, color_black); // blackcheck 表示黑方是否被将军。
    if (!redcheck && !blackcheck)
    {
        return;
    }

    int centerx = panel_left + 150;      // centerx 表示提示文字中心横坐标。
    int texttop = 318;                   // texttop 表示将军文字上边界。
    int arrowy = 390;                    // arrowy 表示箭头纵坐标。
    int redsidex = panel_left + 100;     // redsidex 表示靠近红方列表的箭头端点。
    int blacksidex = panel_left + 200;   // blacksidex 表示靠近黑方列表的箭头端点。
    int warncolor = RGB(190, 38, 38);    // warncolor 表示将军警示颜色。

    setfillcolor(RGB(244, 247, 241));
    solidrectangle(panel_left + 104, texttop - 8, panel_left + 196, arrowy + 50);

    setbkmode(TRANSPARENT);
    settextcolor(warncolor);
    settextstyle(42, 0, L"Microsoft YaHei");
    outtextxy(centerx - textwidth(L"将军") / 2, texttop, L"将军");

    if (redcheck)
    {
        drawcheckarrow(blacksidex, redsidex, arrowy, warncolor);
        drawcentertext(panel_left + 98, arrowy + 13, panel_left + 202, arrowy + 40, L"黑方→红方", warncolor, 16);
    }
    else if (blackcheck)
    {
        drawcheckarrow(redsidex, blacksidex, arrowy, warncolor);
        drawcentertext(panel_left + 98, arrowy + 13, panel_left + 202, arrowy + 40, L"红方→黑方", warncolor, 16);
    }
}

// 这个函数绘制右侧计时信息。
static void drawtimerinfo(const timestate& timer)
{
    std::wstring steptext = L"本步剩余：" + formattime(getstepremainseconds(timer)); // steptext 表示本步倒计时文字。
    int stepcolor = getstepremainseconds(timer) <= 10 ? RGB(190, 38, 38) : RGB(38, 45, 42); // stepcolor 表示本步时间文字颜色。
    drawcentertext(panel_left + 18, 94, window_width - 18, 122, steptext, stepcolor, 20);

    std::wstring redtime = L"总时长：" + formattime(getredremainseconds(timer)); // redtime 表示红方总剩余时间。
    std::wstring blacktime = L"总时长：" + formattime(getblackremainseconds(timer)); // blacktime 表示黑方总剩余时间。
    drawcentertext(panel_left + 28, 194, panel_left + 150, 218, redtime, RGB(174, 38, 38), 16);
    drawcentertext(panel_left + 190, 194, panel_left + 312, 218, blacktime, RGB(20, 20, 20), 16);
}

// 这个函数绘制右侧玩家、按钮和双方棋子列表。
static void drawpanel(const gamestate& state, const timestate& timer, const buttonrect& undobutton)
{
    setfillcolor(RGB(244, 247, 241));
    solidrectangle(panel_left, 0, window_width, window_height);
    setlinecolor(RGB(165, 170, 156));
    line(panel_left, 0, panel_left, window_height);

    drawcentertext(panel_left, 20, window_width, 58, L"中国象棋", RGB(34, 56, 48), 30);

    std::wstring playertext = L"当前是" + getplayertext(state.currentcolor); // playertext 表示当前玩家说明。
    if (state.gameover)
    {
        playertext = L"游戏结束：" + getplayertext(state.winnercolor) + L"获胜";
    }
    drawcentertext(panel_left + 18, 65, window_width - 18, 96, playertext, RGB(38, 45, 42), 20);
    drawtimerinfo(timer);

    setfillcolor(state.history.empty() ? RGB(216, 220, 211) : RGB(54, 119, 91));
    setlinecolor(RGB(42, 88, 70));
    setlinestyle(PS_SOLID, 2);
    solidrectangle(undobutton.left, undobutton.top, undobutton.right, undobutton.bottom);
    rectangle(undobutton.left, undobutton.top, undobutton.right, undobutton.bottom);
    drawcentertext(undobutton.left, undobutton.top, undobutton.right, undobutton.bottom, L"撤回上一步", RGB(255, 255, 255), 20);

    drawcentertext(panel_left + 28, 168, panel_left + 150, 194, L"红方棋子", RGB(174, 38, 38), 20);
    drawcentertext(panel_left + 190, 168, panel_left + 312, 194, L"黑方棋子", RGB(20, 20, 20), 20);

    int redindex = 0;   // redindex 表示红方列表序号。
    int blackindex = 0; // blackindex 表示黑方列表序号。
    for (const piece& item : state.pieces)
    {
        if (item.color == color_red)
        {
            int y = 242 + redindex * 26; // y 表示红方小棋子纵坐标。
            drawsmallpiece(item, panel_left + 70, y);
            ++redindex;
        }
        else if (item.color == color_black)
        {
            int y = 242 + blackindex * 26; // y 表示黑方小棋子纵坐标。
            drawsmallpiece(item, panel_left + 230, y);
            ++blackindex;
        }
    }

    drawcheckwarning(state);
    drawcentertext(panel_left + 18, 646, window_width - 18, 674, L"点击棋子后绿色位置可落子", RGB(83, 90, 69), 16);
}

// 这个函数绘制底部局势分析进度条。
static void drawanalysisbar(const analysisresult& analysis)
{
    int top = window_height - 78;         // top 表示分析区上边界。
    int barleft = 70;                     // barleft 表示进度条左边界。
    int barright = window_width - 70;     // barright 表示进度条右边界。
    int bartop = top + 34;                // bartop 表示进度条上边界。
    int barbottom = top + 58;             // barbottom 表示进度条下边界。
    int barwidth = barright - barleft;    // barwidth 表示进度条宽度。
    int redwidth = barwidth * analysis.redpercent / 100; // redwidth 表示红方进度条宽度。

    setfillcolor(RGB(233, 236, 225));
    solidrectangle(0, top, window_width, window_height);
    setlinecolor(RGB(165, 170, 156));
    line(0, top, window_width, top);

    std::wstring title = L"局势分析  红方 " + std::to_wstring(analysis.redpercent) + L"%  -  黑方 " + std::to_wstring(analysis.blackpercent) + L"%"; // title 表示局势分析标题。
    drawcentertext(0, top + 4, window_width, top + 31, title, RGB(45, 55, 49), 20);

    setfillcolor(RGB(174, 38, 38));
    solidrectangle(barleft, bartop, barleft + redwidth, barbottom);
    setfillcolor(RGB(35, 35, 35));
    solidrectangle(barleft + redwidth, bartop, barright, barbottom);
    setlinecolor(RGB(75, 82, 70));
    setlinestyle(PS_SOLID, 2);
    rectangle(barleft, bartop, barright, barbottom);
    line(barleft + redwidth, bartop, barleft + redwidth, barbottom);
}

// 这个函数绘制完整游戏界面。
void drawgame(const gamestate& state, const timestate& timer, const analysisresult& analysis, int selectedid, const std::vector<point>& moves, const buttonrect& undobutton)
{
    BeginBatchDraw();
    setbkcolor(RGB(233, 236, 225));
    cleardevice();

    setfillcolor(RGB(230, 202, 145));
    solidrectangle(20, 20, panel_left - 28, window_height - 28);
    setlinecolor(RGB(98, 73, 45));
    setlinestyle(PS_SOLID, 3);
    rectangle(20, 20, panel_left - 28, window_height - 28);

    drawboardlines();
    drawboardpieces(state, selectedid);
    drawmoves(state, moves);
    drawpanel(state, timer, undobutton);
    drawanalysisbar(analysis);
    EndBatchDraw();
}

// 这个函数把鼠标坐标转换为棋盘行列。
bool getboardrowcol(int x, int y, int& row, int& col)
{
    double rawcol = static_cast<double>(x - board_left) / cell_size; // rawcol 表示未取整列号。
    double rawrow = static_cast<double>(y - board_top) / cell_size;  // rawrow 表示未取整行号。
    col = static_cast<int>(std::floor(rawcol + 0.5));
    row = static_cast<int>(std::floor(rawrow + 0.5));

    if (!isinsideboard(row, col))
    {
        return false;
    }

    int centerx = getx(col); // centerx 表示最近交叉点横坐标。
    int centery = gety(row); // centery 表示最近交叉点纵坐标。
    int dx = x - centerx;    // dx 表示鼠标到交叉点的横向距离。
    int dy = y - centery;    // dy 表示鼠标到交叉点的纵向距离。
    int limit = 32;          // limit 表示可点击容错半径。
    return dx * dx + dy * dy <= limit * limit;
}

// 这个函数判断鼠标坐标是否落在按钮内。
bool isinbutton(int x, int y, const buttonrect& button)
{
    return x >= button.left && x <= button.right && y >= button.top && y <= button.bottom;
}
