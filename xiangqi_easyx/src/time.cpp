#include "time.h"
#include "robot.h"

#include <graphics.h>
#include <algorithm>
#include <cmath>
#include <windows.h>

const int option_step = 0;  // option_step 表示每步时长选项。
const int option_side = 1;  // option_side 表示单方总时长选项。
const int option_robot = 2; // option_robot 表示机器人数量选项。

// optionbutton 保存开局配置页中的一个按钮。
struct optionbutton
{
    int left;   // left 表示按钮左边界。
    int top;    // top 表示按钮上边界。
    int right;  // right 表示按钮右边界。
    int bottom; // bottom 表示按钮下边界。
    int value;  // value 表示按钮对应的数值。
    int mode;   // mode 表示按钮所属的配置类型。
};

// 这个函数返回当前系统毫秒时间。
static unsigned long long gettick()
{
    return static_cast<unsigned long long>(GetTickCount());
}

// 这个函数判断鼠标坐标是否落在开局配置按钮内。
static bool inoptionbutton(int x, int y, const optionbutton& button)
{
    return x >= button.left && x <= button.right && y >= button.top && y <= button.bottom;
}

// 这个函数在矩形内按文字实际宽高精确居中绘制文字。
static void drawsettingtext(int left, int top, int right, int bottom, const std::wstring& text, int color, int height)
{
    setbkmode(TRANSPARENT);
    settextcolor(color);
    settextstyle(height, 0, L"Microsoft YaHei");
    int boxwidth = right - left + 1;                       // boxwidth 表示文字区域的完整宽度。
    int boxheight = bottom - top + 1;                      // boxheight 表示文字区域的完整高度。
    int textx = left + (boxwidth - textwidth(text.c_str())) / 2;   // textx 表示居中后的文字横坐标。
    int texty = top + (boxheight - textheight(text.c_str())) / 2;  // texty 表示居中后的文字纵坐标。
    outtextxy(textx, texty, text.c_str());
}

// 这个函数返回开局配置按钮上的文字。
static std::wstring getoptiontext(const optionbutton& button)
{
    if (button.mode == option_side)
    {
        return std::to_wstring(button.value / 60) + L" 分钟";
    }
    if (button.mode == option_robot)
    {
        if (button.value == robot_mode_red)
        {
            return L"红方机器人";
        }
        if (button.value == robot_mode_black)
        {
            return L"黑方机器人";
        }
        if (button.value == robot_mode_both)
        {
            return L"双方机器人";
        }
        return L"双方玩家";
    }
    return std::to_wstring(button.value) + L" 秒";
}

// 这个函数判断开局配置按钮是否处于选中状态。
static bool isoptionselected(const optionbutton& button, const timesetting& setting, const robotsetting& robots)
{
    if (button.mode == option_side)
    {
        return setting.sideseconds == button.value;
    }
    if (button.mode == option_robot)
    {
        return robots.mode == button.value;
    }
    return setting.stepseconds == button.value;
}

// 这个函数绘制一个开局配置选项按钮，并标出键盘焦点。
static void drawoptionbutton(const optionbutton& button, bool selected, bool focused)
{
    int fillcolor = selected ? RGB(54, 119, 91) : RGB(245, 247, 240); // fillcolor 表示按钮填充颜色。
    int linecolor = selected ? RGB(34, 86, 65) : RGB(146, 154, 137);  // linecolor 表示按钮边框颜色。
    int textcolor = selected ? RGB(255, 255, 255) : RGB(45, 55, 49);  // textcolor 表示按钮文字颜色。

    setfillcolor(fillcolor);
    setlinecolor(linecolor);
    setlinestyle(PS_SOLID, 2);
    solidrectangle(button.left, button.top, button.right, button.bottom);
    rectangle(button.left, button.top, button.right, button.bottom);
    if (focused)
    {
        setlinecolor(RGB(174, 38, 38));
        setlinestyle(PS_SOLID, 3);
        rectangle(button.left - 5, button.top - 5, button.right + 5, button.bottom + 5);
    }
    int textheight = button.mode == option_robot ? 20 : 22; // textheight 表示适应完整对战模式名称的文字高度。
    drawsettingtext(button.left, button.top, button.right, button.bottom, getoptiontext(button), textcolor, textheight);
}

// 这个函数根据对战模式返回双方控制方式说明。
static std::wstring getrobotdescription(int mode)
{
    if (mode == robot_mode_red)
    {
        return L"对战模式：红方机器人  VS  黑方玩家";
    }
    if (mode == robot_mode_black)
    {
        return L"对战模式：红方玩家  VS  黑方机器人";
    }
    if (mode == robot_mode_both)
    {
        return L"对战模式：红方机器人  VS  黑方机器人";
    }
    return L"对战模式：红方玩家  VS  黑方玩家";
}

// 这个函数绘制包含三组选项的统一开局配置页。
static void drawgamesetting(const timesetting& setting, const robotsetting& robots, const std::vector<optionbutton>& buttons, const optionbutton& startbutton, int activegroup)
{
    BeginBatchDraw();
    setbkcolor(RGB(233, 236, 225));
    cleardevice();

    drawsettingtext(0, 24, window_width - 1, 76, L"欢迎来到中国象棋游戏", RGB(34, 56, 48), 36);
    drawsettingtext(0, 82, window_width - 1, 108, L"请选择本局游戏配置", RGB(83, 90, 69), 18);
    int normalcolor = RGB(45, 55, 49); // normalcolor 表示未获得键盘焦点的选项组标题颜色。
    int focuscolor = RGB(174, 38, 38); // focuscolor 表示当前键盘焦点选项组的标题颜色。
    drawsettingtext(0, 116, window_width - 1, 144, L"每步时长", activegroup == option_step ? focuscolor : normalcolor, 22);
    drawsettingtext(0, 220, window_width - 1, 248, L"单方总时长", activegroup == option_side ? focuscolor : normalcolor, 22);
    drawsettingtext(0, 324, window_width - 1, 352, L"对战模式", activegroup == option_robot ? focuscolor : normalcolor, 22);

    for (const optionbutton& button : buttons)
    {
        bool selected = isoptionselected(button, setting, robots); // selected 表示当前按钮是否被选中。
        bool focused = selected && button.mode == activegroup;     // focused 表示当前按钮是否同时是键盘焦点。
        drawoptionbutton(button, selected, focused);
    }

    drawsettingtext(0, 416, window_width - 1, 444, getrobotdescription(robots.mode), RGB(83, 90, 69), 18);

    setfillcolor(RGB(174, 38, 38));
    setlinecolor(RGB(130, 25, 25));
    setlinestyle(PS_SOLID, 2);
    solidrectangle(startbutton.left, startbutton.top, startbutton.right, startbutton.bottom);
    rectangle(startbutton.left, startbutton.top, startbutton.right, startbutton.bottom);
    drawsettingtext(startbutton.left, startbutton.top, startbutton.right, startbutton.bottom, L"开始游戏", RGB(255, 255, 255), 24);

    drawsettingtext(0, window_height - 62, window_width - 1, window_height - 34, L"选择完成后点击开始游戏，按 ESC 可退出", RGB(83, 90, 69), 18);
    EndBatchDraw();
}

// 这个函数计算一组按钮整体居中后的起始横坐标。
static int getgroupstartx(int count, int buttonwidth, int gap)
{
    int groupwidth = count * buttonwidth + (count - 1) * gap; // groupwidth 表示整组按钮的总宽度。
    return (window_width - groupwidth) / 2;
}

// 这个函数向配置按钮列表中添加一组居中按钮。
static void addoptiongroup(std::vector<optionbutton>& buttons, const int values[], int count, int mode, int top, int buttonwidth, int buttonheight, int gap)
{
    int startx = getgroupstartx(count, buttonwidth, gap); // startx 表示当前按钮组居中后的起始横坐标。
    for (int index = 0; index < count; ++index)
    {
        optionbutton button; // button 表示正在创建的一个配置按钮。
        button.left = startx + index * (buttonwidth + gap);
        button.top = top;
        button.right = button.left + buttonwidth;
        button.bottom = button.top + buttonheight;
        button.value = values[index];
        button.mode = mode;
        buttons.push_back(button);
    }
}

// 这个函数在一组有序选项中按方向返回调整后的数值。
static int getadjustedvalue(const int values[], int count, int currentvalue, int direction)
{
    int currentindex = 0; // currentindex 表示当前数值在有序选项中的索引。
    for (int index = 0; index < count; ++index)
    {
        if (values[index] == currentvalue)
        {
            currentindex = index;
            break;
        }
    }
    int nextindex = currentindex + direction; // nextindex 表示按左右键调整后的目标索引。
    if (nextindex < 0)
    {
        nextindex = 0;
    }
    if (nextindex >= count)
    {
        nextindex = count - 1;
    }
    return values[nextindex];
}

// 这个函数按左右方向键调小或调大当前焦点选项。
static void adjustactiveoption(timesetting& setting, robotsetting& robots, int activegroup, int direction)
{
    int stepvalues[3] = { 30, 60, 90 }; // stepvalues 保存按从小到大排列的每步时长选项。
    int sidevalues[4] = { 5 * 60, 10 * 60, 15 * 60, 30 * 60 }; // sidevalues 保存按从小到大排列的单方总时长选项。
    int robotvalues[4] = { robot_mode_players, robot_mode_red, robot_mode_black, robot_mode_both }; // robotvalues 保存按界面顺序排列的四种对战模式。
    if (activegroup == option_step)
    {
        setting.stepseconds = getadjustedvalue(stepvalues, 3, setting.stepseconds, direction);
    }
    else if (activegroup == option_side)
    {
        setting.sideseconds = getadjustedvalue(sidevalues, 4, setting.sideseconds, direction);
    }
    else if (activegroup == option_robot)
    {
        setrobotmode(robots, getadjustedvalue(robotvalues, 4, robots.mode, direction));
    }
}

// 这个函数让玩家在同一页选择每步时长、单方总时长和对战模式。
bool choosegamesetting(timesetting& setting, robotsetting& robots)
{
    setting.stepseconds = 60;
    setting.sideseconds = 10 * 60;
    setrobotmode(robots, robot_mode_players);

    std::vector<optionbutton> buttons; // buttons 保存三组开局配置按钮。
    int buttonwidth = 132;             // buttonwidth 表示选项按钮的统一宽度。
    int buttonheight = 48;             // buttonheight 表示选项按钮的统一高度。
    int stepvalues[3] = { 30, 60, 90 }; // stepvalues 保存每步时长选项。
    int sidevalues[4] = { 5 * 60, 10 * 60, 15 * 60, 30 * 60 }; // sidevalues 保存单方总时长选项。
    int robotvalues[4] = { robot_mode_players, robot_mode_red, robot_mode_black, robot_mode_both }; // robotvalues 保存四种对战模式选项。
    addoptiongroup(buttons, stepvalues, 3, option_step, 152, buttonwidth, buttonheight, 24);
    addoptiongroup(buttons, sidevalues, 4, option_side, 256, buttonwidth, buttonheight, 20);
    addoptiongroup(buttons, robotvalues, 4, option_robot, 360, 184, buttonheight, 16);

    optionbutton startbutton; // startbutton 表示开始游戏按钮。
    int startwidth = 240;      // startwidth 表示开始游戏按钮的宽度。
    startbutton.left = (window_width - startwidth) / 2;
    startbutton.top = 478;
    startbutton.right = startbutton.left + startwidth;
    startbutton.bottom = 538;
    startbutton.value = 0;
    startbutton.mode = option_step;

    int activegroup = option_step; // activegroup 表示当前由上下方向键选中的配置组。
    drawgamesetting(setting, robots, buttons, startbutton, activegroup);

    while (true)
    {
        ExMessage message; // message 表示 EasyX 输入消息。
        getmessage(&message, EM_MOUSE | EM_KEY);

        if (message.message == WM_LBUTTONDOWN)
        {
            bool handled = false; // handled 表示本次点击是否命中选项。
            for (const optionbutton& button : buttons)
            {
                if (inoptionbutton(message.x, message.y, button))
                {
                    activegroup = button.mode;
                    if (button.mode == option_side)
                    {
                        setting.sideseconds = button.value;
                    }
                    else if (button.mode == option_robot)
                    {
                        setrobotmode(robots, button.value);
                    }
                    else
                    {
                        setting.stepseconds = button.value;
                    }
                    handled = true;
                    break;
                }
            }

            if (inoptionbutton(message.x, message.y, startbutton))
            {
                return true;
            }

            if (handled)
            {
                drawgamesetting(setting, robots, buttons, startbutton, activegroup);
            }
        }
        else if (message.message == WM_KEYDOWN)
        {
            if (message.vkcode == VK_ESCAPE)
            {
                return false;
            }
            if (message.vkcode == VK_RETURN)
            {
                return true;
            }

            bool handled = false; // handled 表示本次按键是否完成了配置页导航或数值调整。
            if (message.vkcode == VK_UP)
            {
                activegroup = activegroup > option_step ? activegroup - 1 : option_step;
                handled = true;
            }
            else if (message.vkcode == VK_DOWN)
            {
                activegroup = activegroup < option_robot ? activegroup + 1 : option_robot;
                handled = true;
            }
            else if (message.vkcode == VK_LEFT)
            {
                adjustactiveoption(setting, robots, activegroup, -1);
                handled = true;
            }
            else if (message.vkcode == VK_RIGHT)
            {
                adjustactiveoption(setting, robots, activegroup, 1);
                handled = true;
            }

            if (handled)
            {
                drawgamesetting(setting, robots, buttons, startbutton, activegroup);
            }
        }
    }
}

// 这个函数初始化整局游戏计时器。
void inittimer(timestate& timer, const timesetting& setting)
{
    timer.steplimit = setting.stepseconds;
    timer.sidelimit = setting.sideseconds;
    timer.redremain = static_cast<double>(setting.sideseconds);
    timer.blackremain = static_cast<double>(setting.sideseconds);
    timer.stepremain = static_cast<double>(setting.stepseconds);
    timer.currentcolor = color_red;
    timer.running = true;
    timer.lasttick = gettick();
    timer.history.clear();
}

// 这个函数根据真实经过时间刷新倒计时，返回超时方颜色。
int updatetimer(timestate& timer)
{
    if (!timer.running)
    {
        return color_none;
    }

    unsigned long long now = gettick(); // now 表示当前毫秒时间。
    double elapsed = static_cast<double>(now - timer.lasttick) / 1000.0; // elapsed 表示距离上次刷新的秒数。
    if (elapsed <= 0.0)
    {
        return color_none;
    }

    timer.lasttick = now;
    timer.stepremain -= elapsed;
    if (timer.currentcolor == color_red)
    {
        timer.redremain -= elapsed;
    }
    else if (timer.currentcolor == color_black)
    {
        timer.blackremain -= elapsed;
    }

    if (timer.stepremain <= 0.0)
    {
        timer.stepremain = 0.0;
        timer.running = false;
        return timer.currentcolor;
    }

    if (timer.currentcolor == color_red && timer.redremain <= 0.0)
    {
        timer.redremain = 0.0;
        timer.running = false;
        return color_red;
    }

    if (timer.currentcolor == color_black && timer.blackremain <= 0.0)
    {
        timer.blackremain = 0.0;
        timer.running = false;
        return color_black;
    }

    return color_none;
}

// 这个函数在成功走棋后切换到下一方计时。
void switchtimer(timestate& timer, int nextcolor)
{
    timebackup backup; // backup 表示走棋前的计时快照。
    backup.redremain = timer.redremain;
    backup.blackremain = timer.blackremain;
    backup.stepremain = timer.stepremain;
    backup.currentcolor = timer.currentcolor;
    timer.history.push_back(backup);

    timer.currentcolor = nextcolor;
    timer.stepremain = static_cast<double>(timer.steplimit);
    timer.running = true;
    timer.lasttick = gettick();
}

// 这个函数在悔棋后恢复上一步之前的计时状态。
bool undotimer(timestate& timer)
{
    if (timer.history.empty())
    {
        return false;
    }

    timebackup backup = timer.history.back(); // backup 表示要恢复的计时快照。
    timer.history.pop_back();
    timer.redremain = backup.redremain;
    timer.blackremain = backup.blackremain;
    timer.stepremain = backup.stepremain;
    timer.currentcolor = backup.currentcolor;
    timer.running = true;
    timer.lasttick = gettick();
    return true;
}

// 这个函数停止计时器。
void stoptimer(timestate& timer)
{
    timer.running = false;
}

// 这个函数返回本步剩余整数秒。
int getstepremainseconds(const timestate& timer)
{
    int seconds = static_cast<int>(std::ceil(timer.stepremain)); // seconds 表示取整后的剩余秒数。
    return seconds < 0 ? 0 : seconds;
}

// 这个函数返回红方剩余整数秒。
int getredremainseconds(const timestate& timer)
{
    int seconds = static_cast<int>(std::ceil(timer.redremain)); // seconds 表示取整后的红方剩余秒数。
    return seconds < 0 ? 0 : seconds;
}

// 这个函数返回黑方剩余整数秒。
int getblackremainseconds(const timestate& timer)
{
    int seconds = static_cast<int>(std::ceil(timer.blackremain)); // seconds 表示取整后的黑方剩余秒数。
    return seconds < 0 ? 0 : seconds;
}

// 这个函数把秒数格式化为 mm:ss 文本。
std::wstring formattime(int seconds)
{
    int safevalue = seconds < 0 ? 0 : seconds; // safevalue 表示不小于零的秒数。
    int minutes = safevalue / 60;         // minutes 表示分钟数。
    int remainseconds = safevalue % 60;   // remainseconds 表示不足一分钟的秒数。
    std::wstring text = std::to_wstring(minutes) + L":"; // text 表示格式化后的时间文本。
    if (remainseconds < 10)
    {
        text += L"0";
    }
    text += std::to_wstring(remainseconds);
    return text;
}
