// CDemoPlay.cpp: implementation of the CDemoPlay class.
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "fdemoplay.h"
#include "xr_ioconsole.h"
#include "motion.h"
#include "Render.h"
#include "CameraManager.h"
#include "Benchmark.h"
#include <demo_common.h>
#include "gamefont.h"
#include "x_ray.h"
#include "xr_input.h"
#include "igame_level.h"
#include "iinputreceiver.h"
#include "../xrGame/Level.h"
#include "CustomHUD.h"
//////////////////////////////////////////////////////////////////////
CDemoPlay::CDemoPlay(const char* name, float ms, u32 cycles, float life_time): CEffectorCam(cefDemo, life_time)
{
	// Есть ли файл
	if (!FS.exist(name))
	{
		Msg("Can't find file: %s", name);
		g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
		return;
	}

	strcpy(demo_file_name, name);
	ReadAllFramesDataFromIni(demo_file_name);
	Msg("*** Playing demo: %s", demo_file_name);

	m_frames_count = TotalFramesCount;
	Log("~ Total key-frames: ", m_frames_count);

	// Защита на случай если файл пришел пустым
	if (m_frames_count == NULL)
	{
		Msg("File corrupted: frames count is zero");
		g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
		return;
	}

	// Захват инпута с клавиатуры и мыши
	IR_Capture();

	// Отключение лишнего
	m_bGlobalHudDraw = psHUD_Flags.test(HUD_DRAW);
	psHUD_Flags.set(HUD_DRAW, false);

	m_bGlobalCrosshairDraw = psHUD_Flags.test(HUD_CROSSHAIR);
	psHUD_Flags.set(HUD_CROSSHAIR, false);

	// Скорость и количество повторов демо
	fSpeed = ms;
	dwCyclesLeft = cycles ? cycles : 1;

	// Запущен ли сбор общей статистики (для бенчмарка)
	stat_started = FALSE;

	// Прекеш
	Device.PreCache(50);

	// Сет дефолтных значений параметров, которые будут читаться из секции кадра
	SetDefaultParameters();

	// Переменные для окна конца бенчмарка
	bNeedDrawResults = false;
	bNeedToTakeStatsResoultScreenShot = false;
	uTimeToQuit = uTimeToScreenShot = NULL;

	// Переменные для сбора общей и покадровой статистики
	ResetPerFrameStatistic();
}

CDemoPlay::~CDemoPlay()
{
	// Отпускаем инпут
	IR_Release();

	// Возвращаем индикаторы обратно
	psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
	psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);

	// Сет дефолтных значений параметров, которые установлены из секции кадра
	ResetParameters();
}

// Оригинальная формула для интерполяции между кадрами
// t: A parameter between 0 and 1 representing the interpolation factor.
// p: An array of three Fvector points that serve as control points for the spline.
// ret: A pointer to an Fvector where the result will be stored.
void spline1(float t, Fvector* p, Fvector* ret)
{
	float t2 = t * t;
	float t3 = t2 * t;
	float m[4];

	ret->x = 0.0f;
	ret->y = 0.0f;
	ret->z = 0.0f;

	// Calculate spline coefficients
	m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
	m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
	m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
	m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

	// Interpolate position based on control points
	for (int i = 0; i < 4; i++)
	{
		ret->x += p[i].x * m[i];
		ret->y += p[i].y * m[i];
		ret->z += p[i].z * m[i];
	}
}

void CDemoPlay::MoveCameraSpline(float InterpolationFactor, int frame0, int frame1, int frame2, int frame3)
{
	Fmatrix *m1, *m2, *m3, *m4;
	m1 = (Fmatrix*)&MakeCameraMatrixFromFrameNumber(frame0);
	m2 = (Fmatrix*)&MakeCameraMatrixFromFrameNumber(frame1);
	m3 = (Fmatrix*)&MakeCameraMatrixFromFrameNumber(frame2);
	m4 = (Fmatrix*)&MakeCameraMatrixFromFrameNumber(frame3);

	for (int i = 0; i < 4; i++)
	{
		Fvector v[4];
		v[0].x = m1->m[i][0];
		v[0].y = m1->m[i][1];
		v[0].z = m1->m[i][2];

		v[1].x = m2->m[i][0];
		v[1].y = m2->m[i][1];
		v[1].z = m2->m[i][2];

		v[2].x = m3->m[i][0];
		v[2].y = m3->m[i][1];
		v[2].z = m3->m[i][2];

		v[3].x = m4->m[i][0];
		v[3].y = m4->m[i][1];
		v[3].z = m4->m[i][2];

		spline1(InterpolationFactor, &(v[0]), (Fvector*)&(m_Camera.m[i][0]));
	}
}

// t: A parameter between 0 and 1 representing the interpolation factor.
// p0: An array of three Fvector points that serve first point
// p1: An array of three Fvector points that serve second point
// ret: A pointer to an Fvector where the result will be stored.
void linearInterpolate(float t, Fvector* p0, Fvector* p1, Fvector* ret)
{
	ret->x = (1 - t) * p0->x + t * p1->x;
	ret->y = (1 - t) * p0->y + t * p1->y;
	ret->z = (1 - t) * p0->z + t * p1->z;
}

void CDemoPlay::MoveCameraLinear(float InterpolationFactor, int frame0, int frame1)
{
	Fmatrix* m1 = (Fmatrix*)&MakeCameraMatrixFromFrameNumber(frame0);
	Fmatrix* m2 = (Fmatrix*)&MakeCameraMatrixFromFrameNumber(frame1);

	for (int i = 0; i < 4; i++)
	{
		Fvector pos0, pos1;
		pos0.x = m1->m[i][0];
		pos0.y = m1->m[i][1];
		pos0.z = m1->m[i][2];

		pos1.x = m2->m[i][0];
		pos1.y = m2->m[i][1];
		pos1.z = m2->m[i][2];

		// Perform linear interpolation
		linearInterpolate(InterpolationFactor, &pos0, &pos1, (Fvector*)&(m_Camera.m[i][0]));
	}
}

// Движение камеры между кадрами
void CDemoPlay::MoveCamera(u32 frame, float interpolation_factor, int interpolation_type)
{
	int f1 = frame;

	int f2 = f1 + 1;
	f2 = NeedInterpolation(f2) ? f2 : f1;

	int f3 = f2 + 1;
	f3 = NeedInterpolation(f3) ? f3 : f2;

	int f4 = f3 + 1;
	f4 = NeedInterpolation(f4) ? f4 : f3;

	switch (interpolation_type)
	{
	case LINEAR_INTERPOLATION_TYPE:
		MoveCameraLinear(interpolation_factor, f1, f2);
		break;
	case SPLINE_INTERPOLATION_TYPE:
		MoveCameraSpline(interpolation_factor, f1, f2, f3, f4);
		break;
	case DISABLE_INTERPOLATION:
		m_Camera.set(MakeCameraMatrixFromFrameNumber(frame));
		break;
	}
}

// Апдейт камеры
void CDemoPlay::Update(SCamEffectorInfo& info)
{
#ifdef BENCHMARK_BUILD
	if (bNeedDrawResults)
		PrintSummaryBanchmarkStatistic();
	else
		ShowPerFrameStatistic();
#endif

	fStartTime += Device.fTimeDelta;

	float ip;
	float p = fStartTime / fSpeed;
	float InterpolationFactor = modff(p, &ip);
	int Frame = iFloor(ip);
	VERIFY(t >= 0);

	// Отслеживаем подошли ли мы к последним кадрам демо
	// чтобы засетить в это время вывод статистики
#ifdef BENCHMARK_BUILD
	if (frame == (m_frames_count - 10))
		EnableBenchmarkResultPrint();
#endif

	// Что делать если мы подошли к концу демо
	if (Frame >= m_frames_count)
	{
#ifdef BENCHMARK_BUILD
		// Закольцовываем демо в себе
		//ResetPerFrameStatistic();
		//frame = 0;
		//fStartTime = 1;
		//fLifeTime = 10000;
		Console->Execute("quit");
#else
		dwCyclesLeft--;

		if (0 == dwCyclesLeft)
			Close();
#endif
	}

	// Берем номер ключевого кадра и читаем соотвтетствующую ему секцию
	// после чего сетим постпроцесс из ее параметров c интерполяцией
	ApplyFrameParameters(Frame, InterpolationFactor);

	// Move обновляет view матрицу при помощи нужного типа интерполяции
	if (NeedInterpolation(Frame) && Frame != m_frames_count)
	{
		MoveCamera(Frame, InterpolationFactor, GetInterpolationType(Frame));
	}
	else
	{
		// Телепортируем камеру на нужную точку и сразу же начинаем оттуда же уже с интерполяцией
		MoveCamera(Frame, InterpolationFactor, DISABLE_INTERPOLATION);
		MoveCamera(Frame, InterpolationFactor, GetInterpolationType(Frame + 1));
	}

	// Применяем view матрицу для трансформации нормали, направления и позиции
	info.n.set(m_Camera.j);
	info.d.set(m_Camera.k);
	info.p.set(m_Camera.c);
	info.fFov = g_fFov;

	fLifeTime -= Device.fTimeDelta;

	// Короткая демонстрация результатов бенчмарка со скриншотом и завершением работы
#ifdef BENCHMARK_BUILD
	if (!bNeedDrawResults)
		return;

	if ((Device.dwTimeGlobal >= uTimeToScreenShot) && bNeedToTakeStatsResoultScreenShot)
	{
		bNeedToTakeStatsResoultScreenShot = false;
		Screenshot();
	}

	if (Device.dwTimeGlobal >= uTimeToQuit)
		Console->Execute("quit");
#endif
}

BOOL CDemoPlay::ProcessCam(SCamEffectorInfo& info)
{
	// skeep a few frames before counting
	if (Device.dwPrecacheFrame)
		return TRUE;

	// Защита на случай если файл придет пустой
	if (m_frames_count == NULL)
		Close();

	Update(info);

	return TRUE;
}

void CDemoPlay::Screenshot()
{
	Render->Screenshot();
}

void CDemoPlay::EnableBenchmarkResultPrint()
{
	uTimeToQuit = Device.dwTimeGlobal + 5000;
	uTimeToScreenShot = Device.dwTimeGlobal + 1000;
	bNeedDrawResults = true;
	bNeedToTakeStatsResoultScreenShot = true;
	fLifeTime = 1000;
}

void CDemoPlay::Close()
{
	g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
	fLifeTime = -1;
}

// Обработчик нажатий клавиш клавиатуры
void CDemoPlay::IR_OnKeyboardPress(int dik)
{
#ifdef BENCHMARK_BUILD
	if (dik == DIK_ESCAPE)
		EnableBenchmarkResultPrint();
#else
	if (dik == DIK_ESCAPE)
		Close();

	if (dik == DIK_GRAVE)
		Console->Show();
#endif

	if (dik == DIK_F12)
		Screenshot();
}


void CDemoPlay::PrintSummaryBanchmarkStatistic()
{
	// Выравниваем надпись по левому краю строки
	pApp->pFontSystem->SetAligment(CGameFont::alCenter);

	// Сетим надписи в центре экрана
	pApp->pFontSystem->OutSetI(0.0, -0.2f);

	pApp->pFontSystem->OutNext("Benchmark results");

	ChooseTextColor(fFPS_max);
	pApp->pFontSystem->OutNext("FPS Maximal: %f", fFPS_max);

	ChooseTextColor(fFPS_min);
	pApp->pFontSystem->OutNext("FPS Minimal: %f", fFPS_min);

	pApp->pFontSystem->OutNext("GPU: %s", HW.Caps.id_description);

	if (Device.dwTimeGlobal > uTimeToScreenShot)
		pApp->pFontSystem->OutNext("Results saved to screenshots");
}

void CDemoPlay::ResetPerFrameStatistic()
{
	fFPS = 0.0f;
	fFPS_min = flt_max;
	fFPS_max = flt_min;
}

// Разные цвета для разных значений кадров в секунду
// Зеленый если больше 50
// Желтый если меньше 50
// Красный если меньше 24
void CDemoPlay::ChooseTextColor(float FPSValue)
{
	if (fFPS > 50.0f)
		pApp->pFontSystem->SetColor(color_rgba(101, 255, 0, 200));
	else if (fFPS < 50.0f)
		pApp->pFontSystem->SetColor(color_rgba(230, 255, 130, 200));
	else if (fFPS < 24.0f)
		pApp->pFontSystem->SetColor(color_rgba(255, 59, 0, 200));
}

// Статистика в левом верхнем углу экрана
void CDemoPlay::ShowPerFrameStatistic()
{
	// Считаем время кадра
	float fps = 1.f / Device.fTimeDelta;
	float fOne = 0.3f;
	float fInv = 1.f - fOne;
	fFPS = fInv * fFPS + fOne * fps;

	// При alt + tab бывает скачок количества кадров
	// попытка убавить значение до среднего чтобы в
	// следующем вызове он выравнялся до нормального
	if (fFPS_max > 256.0f)
		fFPS_max = 60.0f;

	// Если актуальное значение меньше чем у прошлого кадра
	// то приравниваем его
	if (fFPS < fFPS_min)
		fFPS_min = fFPS;

	// Если актуальное значение больше чем у прошлого кадра
	// то приравниваем его
	if (fFPS > fFPS_max)
		fFPS_max = fFPS;

	// Выравниваем надпись по левому краю строки
	pApp->pFontSystem->SetAligment(CGameFont::alLeft);

	// Сетим надписи в левом верхнем углу
	pApp->pFontSystem->OutSetI(-1.0, -0.8f);

	ChooseTextColor(fFPS);
	pApp->pFontSystem->OutNext("FPS: %f", fFPS);

	ChooseTextColor(fFPS_max);
	pApp->pFontSystem->OutNext("FPS Maximal: %f", fFPS_max);

	ChooseTextColor(fFPS_min);
	pApp->pFontSystem->OutNext("FPS Minimal: %f", fFPS_min);

	pApp->pFontSystem->OutNext("GPU: %s", HW.Caps.id_description);
}
//////////////////////////////////////////////////////////////////////
