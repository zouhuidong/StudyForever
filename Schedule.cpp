#include "Schedule.h"
#include "HiEasyX.h"

// 全局时间
SYSTEMTIME stSystime;

// 到了一天的结束
bool bDayEnd = false;

// 小旗子
IMAGE imgFlag;

// 上下课音效
hiex::MusicMCI musicBeginClass;
hiex::MusicMCI musicEndClass;

// 时间点，{-1,-1} 表示和上一个时间点相同
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
	int report_sum = 0;	// 打报告次数
};

// 当前课程信息
struct CurrentLessonInfo
{
	int nLessonID = -1;	// 首次运行程序时，以 -1 标记 ID
	int nTotalMins = 0;
	int nMinsGone = 0;
	int nMinsRemaining = 0;
	double dRatioGone = 0;

}stCurrentLesson;

const int nLessonSum = 29;	// 课程总数

// 课程表（简写版）
Lesson p_LessonSchedule_Simplified[nLessonSum] =
{
	{{6,55}, {7,45}, Class, L"早自习"},
	{{-1,-1}, {7,55}, Break},
	{{-1,-1}, {8,35}, Class, L"上午·第一节课"},
	{{-1,-1}, {8,45}, Break},
	{{-1,-1}, {9,25}, Class, L"上午·第二节课"},
	{{-1,-1}, {9,35}, Break},
	{{-1,-1}, {10,15}, Class, L"上午·第三节课"},
	{{-1,-1}, {10,30}, Break, L"大课间"},
	{{-1,-1}, {11,10}, Class, L"上午·第四节课"},
	{{-1,-1}, {11,20}, Break},
	{{-1,-1}, {12,00}, Class, L"上午·第五节课"},
	{{-1,-1}, {12,30}, Break, L"午饭休息"},
	{{-1,-1}, {13,10}, Class, L"午自习"},
	{{-1,-1}, {14,20}, Break, L"午休"},
	{{-1,-1}, {15,00}, Class, L"下午·第一节课"},
	{{-1,-1}, {15,10}, Break},
	{{-1,-1}, {15,50}, Class, L"下午·第二节课"},
	{{-1,-1}, {16,00}, Break},
	{{-1,-1}, {16,40}, Class, L"下午·第三节课"},
	{{-1,-1}, {16,50}, Break},
	{{-1,-1}, {17,30}, Class, L"下午·第四节课"},
	{{-1,-1}, {18,20}, Break, L"下午自由活动"},
	{{-1,-1}, {19,00}, Class, L"提前晚自习"},
	{{-1,-1}, {19,20}, Break, L"大课间"},
	{{-1,-1}, {20,20}, Class, L"晚修一"},
	{{-1,-1}, {20,30}, Break},
	{{-1,-1}, {21,10}, Class, L"晚修二"},
	{{-1,-1}, {21,20}, Break},
	{{-1,-1}, {22,00}, Class, L"晚修三"},
};

// 课程表（全部已转换为具体时间，不含 {-1,-1}）
Lesson p_LessonSchedule[nLessonSum];

// 课程表对应的绘制区块
RECT p_rctLessonSchedule[nLessonSum];

enum ButtonState
{
	Unselected,
	Good,
	Normal,
	Bad,
	Others
};

// 课程表条目对应的按钮
struct Button
{
	bool show;	// 是否显示
	RECT rct;
	ButtonState state = Unselected;
};

Button p_btnLessonSchedule[nLessonSum];
RECT rct_btnReport;	// 打报告按钮

// 绘制属性
struct tagLessonTableDrawingSetting
{
	// 进度条左上角位置
	POINT pProgess = { 50,10 };

	// 表格左上角位置
	POINT pTable = { 50,50 };

	double d_len_pre_min = 0.8;	// 课程条目长度（每分钟）
	int n_width = 500;			// 课程条目宽度
	int n_progess_height = 20;	// 进度条高度

	int n_ball_interval = 10;
	int n_ball_radius = 5;

	// 打报告按钮
	POINT pReport = { pProgess.x + n_width + 10,pProgess.y };

	// 课程表按钮
	int n_btn_interval = 10;	// 按钮和课程条目的间隔
	int n_btn_width = 30;		// 按钮宽度

	// 进度条颜色
	COLORREF color_Progress = GREEN;

	COLORREF color_FloatBall = WHITE;

	// 课程条目颜色
	COLORREF color_Lesson = ORANGE;
	COLORREF color_Lesson_Text = BLACK;
	COLORREF color_Break = BLUE;
	COLORREF color_Break_Text = WHITE;

	// 课程按钮颜色
	COLORREF color_Btn_Good = GREEN;
	COLORREF color_Btn_Normal = RGB(150, 170, 0);
	COLORREF color_Btn_Bad = RED;
	COLORREF color_Btn_Others = DARKGRAY;
	COLORREF color_Btn_Unselected = RGB(230, 230, 230);

}DrawingSetting;

// 获取两个时间点之间的时长（分钟）
int GetTimeLen_Min(TimePoint begin, TimePoint end)
{
	return (end.h * 60 + end.m) - (begin.h * 60 + begin.m);
}

// 判断某时间点是否处于某时间段内（不包括终点时刻）
bool IsInTimePeriod(TimePoint target, TimePoint begin, TimePoint end)
{
	int len_target = GetTimeLen_Min(begin, target);
	int len_total = GetTimeLen_Min(begin, end);
	return len_target >= 0 && len_target < len_total;
}

// 更新课程表的时间为真实时间（修改 {-1,-1} 为实际时间）
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

// 初始化相关信息
// 必须先 UpdateCurrentInfo
void InitInfo()
{
	// 填充 RECT 区块信息
	// 初始位移
	int x = DrawingSetting.pTable.x;
	int y = DrawingSetting.pTable.y;

	// 不考虑初始位移的 y 坐标记录点
	int n_current_y = 0;
	for (int i = 0; i < nLessonSum; i++)
	{
		Lesson lesson = p_LessonSchedule[i];
		int mins = GetTimeLen_Min(lesson.begin, lesson.end);

		// 矩形条目高度
		int height = (int)(mins * DrawingSetting.d_len_pre_min);

		// 填充课程表条目的矩形区块
		p_rctLessonSchedule[i].left = x;
		p_rctLessonSchedule[i].right = x + DrawingSetting.n_width;
		p_rctLessonSchedule[i].top = y + n_current_y;
		p_rctLessonSchedule[i].bottom = y + n_current_y + height;

		// 填充按钮信息

		// 在此初始化阶段，已经迟到的课程按钮标记为 Others 状态，其余课程按钮均不显示。
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

	// 打报告按钮
	rct_btnReport = {
		DrawingSetting.pReport.x,
		DrawingSetting.pReport.y,
		DrawingSetting.pReport.x + DrawingSetting.n_btn_width,
		DrawingSetting.pReport.y + DrawingSetting.n_progess_height,
	};
}

// 加载资源
void LoadRes()
{
	loadimage(&imgFlag, L"./res/flag.bmp");
	musicBeginClass.open(L"./res/begin_class.mp3");
	musicEndClass.open(L"./res/end_class.mp3");
}

// 更新全局时间（在 UpdateCurrentInfo 中会被调用）
void UpdateTime()
{
	GetLocalTime(&stSystime);
}

// 播放上下课铃声
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

// 时间到了，切换课程
// 若为一天的结束（end_of_day），则参数 id 无效，可传入 0
void SwitchClass(int id, bool end_of_day)
{
	if (!end_of_day)
	{
		// 播放铃声
		PlayClassSound(p_LessonSchedule[id].type == Class);

		// 可以使用按钮
		if (id - 1 >= 0 && p_LessonSchedule[id - 1].type == Class)
		{
			p_btnLessonSchedule[id - 1].show = true;
		}
	}
	else
	{
		// 播放铃声
		PlayClassSound(false);

		// 可以使用按钮
		p_btnLessonSchedule[nLessonSum - 1].show = true;
	}
}

// 更新当前课程信息和时间信息
void UpdateCurrentLessonInfo()
{
	UpdateTime();

	TimePoint pCurrentTime = { stSystime.wHour,stSystime.wMinute };

	// 匹配所在课程
	bool bMatched = false;	// 是否匹配成功
	for (int i = 0; i < nLessonSum; i++)
	{
		Lesson lesson = p_LessonSchedule[i];
		if (IsInTimePeriod(pCurrentTime, lesson.begin, lesson.end))
		{
			// 上下课
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

	// 超出时间范围，补救
	if (!bMatched)
	{
		// 上课前
		if (pCurrentTime.h <= p_LessonSchedule[0].begin.h)
		{
			stCurrentLesson.nLessonID = 0;
			stCurrentLesson.dRatioGone = 0;
		}
		else
		{
			// 到了一天的结束
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
	// 填充
	setfillcolor(DrawingSetting.color_Progress);
	solidrectangle(
		DrawingSetting.pProgess.x,
		DrawingSetting.pProgess.y,
		DrawingSetting.pProgess.x + (int)(DrawingSetting.n_width * stCurrentLesson.dRatioGone),
		DrawingSetting.pProgess.y + DrawingSetting.n_progess_height
	);

	// 外框
	rectangle(
		DrawingSetting.pProgess.x,
		DrawingSetting.pProgess.y,
		DrawingSetting.pProgess.x + DrawingSetting.n_width,
		DrawingSetting.pProgess.y + DrawingSetting.n_progess_height
	);

	// 文字
	wchar_t buf[128] = {};
	wsprintf(buf, L"%ls共 %d 分钟。( %d'%d\"%dms / %d )",
		(p_LessonSchedule[stCurrentLesson.nLessonID].type == Class) ? L"本节课" : L"休息时间",
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
	drawtext(L"报告", &rct_btnReport, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// 绘制小型的打报告图标
void PaintReportIcon(int x, int y)
{
	putimage(x, y, &imgFlag);
}

void PaintSchedule()
{
	for (int i = 0; i < nLessonSum; i++)
	{
		Lesson lesson = p_LessonSchedule[i];

		// 设置填充颜色
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

		// 课程条目
		fillrectangle_RECT(p_rctLessonSchedule[i]);
		drawtext(lesson.note, &p_rctLessonSchedule[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		// 绘制时间段
		if (lesson.type == Class)
		{
			wchar_t buf[128] = {};
			wsprintf(buf, L"%02d:%02d ~ %02d:%02d", lesson.begin.h, lesson.begin.m, lesson.end.h, lesson.end.m);
			drawtext(buf, &p_rctLessonSchedule[i], DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}

		// 课程按钮
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

		// 打报告记录
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
		outtextxy(0, y, L"严格遵守时间！");
		outtextxy(0, y + 20, L"上课期间请勿随意走动，如有需要请打报告。");
	}
}

// 绘制整个课程表
void Schedule::Paint()
{
	UpdateCurrentLessonInfo();

	SetWorkingImage(&m_imgCanvas);
	setbkmode(TRANSPARENT);
	cleardevice();

	PaintTime();			// 绘制时间
	PaintProgress();		// 绘制进度条
	PaintReportBtn();		// 绘制打报告按钮
	PaintSchedule();		// 绘制课程表
	PaintFloatBall();		// 绘制悬浮球
	PaintTip();				// 绘制提示文字
}

// 初始化
Schedule::Schedule()
{
	// 此初始化有先后顺序
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

	// TODO：可以优化为部分区域重绘，降低 CPU 占用
	BEGIN_TASK();
	{
		// 不定时清屏
		if (clock() % 12 == 0)
			cleardevice();

		putimage(m_nOriginX, m_nOriginY, &m_imgCanvas);
	}
	END_TASK();
	REDRAW_WINDOW();
}

// 保存历史
void Schedule::SaveHistory()
{
	wchar_t buf[128] = {};
	wsprintf(buf, L"./history/%d_%d_%d.jpg", stSystime.wYear, stSystime.wMonth, stSystime.wDay);
	saveimage(buf, &m_imgCanvas);
}

// 判断鼠标位置（按钮判定），返回所在按钮 ID，未找到返回 -1
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

// 点击课程按钮，切换选择
void Schedule::ScheduleButtonClick(int id)
{
	// 只可以操作显示的按钮
	if (!p_btnLessonSchedule[id].show)
		return;

	// 最多只可以操作上一节课的，再过时的不可以再操作
	//if (id < stCurrentLesson.nLessonID - 1)
	//	return;

	// 选择列表
	ButtonState p_states[5] = { Unselected,Good,Normal,Bad,Others };

	// 找到原选择在列表中对应的 ID
	int state_id = 0;
	for (int i = 0; i < 5; i++)
	{
		if (p_btnLessonSchedule[id].state == p_states[i])
		{
			state_id = i;
		}
	}

	// 切换为下一个选择
	state_id++;
	if (state_id >= 5)
		state_id = 1;

	p_btnLessonSchedule[id].state = p_states[state_id];
}

// 点击打报告按钮
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
	// 记录是否进行了实质性的操作
	bool operated = false;

	int hover_id = JudgeCursorPosition(x, y);
	if (hover_id != -1)
	{
		ScheduleButtonClick(hover_id);
		operated = true;
	}

	// 上课期间才能打报告
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

	// 每次操作都保存历史
	if (operated)
	{
		SaveHistory();
	}
}

// 转换鼠标坐标
void ConvertCursorPosition(int& x, int& y)
{
	// 此版本 HiEasyX 计算鼠标坐标时不考虑已经 setorigin 的情况
	// 所以此处需要进行转换
	ScreenSize screen_size = GetScreenSize();

	// 相对于绘图原点的鼠标坐标
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
		// 最小化
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
