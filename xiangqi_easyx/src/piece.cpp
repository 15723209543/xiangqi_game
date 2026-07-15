#include "piece.h"

// 这个函数返回带颜色的棋子完整名称。
std::wstring getpiecename(int type, int color)
{
    std::wstring colorname = getcolorname(color); // colorname 保存颜色文字。
    return colorname + getpieceword(type, color);
}

// 这个函数返回棋子在棋盘上显示的单字。
std::wstring getpieceword(int type, int color)
{
    if (type == type_general)
    {
        return color == color_red ? L"帅" : L"将";
    }
    if (type == type_advisor)
    {
        return color == color_red ? L"仕" : L"士";
    }
    if (type == type_elephant)
    {
        return color == color_red ? L"相" : L"象";
    }
    if (type == type_horse)
    {
        return L"马";
    }
    if (type == type_rook)
    {
        return L"车";
    }
    if (type == type_cannon)
    {
        return L"炮";
    }
    if (type == type_soldier)
    {
        return color == color_red ? L"兵" : L"卒";
    }
    return L"空";
}

// 这个函数返回阵营颜色名称。
std::wstring getcolorname(int color)
{
    if (color == color_red)
    {
        return L"红";
    }
    if (color == color_black)
    {
        return L"黑";
    }
    return L"无";
}

// 这个函数返回当前玩家显示文字。
std::wstring getplayertext(int color)
{
    if (color == color_red)
    {
        return L"1号玩家（红方）";
    }
    if (color == color_black)
    {
        return L"2号玩家（黑方）";
    }
    return L"无玩家";
}
