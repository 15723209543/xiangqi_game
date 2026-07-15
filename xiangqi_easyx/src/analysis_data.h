#pragma once

#include "gamedata.h"

const double analysis_start_score = 100.0;       // analysis_start_score 表示双方开局基础分。
const double analysis_cross_bonus = 10.0;        // analysis_cross_bonus 表示过河加分，影响最小。
const double analysis_protect_bonus = 18.0;      // analysis_protect_bonus 表示被己方保护加分。
const double analysis_cooperate_bonus = 30.0;    // analysis_cooperate_bonus 表示己方配合加分。
const double analysis_trapped_penalty = 55.0;    // analysis_trapped_penalty 表示受威胁且无法移动扣分。
const double analysis_check_bonus = 140.0;       // analysis_check_bonus 表示威胁对方王加分，影响最大。
const double analysis_percent_scale = 0.12;      // analysis_percent_scale 表示分差折算胜率时的敏感系数。

// 这个函数返回棋子的基础分析分。
inline double getanalysisbasevalue(int type)
{
    if (type == type_rook)
    {
        return 100.0;
    }
    if (type == type_cannon)
    {
        return 70.0;
    }
    if (type == type_horse)
    {
        return 65.0;
    }
    if (type == type_elephant)
    {
        return 35.0;
    }
    if (type == type_advisor)
    {
        return 35.0;
    }
    if (type == type_soldier)
    {
        return 20.0;
    }
    return 0.0;
}
