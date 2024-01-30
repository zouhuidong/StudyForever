#include "Wnd.h"
#include "Schedule.h"
#include "ini.hpp"

int main()
{
	Wnd wnd;
	wnd.Create();

	// ��ȡ����
	int x = GetIniFileInfoInt(L"./settings.ini", L"Position", L"x", 100);
	int y = GetIniFileInfoInt(L"./settings.ini", L"Position", L"y", 100);

	Schedule schedule;
	schedule.SetOrigin(x, y);	// ���ÿγ̱�λ�ã�����Ļ���꣩
	
	while (true)
	{
		schedule.Redraw();
		schedule.UpdateMsg();
		Sleep(50);
	}

	return 0;
}
