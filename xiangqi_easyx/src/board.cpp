#include "board.h"

#include <cmath>

// 这个函数判断指定坐标是否在九宫内。
static bool ispalace(int row, int col, int color)
{
    bool colok = col >= 3 && col <= 5; // colok 表示列号是否落在九宫内。
    if (!colok)
    {
        return false;
    }
    if (color == color_red)
    {
        return row >= 7 && row <= 9;
    }
    if (color == color_black)
    {
        return row >= 0 && row <= 2;
    }
    return false;
}

// 这个函数判断象或相是否仍在自己半边棋盘。
static bool iselephantside(int row, int color)
{
    if (color == color_red)
    {
        return row >= 5 && row <= 9;
    }
    if (color == color_black)
    {
        return row >= 0 && row <= 4;
    }
    return false;
}

// 这个函数判断兵或卒是否已经过河。
static bool isrivercrossed(int row, int color)
{
    if (color == color_red)
    {
        return row <= 4;
    }
    if (color == color_black)
    {
        return row >= 5;
    }
    return false;
}

// 这个函数统计两个同线坐标之间隔着多少棋子。
static int countbetween(const gamestate& state, int fromrow, int fromcol, int torow, int tocol)
{
    int rowstep = (torow > fromrow) - (torow < fromrow); // rowstep 表示行方向步长。
    int colstep = (tocol > fromcol) - (tocol < fromcol); // colstep 表示列方向步长。
    int row = fromrow + rowstep;                         // row 表示当前扫描行号。
    int col = fromcol + colstep;                         // col 表示当前扫描列号。
    int count = 0;                                       // count 表示中间棋子数量。

    while (row != torow || col != tocol)
    {
        if (state.board[row][col] != empty_id)
        {
            ++count;
        }
        row += rowstep;
        col += colstep;
    }
    return count;
}

// 这个函数把一个目标点加入候选走法，自动排除越界和己方棋子。
static void addtarget(const gamestate& state, std::vector<point>& moves, int color, int row, int col)
{
    if (!isinsideboard(row, col))
    {
        return;
    }

    int targetid = state.board[row][col]; // targetid 表示目标点上的棋子编号。
    if (targetid == empty_id || state.pieces[targetid].color != color)
    {
        moves.push_back({ row, col });
    }
}

// 这个函数生成将或帅的基础走法。
static void addgeneralmoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    const int rowdelta[4] = { -1, 1, 0, 0 }; // rowdelta 表示上下左右的行变化。
    const int coldelta[4] = { 0, 0, -1, 1 }; // coldelta 表示上下左右的列变化。

    for (int index = 0; index < 4; ++index)
    {
        int targetrow = current.row + rowdelta[index]; // targetrow 表示目标行号。
        int targetcol = current.col + coldelta[index]; // targetcol 表示目标列号。
        if (ispalace(targetrow, targetcol, current.color))
        {
            addtarget(state, moves, current.color, targetrow, targetcol);
        }
    }

    const int scandelta[2] = { -1, 1 }; // scandelta 表示飞将扫描方向。
    for (int index = 0; index < 2; ++index)
    {
        int row = current.row + scandelta[index]; // row 表示飞将扫描行号。
        while (isinsideboard(row, current.col))
        {
            int targetid = state.board[row][current.col]; // targetid 表示扫描到的棋子编号。
            if (targetid != empty_id)
            {
                if (state.pieces[targetid].type == type_general && state.pieces[targetid].color != current.color)
                {
                    moves.push_back({ row, current.col });
                }
                break;
            }
            row += scandelta[index];
        }
    }
}

// 这个函数生成士或仕的基础走法。
static void addadvisormoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    const int rowdelta[4] = { -1, -1, 1, 1 }; // rowdelta 表示斜走的行变化。
    const int coldelta[4] = { -1, 1, -1, 1 }; // coldelta 表示斜走的列变化。

    for (int index = 0; index < 4; ++index)
    {
        int targetrow = current.row + rowdelta[index]; // targetrow 表示目标行号。
        int targetcol = current.col + coldelta[index]; // targetcol 表示目标列号。
        if (ispalace(targetrow, targetcol, current.color))
        {
            addtarget(state, moves, current.color, targetrow, targetcol);
        }
    }
}

// 这个函数生成象或相的基础走法，并处理象眼阻挡。
static void addelephantmoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    const int rowdelta[4] = { -2, -2, 2, 2 }; // rowdelta 表示象走田的行变化。
    const int coldelta[4] = { -2, 2, -2, 2 }; // coldelta 表示象走田的列变化。

    for (int index = 0; index < 4; ++index)
    {
        int targetrow = current.row + rowdelta[index];      // targetrow 表示目标行号。
        int targetcol = current.col + coldelta[index];      // targetcol 表示目标列号。
        int eyerow = current.row + rowdelta[index] / 2;     // eyerow 表示象眼行号。
        int eyecol = current.col + coldelta[index] / 2;     // eyecol 表示象眼列号。

        if (isinsideboard(targetrow, targetcol) &&
            iselephantside(targetrow, current.color) &&
            state.board[eyerow][eyecol] == empty_id)
        {
            addtarget(state, moves, current.color, targetrow, targetcol);
        }
    }
}

// 这个函数生成马的基础走法，并处理蹩马腿。
static void addhorsemoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    const int rowdelta[8] = { -2, -2, -1, 1, 2, 2, 1, -1 }; // rowdelta 表示马步行变化。
    const int coldelta[8] = { -1, 1, 2, 2, 1, -1, -2, -2 }; // coldelta 表示马步列变化。

    for (int index = 0; index < 8; ++index)
    {
        int targetrow = current.row + rowdelta[index]; // targetrow 表示目标行号。
        int targetcol = current.col + coldelta[index]; // targetcol 表示目标列号。
        int legrow = current.row;                      // legrow 表示马腿行号。
        int legcol = current.col;                      // legcol 表示马腿列号。

        if (std::abs(rowdelta[index]) == 2)
        {
            legrow = current.row + rowdelta[index] / 2;
        }
        else
        {
            legcol = current.col + coldelta[index] / 2;
        }

        if (isinsideboard(targetrow, targetcol) && state.board[legrow][legcol] == empty_id)
        {
            addtarget(state, moves, current.color, targetrow, targetcol);
        }
    }
}

// 这个函数生成车的基础走法。
static void addrookmoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    const int rowdelta[4] = { -1, 1, 0, 0 }; // rowdelta 表示直线扫描行变化。
    const int coldelta[4] = { 0, 0, -1, 1 }; // coldelta 表示直线扫描列变化。

    for (int index = 0; index < 4; ++index)
    {
        int row = current.row + rowdelta[index]; // row 表示当前扫描行号。
        int col = current.col + coldelta[index]; // col 表示当前扫描列号。
        while (isinsideboard(row, col))
        {
            int targetid = state.board[row][col]; // targetid 表示扫描到的棋子编号。
            if (targetid == empty_id)
            {
                moves.push_back({ row, col });
            }
            else
            {
                if (state.pieces[targetid].color != current.color)
                {
                    moves.push_back({ row, col });
                }
                break;
            }
            row += rowdelta[index];
            col += coldelta[index];
        }
    }
}

// 这个函数生成炮的基础走法，并处理隔山打炮。
static void addcannonmoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    const int rowdelta[4] = { -1, 1, 0, 0 }; // rowdelta 表示直线扫描行变化。
    const int coldelta[4] = { 0, 0, -1, 1 }; // coldelta 表示直线扫描列变化。

    for (int index = 0; index < 4; ++index)
    {
        int row = current.row + rowdelta[index]; // row 表示当前扫描行号。
        int col = current.col + coldelta[index]; // col 表示当前扫描列号。
        bool hasjumped = false;                  // hasjumped 表示炮是否已经遇到炮架。

        while (isinsideboard(row, col))
        {
            int targetid = state.board[row][col]; // targetid 表示扫描到的棋子编号。
            if (!hasjumped)
            {
                if (targetid == empty_id)
                {
                    moves.push_back({ row, col });
                }
                else
                {
                    hasjumped = true;
                }
            }
            else
            {
                if (targetid != empty_id)
                {
                    if (state.pieces[targetid].color != current.color)
                    {
                        moves.push_back({ row, col });
                    }
                    break;
                }
            }
            row += rowdelta[index];
            col += coldelta[index];
        }
    }
}

// 这个函数生成兵或卒的基础走法。
static void addsoldiermoves(const gamestate& state, const piece& current, std::vector<point>& moves)
{
    int forward = current.color == color_red ? -1 : 1; // forward 表示前进方向。
    addtarget(state, moves, current.color, current.row + forward, current.col);

    if (isrivercrossed(current.row, current.color))
    {
        addtarget(state, moves, current.color, current.row, current.col - 1);
        addtarget(state, moves, current.color, current.row, current.col + 1);
    }
}

// 这个函数生成不考虑送将的基础走法。
static std::vector<point> getpseudomoves(const gamestate& state, int pieceid)
{
    std::vector<point> moves; // moves 保存候选落点。
    if (pieceid < 0 || pieceid >= static_cast<int>(state.pieces.size()))
    {
        return moves;
    }

    const piece& current = state.pieces[pieceid]; // current 表示当前要计算的棋子。
    if (!current.alive)
    {
        return moves;
    }

    if (current.type == type_general)
    {
        addgeneralmoves(state, current, moves);
    }
    else if (current.type == type_advisor)
    {
        addadvisormoves(state, current, moves);
    }
    else if (current.type == type_elephant)
    {
        addelephantmoves(state, current, moves);
    }
    else if (current.type == type_horse)
    {
        addhorsemoves(state, current, moves);
    }
    else if (current.type == type_rook)
    {
        addrookmoves(state, current, moves);
    }
    else if (current.type == type_cannon)
    {
        addcannonmoves(state, current, moves);
    }
    else if (current.type == type_soldier)
    {
        addsoldiermoves(state, current, moves);
    }

    return moves;
}

// 这个函数只修改棋盘状态，不切换玩家，用于正式走棋和模拟走棋。
static void applymoveonly(gamestate& state, const moveinfo& info)
{
    piece& moving = state.pieces[info.pieceid]; // moving 表示正在移动的棋子。
    state.board[info.fromrow][info.fromcol] = empty_id;

    if (info.captureid != empty_id)
    {
        piece& captured = state.pieces[info.captureid]; // captured 表示被吃掉的棋子。
        captured.alive = false;
        captured.row = -1;
        captured.col = -1;
    }

    moving.row = info.torow;
    moving.col = info.tocol;
    state.board[info.torow][info.tocol] = info.pieceid;
}

// 这个函数回退一步棋盘状态，用于悔棋。
static void revertmoveonly(gamestate& state, const moveinfo& info)
{
    piece& moving = state.pieces[info.pieceid]; // moving 表示需要退回的棋子。
    state.board[info.torow][info.tocol] = empty_id;
    moving.row = info.fromrow;
    moving.col = info.fromcol;
    state.board[info.fromrow][info.fromcol] = info.pieceid;

    if (info.captureid != empty_id)
    {
        piece& captured = state.pieces[info.captureid]; // captured 表示需要恢复的棋子。
        captured.alive = true;
        captured.row = info.torow;
        captured.col = info.tocol;
        state.board[info.torow][info.tocol] = info.captureid;
    }
}

// 这个函数判断某枚棋子是否正在攻击指定位置。
static bool pieceattackssquare(const gamestate& state, const piece& attacker, int targetrow, int targetcol)
{
    int rowdiff = targetrow - attacker.row; // rowdiff 表示目标与攻击者的行差。
    int coldiff = targetcol - attacker.col; // coldiff 表示目标与攻击者的列差。
    int absrow = std::abs(rowdiff);         // absrow 表示行差绝对值。
    int abscol = std::abs(coldiff);         // abscol 表示列差绝对值。

    if (attacker.type == type_general)
    {
        if (attacker.col == targetcol && countbetween(state, attacker.row, attacker.col, targetrow, targetcol) == 0)
        {
            return true;
        }
        return absrow + abscol == 1 && ispalace(targetrow, targetcol, attacker.color);
    }

    if (attacker.type == type_advisor)
    {
        return absrow == 1 && abscol == 1 && ispalace(targetrow, targetcol, attacker.color);
    }

    if (attacker.type == type_elephant)
    {
        int eyerow = attacker.row + rowdiff / 2; // eyerow 表示象眼行号。
        int eyecol = attacker.col + coldiff / 2; // eyecol 表示象眼列号。
        return absrow == 2 &&
            abscol == 2 &&
            iselephantside(targetrow, attacker.color) &&
            state.board[eyerow][eyecol] == empty_id;
    }

    if (attacker.type == type_horse)
    {
        int legrow = attacker.row; // legrow 表示马腿行号。
        int legcol = attacker.col; // legcol 表示马腿列号。
        if (!((absrow == 2 && abscol == 1) || (absrow == 1 && abscol == 2)))
        {
            return false;
        }
        if (absrow == 2)
        {
            legrow = attacker.row + rowdiff / 2;
        }
        else
        {
            legcol = attacker.col + coldiff / 2;
        }
        return state.board[legrow][legcol] == empty_id;
    }

    if (attacker.type == type_rook)
    {
        return (attacker.row == targetrow || attacker.col == targetcol) &&
            countbetween(state, attacker.row, attacker.col, targetrow, targetcol) == 0;
    }

    if (attacker.type == type_cannon)
    {
        return (attacker.row == targetrow || attacker.col == targetcol) &&
            countbetween(state, attacker.row, attacker.col, targetrow, targetcol) == 1;
    }

    if (attacker.type == type_soldier)
    {
        int forward = attacker.color == color_red ? -1 : 1; // forward 表示兵卒前进方向。
        if (targetrow == attacker.row + forward && targetcol == attacker.col)
        {
            return true;
        }
        return isrivercrossed(attacker.row, attacker.color) &&
            targetrow == attacker.row &&
            abscol == 1;
    }

    return false;
}

// 这个函数初始化一局新游戏。
void initgame(gamestate& state)
{
    for (int row = 0; row < board_rows; ++row)
    {
        for (int col = 0; col < board_cols; ++col)
        {
            state.board[row][col] = empty_id;
        }
    }

    state.pieces.clear();
    state.history.clear();
    state.currentcolor = color_red;
    state.gameover = false;
    state.winnercolor = color_none;

    for (int index = 0; index < startpiece_count; ++index)
    {
        startpiece data = startpieces[index]; // data 表示当前初始棋子固定数据。
        piece item;                           // item 表示要加入状态的棋子。
        item.id = index;
        item.type = data.type;
        item.color = data.color;
        item.row = data.row;
        item.col = data.col;
        item.alive = true;
        state.pieces.push_back(item);
        state.board[item.row][item.col] = item.id;
    }
}

// 这个函数返回另一方颜色。
int getoppositecolor(int color)
{
    if (color == color_red)
    {
        return color_black;
    }
    if (color == color_black)
    {
        return color_red;
    }
    return color_none;
}

// 这个函数读取某个交叉点上的棋子编号。
int getpieceidat(const gamestate& state, int row, int col)
{
    if (!isinsideboard(row, col))
    {
        return empty_id;
    }
    return state.board[row][col];
}

// 这个函数判断坐标是否在棋盘范围内。
bool isinsideboard(int row, int col)
{
    return row >= 0 && row < board_rows && col >= 0 && col < board_cols;
}

// 这个函数判断指定坐标是否是当前颜色自己的棋子。
bool isownpiece(const gamestate& state, int row, int col, int color)
{
    int pieceid = getpieceidat(state, row, col); // pieceid 表示坐标上的棋子编号。
    return pieceid != empty_id && state.pieces[pieceid].alive && state.pieces[pieceid].color == color;
}

// 这个函数判断某一方的将帅是否正被攻击。
bool iskingincheck(const gamestate& state, int color)
{
    int kingrow = -1; // kingrow 表示本方将帅行号。
    int kingcol = -1; // kingcol 表示本方将帅列号。

    for (const piece& item : state.pieces)
    {
        if (item.alive && item.color == color && item.type == type_general)
        {
            kingrow = item.row;
            kingcol = item.col;
            break;
        }
    }

    if (kingrow == -1)
    {
        return true;
    }

    for (const piece& attacker : state.pieces)
    {
        if (attacker.alive && attacker.color != color && pieceattackssquare(state, attacker, kingrow, kingcol))
        {
            return true;
        }
    }

    return false;
}

// 这个函数判断某一方是否至少还有一步合法棋。
bool haslegalmove(const gamestate& state, int color)
{
    for (const piece& item : state.pieces)
    {
        if (item.alive && item.color == color)
        {
            std::vector<point> moves = getlegalmoves(state, item.id); // moves 保存该棋子的合法落点。
            if (!moves.empty())
            {
                return true;
            }
        }
    }
    return false;
}

// 这个函数返回一枚棋子所有不送将的合法落点。
std::vector<point> getlegalmoves(const gamestate& state, int pieceid)
{
    std::vector<point> legalmoves; // legalmoves 保存最终合法落点。
    if (state.gameover || pieceid < 0 || pieceid >= static_cast<int>(state.pieces.size()))
    {
        return legalmoves;
    }

    const piece& current = state.pieces[pieceid]; // current 表示当前要计算的棋子。
    if (!current.alive)
    {
        return legalmoves;
    }

    int color = current.color;                         // color 表示当前棋子颜色。
    std::vector<point> pseudomoves = getpseudomoves(state, pieceid); // pseudomoves 保存基础落点。

    for (const point& target : pseudomoves)
    {
        gamestate nextstate = state; // nextstate 表示模拟走棋后的状态。
        int captureid = nextstate.board[target.row][target.col]; // captureid 表示模拟被吃棋子。
        moveinfo info;              // info 表示模拟走法。
        info.pieceid = pieceid;
        info.fromrow = current.row;
        info.fromcol = current.col;
        info.torow = target.row;
        info.tocol = target.col;
        info.captureid = captureid;
        info.playercolor = color;
        applymoveonly(nextstate, info);
        if (!iskingincheck(nextstate, color))
        {
            legalmoves.push_back(target);
        }
    }

    return legalmoves;
}

// 这个函数执行一次真实走棋。
bool movepiece(gamestate& state, int pieceid, int torow, int tocol, moveinfo* outmove)
{
    if (state.gameover || pieceid < 0 || pieceid >= static_cast<int>(state.pieces.size()))
    {
        return false;
    }

    piece& current = state.pieces[pieceid]; // current 表示当前要移动的棋子。
    if (!current.alive || current.color != state.currentcolor)
    {
        return false;
    }

    std::vector<point> legalmoves = getlegalmoves(state, pieceid); // legalmoves 保存合法落点。
    bool allowed = false;                                         // allowed 表示目标是否允许落子。
    for (const point& target : legalmoves)
    {
        if (target.row == torow && target.col == tocol)
        {
            allowed = true;
            break;
        }
    }
    if (!allowed)
    {
        return false;
    }

    int captureid = state.board[torow][tocol]; // captureid 表示被吃棋子编号。
    moveinfo info;                             // info 表示本次真实走法。
    info.pieceid = pieceid;
    info.fromrow = current.row;
    info.fromcol = current.col;
    info.torow = torow;
    info.tocol = tocol;
    info.captureid = captureid;
    info.playercolor = state.currentcolor;

    int nextcolor = getoppositecolor(state.currentcolor); // nextcolor 表示下一手颜色。
    bool capturedking = captureid != empty_id && state.pieces[captureid].type == type_general; // capturedking 表示是否吃到将帅。

    applymoveonly(state, info);
    state.history.push_back(info);
    state.currentcolor = nextcolor;

    if (capturedking || !haslegalmove(state, nextcolor))
    {
        state.gameover = true;
        state.winnercolor = info.playercolor;
    }

    if (outmove != nullptr)
    {
        *outmove = info;
    }
    return true;
}

// 这个函数悔棋一步并恢复当前玩家。
bool undomove(gamestate& state, moveinfo* outmove)
{
    if (state.history.empty())
    {
        return false;
    }

    moveinfo info = state.history.back(); // info 表示需要撤回的最后一步棋。
    state.history.pop_back();
    state.gameover = false;
    state.winnercolor = color_none;
    state.currentcolor = info.playercolor;
    revertmoveonly(state, info);

    if (outmove != nullptr)
    {
        *outmove = info;
    }
    return true;
}
