#pragma once

// piececolor 表示棋子的阵营，红方先手，黑方后手。
enum piececolor
{
    color_none = 0,
    color_red = 1,
    color_black = 2
};

// piecetype 表示中国象棋中的七类棋子。
enum piecetype
{
    type_none = 0,
    type_general = 1,
    type_advisor = 2,
    type_elephant = 3,
    type_horse = 4,
    type_rook = 5,
    type_cannon = 6,
    type_soldier = 7
};

// startpiece 保存一枚初始棋子的固定数据。
struct startpiece
{
    int type;  // type 表示棋子类型。
    int color; // color 表示棋子颜色。
    int row;   // row 表示棋盘行号。
    int col;   // col 表示棋盘列号。
};

const int board_rows = 10;          // board_rows 表示棋盘总行数。
const int board_cols = 9;           // board_cols 表示棋盘总列数。
const int empty_id = -1;            // empty_id 表示棋盘格没有棋子。
const int startpiece_count = 32;    // startpiece_count 表示开局棋子总数。
const int window_width = 980;       // window_width 表示 EasyX 窗口宽度。
const int window_height = 760;      // window_height 表示 EasyX 窗口高度。
const int board_left = 48;          // board_left 表示棋盘左上角横坐标。
const int board_top = 48;           // board_top 表示棋盘左上角纵坐标。
const int cell_size = 58;           // cell_size 表示棋盘相邻交叉点距离。
const int piece_radius = 24;        // piece_radius 表示棋子半径。
const int panel_left = 620;         // panel_left 表示右侧信息区左边界。

// startpieces 保存开局所有棋子的固定地图数据。
inline const startpiece startpieces[startpiece_count] =
{
    { type_rook, color_black, 0, 0 },
    { type_horse, color_black, 0, 1 },
    { type_elephant, color_black, 0, 2 },
    { type_advisor, color_black, 0, 3 },
    { type_general, color_black, 0, 4 },
    { type_advisor, color_black, 0, 5 },
    { type_elephant, color_black, 0, 6 },
    { type_horse, color_black, 0, 7 },
    { type_rook, color_black, 0, 8 },
    { type_cannon, color_black, 2, 1 },
    { type_cannon, color_black, 2, 7 },
    { type_soldier, color_black, 3, 0 },
    { type_soldier, color_black, 3, 2 },
    { type_soldier, color_black, 3, 4 },
    { type_soldier, color_black, 3, 6 },
    { type_soldier, color_black, 3, 8 },
    { type_rook, color_red, 9, 0 },
    { type_horse, color_red, 9, 1 },
    { type_elephant, color_red, 9, 2 },
    { type_advisor, color_red, 9, 3 },
    { type_general, color_red, 9, 4 },
    { type_advisor, color_red, 9, 5 },
    { type_elephant, color_red, 9, 6 },
    { type_horse, color_red, 9, 7 },
    { type_rook, color_red, 9, 8 },
    { type_cannon, color_red, 7, 1 },
    { type_cannon, color_red, 7, 7 },
    { type_soldier, color_red, 6, 0 },
    { type_soldier, color_red, 6, 2 },
    { type_soldier, color_red, 6, 4 },
    { type_soldier, color_red, 6, 6 },
    { type_soldier, color_red, 6, 8 }
};
