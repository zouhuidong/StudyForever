#pragma once
#include <easyx.h>

class Schedule
{
private:

	IMAGE m_imgCanvas;
	int m_nOriginX = 0;
	int m_nOriginY = 0;

	void Paint();

	void OnKeyboard();
	void OnMouse(int x, int y);

	int JudgeCursorPosition(int x, int y);
	void ScheduleButtonClick(int id);
	void ReportButtonClick();
	void SaveHistory();

public:

	Schedule();

	void SetOrigin(int x, int y);	// 设置绘制原点
	void Redraw();		// 重绘
	void UpdateMsg();	// 响应用户消息

};

