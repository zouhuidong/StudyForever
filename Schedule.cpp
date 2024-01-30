#include "Schedule.h"
#include "HiEasyX.h"

// ȫ��ʱ��
SYSTEMTIME stSystime;

// ����һ��Ľ���
bool bDayEnd = false;

// С����
IMAGE imgFlag;

// ���¿���Ч
hiex::MusicMCI musicBeginClass;
hiex::MusicMCI musicEndClass;

// ʱ��㣬{-1,-1} ��ʾ����һ��ʱ�����ͬ
struct TimePoint
{
	int h;
	int m;
};

enum LessonType
{
	Class,
	Break
};

struct Lesson
{
	TimePoint begin, end;
	LessonType type;
	LPCTSTR note;
	int report_sum = 0;	// �򱨸����
};

// ��ǰ�γ���Ϣ
struct CurrentLessonInfo
{
	int nLessonID = -1;	// �״����г���ʱ���� -1 ��� ID
	int nTotalMins = 0;
	int nMinsGone = 0;
	int nMinsRemaining = 0;
	double dRatioGone = 0;

}stCurrentLesson;

const int nLessonSum = 29;	// �γ�����

// �γ̱���д�棩
Lesson p_LessonSchedule_Simplified[nLessonSum] =
{
	{{6,55}, {7,45}, Class, L"����ϰ"},
	{{-1,-1}, {7,55}, Break},
	{{-1,-1}, {8,35}, Class, L"���硤��һ�ڿ�"},
	{{-1,-1}, {8,45}, Break},
	{{-1,-1}, {9,25}, Class, L"���硤�ڶ��ڿ�"},
	{{-1,-1}, {9,35}, Break},
	{{-1,-1}, {10,15}, Class, L"���硤�����ڿ�"},
	{{-1,-1}, {10,30}, Break, L"��μ�"},
	{{-1,-1}, {11,10}, Class, L"���硤���Ľڿ�"},
	{{-1,-1}, {11,20}, Break},
	{{-1,-1}, {12,00}, Class, L"���硤����ڿ�"},
	{{-1,-1}, {12,30}, Break, L"�緹��Ϣ"},
	{{-1,-1}, {13,10}, Class, L"����ϰ"},
	{{-1,-1}, {14,20}, Break, L"����"},
	{{-1,-1}, {15,00}, Class, L"���硤��һ�ڿ�"},
	{{-1,-1}, {15,10}, Break},
	{{-1,-1}, {15,50}, Class, L"���硤�ڶ��ڿ�"},
	{{-1,-1}, {16,00}, Break},
	{{-1,-1}, {16,40}, Class, L"���硤�����ڿ�"},
	{{-1,-1}, {16,50}, Break},
	{{-1,-1}, {17,30}, Class, L"���硤���Ľڿ�"},
	{{-1,-1}, {18,20}, Break, L"�������ɻ"},
	{{-1,-1}, {19,00}, Class, L"��ǰ����ϰ"},
	{{-1,-1}, {19,20}, Break, L"��μ�"},
	{{-1,-1}, {20,20}, Class, L"����һ"},
	{{-1,-1}, {20,30}, Break},
	{{-1,-1}, {21,10}, Class, L"���޶�"},
	{{-1,-1}, {21,20}, Break},
	{{-1,-1}, {22,00}, Class, L"������"},
};

// �γ̱�ȫ����ת��Ϊ����ʱ�䣬���� {-1,-1}��
Lesson p_LessonSchedule[nLessonSum];

// �γ̱��Ӧ�Ļ�������
RECT p_rctLessonSchedule[nLessonSum];

enum ButtonState
{
	Unselected,
	Good,
	Normal,
	Bad,
	Others
};

// �γ̱���Ŀ��Ӧ�İ�ť
struct Button
{
	bool show;	// �Ƿ���ʾ
	RECT rct;
	ButtonState state = Unselected;
};

Button p_btnLessonSchedule[nLessonSum];
RECT rct_btnReport;	// �򱨸水ť

// ��������
struct tagLessonTableDrawingSetting
{
	// ���������Ͻ�λ��
	POINT pProgess = { 50,10 };

	// ������Ͻ�λ��
	POINT pTable = { 50,50 };

	double d_len_pre_min = 0.8;	// �γ���Ŀ���ȣ�ÿ���ӣ�
	int n_width = 500;			// �γ���Ŀ���
	int n_progess_height = 20;	// �������߶�

	int n_ball_interval = 10;
	int n_ball_radius = 5;

	// �򱨸水ť
	POINT pReport = { pProgess.x + n_width + 10,pProgess.y };

	// �γ̱�ť
	int n_btn_interval = 10;	// ��ť�Ϳγ���Ŀ�ļ��
	int n_btn_width = 30;		// ��ť���

	// ��������ɫ
	COLORREF color_Progress = GREEN;

	COLORREF color_FloatBall = WHITE;

	// �γ���Ŀ��ɫ
	COLORREF color_Lesson = ORANGE;
	COLORREF color_Lesson_Text = BLACK;
	COLORREF color_Break = BLUE;
	COLORREF color_Break_Text = WHITE;

	// �γ̰�ť��ɫ
	COLORREF color_Btn_Good = GREEN;
	COLORREF color_Btn_Normal = RGB(150, 170, 0);
	COLORREF color_Btn_Bad = RED;
	COLORREF color_Btn_Others = DARKGRAY;
	COLORREF color_Btn_Unselected = RGB(230, 230, 230);

}DrawingSetting;

// ��ȡ����ʱ���֮���ʱ�������ӣ�
int GetTimeLen_Min(TimePoint begin, TimePoint end)
{
	return (end.h * 60 + end.m) - (begin.h * 60 + begin.m);
}

// �ж�ĳʱ����Ƿ���ĳʱ����ڣ��������յ�ʱ�̣�
bool IsInTimePeriod(TimePoint target, TimePoint begin, TimePoint end)
{
	int len_target = GetTimeLen_Min(begin, target);
	int len_total = GetTimeLen_Min(begin, end);
	return len_target >= 0 && len_target < len_total;
}

// ���¿γ̱��ʱ��Ϊ��ʵʱ�䣨�޸� {-1,-1} Ϊʵ��ʱ�䣩
void UpdateRealTime()
{
	memcpy_s(p_LessonSchedule, sizeof(Lesson) * nLessonSum,
		p_LessonSchedule_Simplified, sizeof(Lesson) * nLessonSum);

	for (int i = 0; i < nLessonSum; i++)
	{
		if (p_LessonSchedule[i].begin.h == -1)
		{
			if (i >= 1)
			{
				p_LessonSchedule[i].begin = p_LessonSchedule[i - 1].end;
			}
		}
	}
}

// ��ʼ�������Ϣ
// ������ UpdateCurrentInfo
void InitInfo()
{
	// ��� RECT ������Ϣ
	// ��ʼλ��
	int x = DrawingSetting.pTable.x;
	int y = DrawingSetting.pTable.y;

	// �����ǳ�ʼλ�Ƶ� y �����¼��
	int n_current_y = 0;
	for (int i = 0; i < nLessonSum; i++)
	{
		Lesson lesson = p_LessonSchedule[i];
		int mins = GetTimeLen_Min(lesson.begin, lesson.end);

		// ������Ŀ�߶�
		int height = (int)(mins * DrawingSetting.d_len_pre_min);

		// ���γ̱���Ŀ�ľ�������
		p_rctLessonSchedule[i].left = x;
		p_rctLessonSchedule[i].right = x + DrawingSetting.n_width;
		p_rctLessonSchedule[i].top = y + n_current_y;
		p_rctLessonSchedule[i].bottom = y + n_current_y + height;

		// ��䰴ť��Ϣ

		// �ڴ˳�ʼ���׶Σ��Ѿ��ٵ��Ŀγ̰�ť���Ϊ Others ״̬������γ̰�ť������ʾ��
		if (lesson.type == Class && (bDayEnd || stCurrentLesson.nLessonID > i))
		{
			p_btnLessonSchedule[i].show = true;
			p_btnLessonSchedule[i].state = Others;
		}
		else
		{
			p_btnLessonSchedule[i].show = false;
			p_btnLessonSchedule[i].state = Unselected;
		}
		p_btnLessonSchedule[i].rct.left = p_rctLessonSchedule[i].right + DrawingSetting.n_btn_interval;
		p_btnLessonSchedule[i].rct.right = p_btnLessonSchedule[i].rct.left + DrawingSetting.n_btn_width;
		p_btnLessonSchedule[i].rct.top = p_rctLessonSchedule[i].top;
		p_btnLessonSchedule[i].rct.bottom = p_rctLessonSchedule[i].bottom;

		n_current_y += height;
	}

	// �򱨸水ť
	rct_btnReport = {
		DrawingSetting.pReport.x,
		DrawingSetting.pReport.y,
		DrawingSetting.pReport.x + DrawingSetting.n_btn_width,
		DrawingSetting.pReport.y + DrawingSetting.n_progess_height,
	};
}

// ������Դ
void LoadRes()
{
	loadimage(&imgFlag, L"./res/flag.bmp");
	musicBeginClass.open(L"./res/begin_class.mp3");
	musicEndClass.open(L"./res/end_class.mp3");
}

// ����ȫ��ʱ�䣨�� UpdateCurrentInfo �лᱻ���ã�
void UpdateTime()
{
	GetLocalTime(&stSystime);
}

// �������¿�����
void PlayClassSound(bool begin)
{
	if (begin)
	{
		musicBeginClass.setStartTime(0);
		musicBeginClass.play();
	}
	else
	{
		musicEndClass.setStartTime(0);
		musicEndClass.play();
	}
}

// ʱ�䵽�ˣ��л��γ�
// ��Ϊһ��Ľ�����end_of_day��������� id ��Ч���ɴ��� 0
void SwitchClass(int id, bool end_of_day)
{
	if (!end_of_day)
	{
		// ��������
		PlayClassSound(p_LessonSchedule[id].type == Class);

		// ����ʹ�ð�ť
		if (id - 1 >= 0 && p_LessonSchedule[id - 1].type == Class)
		{
			p_btnLessonSchedule[id - 1].show = true;
		}
	}
	else
	{
		// ��������
		PlayClassSound(false);

		// ����ʹ�ð�ť
		p_btnLessonSchedule[nLessonSum - 1].show = true;
	}
}

// ���µ�ǰ�γ���Ϣ��ʱ����Ϣ
void UpdateCurrentLessonInfo()
{
	UpdateTime();

	TimePoint pCurrentTime = { stSystime.wHour,stSystime.wMinute };

	// ƥ�����ڿγ�
	bool bMatched = false;	// �Ƿ�ƥ��ɹ�
	for (int i = 0; i < nLessonSum; i++)
	{
		Lesson lesson = p_LessonSchedule[i];
		if (IsInTimePeriod(pCurrentTime, lesson.begin, lesson.end))
		{
			// ���¿�
			if (/*stCurrentLesson.nLessonID != -1 &&*/ stCurrentLesson.nLessonID != i)
			{
				SwitchClass(i, false);
			}

			stCurrentLesson.nLessonID = i;
			stCurrentLesson.nTotalMins = GetTimeLen_Min(lesson.begin, lesson.end);
			stCurrentLesson.nMinsGone = GetTimeLen_Min(lesson.begin, pCurrentTime);
			stCurrentLesson.nMinsRemaining = GetTimeLen_Min(pCurrentTime, lesson.end);
			stCurrentLesson.dRatioGone = (double)stCurrentLesson.nMinsGone / stCurrentLesson.nTotalMins;

			bMatched = true;
			break;
		}
	}

	// ����ʱ�䷶Χ������
	if (!bMatched)
	{
		// �Ͽ�ǰ
		if (pCurrentTime.h <= p_LessonSchedule[0].begin.h)
		{
			stCurrentLesson.nLessonID = 0;
			stCurrentLesson.dRatioGone = 0;
		}
		else
		{
			// ����һ��Ľ���
			if (!bDayEnd)
			{
				SwitchClass(0, true);
			}

			Lesson lesson = p_LessonSchedule[nLessonSum - 1];
			stCurrentLesson.nLessonID = nLessonSum - 1;
			stCurrentLesson.nTotalMins = GetTimeLen_Min(lesson.begin, lesson.end);
			stCurrentLesson.nMinsGone = stCurrentLesson.nTotalMins;
			stCurrentLesson.nMinsRemaining = 0;
			stCurrentLesson.dRatioGone = 1;
			
			bDayEnd = true;
		}
	}
}

void PaintProgress()
{
	// ���
	setfillcolor(DrawingSetting.color_Progress);
	solidrectangle(
		DrawingSetting.pProgess.x,
		DrawingSetting.pProgess.y,
		DrawingSetting.pProgess.x + (int)(DrawingSetting.n_width * stCurrentLesson.dRatioGone),
		DrawingSetting.pProgess.y + DrawingSetting.n_progess_height
	);

	// ���
	rectangle(
		DrawingSetting.pProgess.x,
		DrawingSetting.pProgess.y,
		DrawingSetting.pProgess.x + DrawingSetting.n_width,
		DrawingSetting.pProgess.y + DrawingSetting.n_progess_height
	);

	// ����
	wchar_t buf[128] = {};
	wsprintf(buf, L"%ls�� %d ���ӡ�( %d'%d\"%dms / %d )",
		(p_LessonSchedule[stCurrentLesson.nLessonID].type == Class) ? L"���ڿ�" : L"��Ϣʱ��",
		stCurrentLesson.nTotalMins,
		stCurrentLesson.nMinsGone,
		stSystime.wSecond,
		stSystime.wMilliseconds,
		stCurrentLesson.nMinsRemaining);
	outtextxy(DrawingSetting.pProgess.x + 4, DrawingSetting.pProgess.y + 3, buf);
}

void PaintReportBtn()
{
	rectangle_RECT(rct_btnReport);
	drawtext(L"����", &rct_btnReport, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ����С�͵Ĵ򱨸�ͼ��
void PaintReportIcon(int x, int y)
{
	putimage(x, y, &imgFlag);
}

void PaintSchedule()
{
	for (int i = 0; i < nLessonSum; i++)
	{
		Lesson lesson = p_LessonSchedule[i];

		// ���������ɫ
		if (lesson.type == Class)
		{
			setfillcolor(DrawingSetting.color_Lesson);
			settextcolor(DrawingSetting.color_Lesson_Text);
		}
		else if (lesson.type == Break)
		{
			setfillcolor(DrawingSetting.color_Break);
			settextcolor(DrawingSetting.color_Break_Text);
		}

		// �γ���Ŀ
		fillrectangle_RECT(p_rctLessonSchedule[i]);
		drawtext(lesson.note, &p_rctLessonSchedule[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		// ����ʱ���
		if (lesson.type == Class)
		{
			wchar_t buf[128] = {};
			wsprintf(buf, L"%02d:%02d ~ %02d:%02d", lesson.begin.h, lesson.begin.m, lesson.end.h, lesson.end.m);
			drawtext(buf, &p_rctLessonSchedule[i], DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}

		// �γ̰�ť
		if (p_btnLessonSchedule[i].show)
		{
			COLORREF fillcolor;
			switch (p_btnLessonSchedule[i].state)
			{
			case Unselected:		fillcolor = DrawingSetting.color_Btn_Unselected;	break;
			case Good:				fillcolor = DrawingSetting.color_Btn_Good;			break;
			case Normal:			fillcolor = DrawingSetting.color_Btn_Normal;		break;
			case Bad:				fillcolor = DrawingSetting.color_Btn_Bad;			break;
			case Others: default:	fillcolor = DrawingSetting.color_Btn_Others;		break;
			}
			setfillcolor(fillcolor);
			fillrectangle_RECT(p_btnLessonSchedule[i].rct);
		}

		// �򱨸��¼
		int x = p_btnLessonSchedule[i].rct.right + DrawingSetting.n_btn_interval;
		for (int j = 0; j < lesson.report_sum; j++)
		{
			PaintReportIcon(x, p_btnLessonSchedule[i].rct.top);
			x += imgFlag.getwidth();
		}
	}

	settextcolor(WHITE);
}

void PaintFloatBall()
{
	int x = DrawingSetting.pTable.x - DrawingSetting.n_ball_interval;
	int y = p_rctLessonSchedule[stCurrentLesson.nLessonID].top
		+ (int)(stCurrentLesson.dRatioGone * stCurrentLesson.nTotalMins * DrawingSetting.d_len_pre_min);
	setfillcolor(DrawingSetting.color_FloatBall);
	fillcircle(x, y, DrawingSetting.n_ball_radius);
}

void PaintTime()
{
	wchar_t buf[128] = {};
	wsprintf(buf, L"%02d %ls %02d",
		stSystime.wHour,
		stSystime.wSecond % 2 ? L":" : L" ",
		stSystime.wMinute);
	outtextxy(0, DrawingSetting.pProgess.y + 3, buf);
}

void PaintTip()
{
	if (p_LessonSchedule[stCurrentLesson.nLessonID].type == Class)
	{
		int y = p_rctLessonSchedule[nLessonSum - 1].bottom + 20;
		outtextxy(0, y, L"�ϸ�����ʱ�䣡");
		outtextxy(0, y + 20, L"�Ͽ��ڼ����������߶���������Ҫ��򱨸档");
	}
}

// ���������γ̱�
void Schedule::Paint()
{
	UpdateCurrentLessonInfo();

	SetWorkingImage(&m_imgCanvas);
	setbkmode(TRANSPARENT);
	cleardevice();

	PaintTime();			// ����ʱ��
	PaintProgress();		// ���ƽ�����
	PaintReportBtn();		// ���ƴ򱨸水ť
	PaintSchedule();		// ���ƿγ̱�
	PaintFloatBall();		// ����������
	PaintTip();				// ������ʾ����
}

// ��ʼ��
Schedule::Schedule()
{
	// �˳�ʼ�����Ⱥ�˳��
	m_imgCanvas.Resize(800, 900);
	LoadRes();
	UpdateRealTime();
	UpdateCurrentLessonInfo();
	InitInfo();
}

void Schedule::SetOrigin(int x, int y)
{
	m_nOriginX = x;
	m_nOriginY = y;
}

void Schedule::Redraw()
{
	Paint();

	// TODO�������Ż�Ϊ���������ػ棬���� CPU ռ��
	BEGIN_TASK();
	{
		// ����ʱ����
		if (clock() % 12 == 0)
			cleardevice();

		putimage(m_nOriginX, m_nOriginY, &m_imgCanvas);
	}
	END_TASK();
	REDRAW_WINDOW();
}

// ������ʷ
void Schedule::SaveHistory()
{
	wchar_t buf[128] = {};
	wsprintf(buf, L"./history/%d_%d_%d.jpg", stSystime.wYear, stSystime.wMonth, stSystime.wDay);
	saveimage(buf, &m_imgCanvas);
}

// �ж����λ�ã���ť�ж������������ڰ�ť ID��δ�ҵ����� -1
int Schedule::JudgeCursorPosition(int x, int y)
{
	for (int i = 0; i < nLessonSum; i++)
	{
		RECT rct = p_btnLessonSchedule[i].rct;
		MOVE_RECT(rct, m_nOriginX, m_nOriginY);

		if (IsInRect(x, y, rct))
		{
			return i;
		}
	}
	return -1;
}

// ����γ̰�ť���л�ѡ��
void Schedule::ScheduleButtonClick(int id)
{
	// ֻ���Բ�����ʾ�İ�ť
	if (!p_btnLessonSchedule[id].show)
		return;

	// ���ֻ���Բ�����һ�ڿεģ��ٹ�ʱ�Ĳ������ٲ���
	//if (id < stCurrentLesson.nLessonID - 1)
	//	return;

	// ѡ���б�
	ButtonState p_states[5] = { Unselected,Good,Normal,Bad,Others };

	// �ҵ�ԭѡ�����б��ж�Ӧ�� ID
	int state_id = 0;
	for (int i = 0; i < 5; i++)
	{
		if (p_btnLessonSchedule[id].state == p_states[i])
		{
			state_id = i;
		}
	}

	// �л�Ϊ��һ��ѡ��
	state_id++;
	if (state_id >= 5)
		state_id = 1;

	p_btnLessonSchedule[id].state = p_states[state_id];
}

// ����򱨸水ť
void Schedule::ReportButtonClick()
{
	p_LessonSchedule[stCurrentLesson.nLessonID].report_sum++;
}

void Schedule::OnKeyboard()
{
	ShowWindow(GetHWnd(), SW_MINIMIZE);
}

void Schedule::OnMouse(int x, int y)
{
	// ��¼�Ƿ������ʵ���ԵĲ���
	bool operated = false;

	int hover_id = JudgeCursorPosition(x, y);
	if (hover_id != -1)
	{
		ScheduleButtonClick(hover_id);
		operated = true;
	}

	// �Ͽ��ڼ���ܴ򱨸�
	if (p_LessonSchedule[stCurrentLesson.nLessonID].type == Class)
	{
		RECT rct = rct_btnReport;
		MOVE_RECT(rct, m_nOriginX, m_nOriginY);
		if (IsInRect(x, y, rct))
		{
			ReportButtonClick();
		}
		operated = true;
	}

	// ÿ�β�����������ʷ
	if (operated)
	{
		SaveHistory();
	}
}

// ת���������
void ConvertCursorPosition(int& x, int& y)
{
	// �˰汾 HiEasyX �����������ʱ�������Ѿ� setorigin �����
	// ���Դ˴���Ҫ����ת��
	ScreenSize screen_size = GetScreenSize();

	// ����ڻ�ͼԭ����������
	int nCursorX = x + screen_size.left;
	int nCursorY = y + screen_size.top;

	x = nCursorX;
	y = nCursorY;
}

void Schedule::UpdateMsg()
{
	ExMessage msg;
	while (peekmessage(&msg, EM_KEY | EM_MOUSE))
	{
		// ��С��
		if (msg.vkcode == VK_ESCAPE)
		{
			OnKeyboard();
		}

		if (msg.message == WM_LBUTTONUP)
		{
			int x = msg.x;
			int y = msg.y;
			ConvertCursorPosition(x, y);
			OnMouse(x, y);
		}
	}
}
