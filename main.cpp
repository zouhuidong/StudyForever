#include "Wnd.h"
#include "Schedule.h"

int main()
{
	Wnd wnd;
	wnd.Create();

	Schedule schedule;
	schedule.SetOrigin(1180, 20);	// 设置课程表位置（主屏幕坐标）
	
	while (true)
	{
		schedule.Redraw();
		schedule.UpdateMsg();
		Sleep(50);
	}

	return 0;
}
