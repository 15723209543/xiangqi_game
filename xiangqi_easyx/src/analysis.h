#pragma once

#include "board.h"

// analysisresult 保存局势分析结果。
struct analysisresult
{
    double redscore;   // redscore 表示红方综合分。
    double blackscore; // blackscore 表示黑方综合分。
    int redpercent;    // redpercent 表示红方折算胜率百分比。
    int blackpercent;  // blackpercent 表示黑方折算胜率百分比。
};

analysisresult getanalysis(const gamestate& state);
