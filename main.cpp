#include "Wnd.h"
#include "Schedule.h"

int main()
{
	Wnd wnd;
	wnd.Create();

	Schedule schedule;
	schedule.SetOrigin(1180, 20);	// ���ÿγ̱�λ�ã�����Ļ���꣩
	
	while (true)
	{
		schedule.Redraw();
		schedule.UpdateMsg();
		Sleep(50);
	}

	return 0;
}
