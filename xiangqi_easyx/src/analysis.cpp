#include "analysis.h"
#include "analysis_data.h"

#include <algorithm>
#include <cmath>
#include <vector>

// 这个函数判断指定坐标是否在九宫内。
static bool analysisispalace(int row, int col, int color)
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

// 这个函数判断象或相是否仍在本方半边。
static bool analysisiselephantside(int row, int color)
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

// 这个函数判断棋子是否已经越过楚河汉界。
static bool analysisiscrossriver(int row, int color)
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
static int analysiscountbetween(const gamestate& state, int fromrow, int fromcol, int torow, int tocol)
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

// 这个函数判断某枚棋子是否攻击指定棋盘点。
static bool analysisattackssquare(const gamestate& state, const piece& attacker, int targetrow, int targetcol)
{
    if (!attacker.alive || !isinsideboard(targetrow, targetcol))
    {
        return false;
    }

    int rowdiff = targetrow - attacker.row; // rowdiff 表示目标与攻击者的行差。
    int coldiff = targetcol - attacker.col; // coldiff 表示目标与攻击者的列差。
    int absrow = std::abs(rowdiff);         // absrow 表示行差绝对值。
    int abscol = std::abs(coldiff);         // abscol 表示列差绝对值。

    if (attacker.type == type_general)
    {
        if (attacker.col == targetcol && analysiscountbetween(state, attacker.row, attacker.col, targetrow, targetcol) == 0)
        {
            return true;
        }
        return absrow + abscol == 1 && analysisispalace(targetrow, targetcol, attacker.color);
    }

    if (attacker.type == type_advisor)
    {
        return absrow == 1 && abscol == 1 && analysisispalace(targetrow, targetcol, attacker.color);
    }

    if (attacker.type == type_elephant)
    {
        int eyerow = attacker.row + rowdiff / 2; // eyerow 表示象眼行号。
        int eyecol = attacker.col + coldiff / 2; // eyecol 表示象眼列号。
        return absrow == 2 &&
            abscol == 2 &&
            analysisiselephantside(targetrow, attacker.color) &&
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
            analysiscountbetween(state, attacker.row, attacker.col, targetrow, targetcol) == 0;
    }

    if (attacker.type == type_cannon)
    {
        return (attacker.row == targetrow || attacker.col == targetcol) &&
            analysiscountbetween(state, attacker.row, attacker.col, targetrow, targetcol) == 1;
    }

    if (attacker.type == type_soldier)
    {
        int forward = attacker.color == color_red ? -1 : 1; // forward 表示兵卒前进方向。
        if (targetrow == attacker.row + forward && targetcol == attacker.col)
        {
            return true;
        }
        return analysisiscrossriver(attacker.row, attacker.color) && targetrow == attacker.row && abscol == 1;
    }

    return false;
}

// 这个函数返回指定颜色将帅所在棋子。
static const piece* findking(const gamestate& state, int color)
{
    for (const piece& item : state.pieces)
    {
        if (item.alive && item.color == color && item.type == type_general)
        {
            return &item;
        }
    }
    return nullptr;
}

// 这个函数判断某棋子是否被指定颜色攻击。
static bool ispieceattackedbycolor(const gamestate& state, const piece& current, int attackcolor)
{
    for (const piece& attacker : state.pieces)
    {
        if (attacker.alive && attacker.color == attackcolor && analysisattackssquare(state, attacker, current.row, current.col))
        {
            return true;
        }
    }
    return false;
}

// 这个函数判断某棋子是否被己方保护。
static bool isprotectedbyfriend(const gamestate& state, const piece& current)
{
    for (const piece& helper : state.pieces)
    {
        if (helper.alive && helper.id != current.id && helper.color == current.color && analysisattackssquare(state, helper, current.row, current.col))
        {
            return true;
        }
    }
    return false;
}

// 这个函数判断某棋子是否与己方棋子形成配合。
static bool hasfriendlycooperation(const gamestate& state, const piece& current)
{
    for (const piece& helper : state.pieces)
    {
        if (!helper.alive || helper.id == current.id || helper.color != current.color)
        {
            continue;
        }

        int rowdistance = std::abs(helper.row - current.row); // rowdistance 表示两枚己方棋子的行距。
        int coldistance = std::abs(helper.col - current.col); // coldistance 表示两枚己方棋子的列距。
        int manhattan = rowdistance + coldistance;            // manhattan 表示两枚己方棋子的曼哈顿距离。
        if (manhattan <= 2)
        {
            return true;
        }

        if ((helper.row == current.row || helper.col == current.col) && manhattan <= 4)
        {
            int between = analysiscountbetween(state, helper.row, helper.col, current.row, current.col); // between 表示两枚棋子之间的阻隔数。
            if (between <= 1)
            {
                return true;
            }
        }
    }
    return false;
}

// 这个函数判断某棋子是否威胁对方将帅。
static bool threatensenemyking(const gamestate& state, const piece& current)
{
    const piece* enemyking = findking(state, getoppositecolor(current.color)); // enemyking 表示对方将帅。
    if (enemyking == nullptr)
    {
        return false;
    }
    return analysisattackssquare(state, current, enemyking->row, enemyking->col);
}

// 这个函数计算一枚棋子的局势分。
static double getpiecescore(const gamestate& state, const piece& current)
{
    if (!current.alive || current.type == type_general)
    {
        return 0.0;
    }

    double score = getanalysisbasevalue(current.type); // score 表示当前棋子的分析分。

    if (analysisiscrossriver(current.row, current.color))
    {
        score += analysis_cross_bonus;
    }

    if (hasfriendlycooperation(state, current))
    {
        score += analysis_cooperate_bonus;
    }

    if (isprotectedbyfriend(state, current))
    {
        score += analysis_protect_bonus;
    }

    if (threatensenemyking(state, current))
    {
        score += analysis_check_bonus;
    }

    std::vector<point> moves = getlegalmoves(state, current.id); // moves 表示当前棋子的合法落点。
    if (moves.empty() && ispieceattackedbycolor(state, current, getoppositecolor(current.color)))
    {
        score -= analysis_trapped_penalty;
    }

    return std::max(0.0, score);
}

// 这个函数返回当前棋局的综合局势分析。
analysisresult getanalysis(const gamestate& state)
{
    analysisresult result; // result 表示最终分析结果。
    result.redscore = analysis_start_score;
    result.blackscore = analysis_start_score;
    result.redpercent = 50;
    result.blackpercent = 50;

    for (const piece& current : state.pieces)
    {
        double score = getpiecescore(state, current); // score 表示当前棋子的最终分。
        if (current.color == color_red)
        {
            result.redscore += score;
        }
        else if (current.color == color_black)
        {
            result.blackscore += score;
        }
    }

    double scorediff = result.redscore - result.blackscore; // scorediff 表示红黑双方分差。
    result.redpercent = static_cast<int>(std::round(50.0 + scorediff * analysis_percent_scale));
    result.redpercent = std::clamp(result.redpercent, 0, 100);
    result.blackpercent = 100 - result.redpercent;

    return result;
}
