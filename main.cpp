#include "Wnd.h"
#include "Schedule.h"
#include "ini.hpp"

int main()
{
	Wnd wnd;
	wnd.Create();

	// 读取配置
	int x = GetIniFileInfoInt(L"./settings.ini", L"Position", L"x", 100);
	int y = GetIniFileInfoInt(L"./settings.ini", L"Position", L"y", 100);

	Schedule schedule;
	schedule.SetOrigin(x, y);	// 设置课程表位置（主屏幕坐标）
	
	while (true)
	{
		schedule.Redraw();
		schedule.UpdateMsg();
		Sleep(50);
	}

	return 0;
}
