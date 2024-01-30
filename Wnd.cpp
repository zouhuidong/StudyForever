#include "Wnd.h"
#include "HiEasyX.h"

ScreenSize g_sizeScreen = GetScreenSize();

HWND Wnd::Create()
{
	hiex::PreSetWindowPos(g_sizeScreen.left, g_sizeScreen.top);
	hiex::PreSetWindowStyle(WS_POPUP);
	hiex::PreSetWindowStyleEx(WS_EX_TOPMOST);
	HWND hWnd = initgraph(g_sizeScreen.w, g_sizeScreen.h);

	// ���ڳߴ籣���߳�
	std::thread([](HWND hWnd) {
		while (hiex::IsAliveWindow(hWnd))
		{
			SetWindowPos(hWnd, HWND_NOTOPMOST, g_sizeScreen.left, g_sizeScreen.top, g_sizeScreen.w, g_sizeScreen.h, 0);
			Sleep(100);
		}
		},
		hWnd).detach();

	// ��ֹ��������
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

	// Ԥ���������
	BEGIN_TASK();
	{
		setbkcolor(BLACK);
		cleardevice();

		setorigin(-g_sizeScreen.left, -g_sizeScreen.top);
	}
	END_TASK();

	return hWnd;
}
