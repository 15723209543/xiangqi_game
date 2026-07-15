#pragma once

#include "gamedata.h"
#include <string>
#include <vector>

struct robotsetting;

// timesetting 保存开局选择的计时规则。
struct timesetting
{
    int stepseconds; // stepseconds 表示每一步最多可用秒数。
    int sideseconds; // sideseconds 表示每一方总共可用秒数。
};

// timebackup 保存一步棋之前的计时快照，用于悔棋。
struct timebackup
{
    double redremain;   // redremain 表示红方剩余总秒数。
    double blackremain; // blackremain 表示黑方剩余总秒数。
    double stepremain;  // stepremain 表示当前步剩余秒数。
    int currentcolor;   // currentcolor 表示快照时正在计时的一方。
};

// timestate 保存整局游戏的计时状态。
struct timestate
{
    int steplimit;                   // steplimit 表示每步秒数上限。
    int sidelimit;                   // sidelimit 表示单方总秒数上限。
    double redremain;                // redremain 表示红方剩余总秒数。
    double blackremain;              // blackremain 表示黑方剩余总秒数。
    double stepremain;               // stepremain 表示本步剩余秒数。
    int currentcolor;                // currentcolor 表示当前正在倒计时的一方。
    bool running;                    // running 表示计时器是否正在运行。
    unsigned long long lasttick;     // lasttick 表示上一次刷新计时的毫秒时间。
    std::vector<timebackup> history; // history 保存计时历史快照。
};

bool choosegamesetting(timesetting& setting, robotsetting& robots);
void inittimer(timestate& timer, const timesetting& setting);
int updatetimer(timestate& timer);
void switchtimer(timestate& timer, int nextcolor);
bool undotimer(timestate& timer);
void stoptimer(timestate& timer);
int getstepremainseconds(const timestate& timer);
int getredremainseconds(const timestate& timer);
int getblackremainseconds(const timestate& timer);
std::wstring formattime(int seconds);
