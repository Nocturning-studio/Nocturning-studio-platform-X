// CDemoRecord.cpp: implementation of the CDemoRecord class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "igame_level.h"
#include "x_ray.h"

#include "gamefont.h"
#include "fDemoRecord.h"
#include "xr_ioconsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "render.h"
#include "CustomHUD.h"
#include "IGame_Persistent.h"

CDemoRecord* xrDemoRecord = 0;
//////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#define DEBUG_DEMO_RECORD
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDemoRecord::CDemoRecord(const char* name, float life_time) : CEffectorCam(cefDemo, life_time /*,FALSE*/)
{
	_unlink(name);
	file = FS.w_open(name);
	if (file)
	{
		IR_Capture(); // capture input
		m_Camera.invert(Device.mView);

		// parse yaw
		Fvector& dir = m_Camera.k;
		Fvector DYaw;
		DYaw.set(dir.x, 0.f, dir.z);
		DYaw.normalize_safe();
		if (DYaw.x < 0)
			m_HPB.x = acosf(DYaw.z);
		else
			m_HPB.x = 2 * PI - acosf(DYaw.z);

		// parse pitch
		dir.normalize_safe();
		m_HPB.y = asinf(dir.y);
		m_HPB.z = 0;

		m_Position.set(m_Camera.c);

		m_vVelocity.set(0, 0, 0);
		m_vAngularVelocity.set(0, 0, 0);
		iCount = 0;

		m_fFov = Device.fFOV;

		if (g_pGamePersistent)
			g_pGamePersistent->GetCurrentDof(m_vGlobalDepthOfFieldParameters);
		m_bAutofocusEnabled = false;
		m_bGridEnabled = false;
		m_bBordersEnabled = false;
		m_bShowInputInfo = true;
		m_bWatermarkEnabled = false;

		m_bGlobalHudDraw = psHUD_Flags.test(HUD_DRAW);
		psHUD_Flags.set(HUD_DRAW, false);

		m_bGlobalCrosshairDraw = psHUD_Flags.test(HUD_CROSSHAIR);
		psHUD_Flags.set(HUD_CROSSHAIR, false);

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);
		m_bMakeCubeMap = FALSE;
		m_bMakeScreenshot = FALSE;
		m_bMakeLevelMap = FALSE;

		m_fSpeed0 = pSettings->r_float("demo_record", "speed0");
		m_fSpeed1 = pSettings->r_float("demo_record", "speed1");
		m_fSpeed2 = pSettings->r_float("demo_record", "speed2");
		m_fSpeed3 = pSettings->r_float("demo_record", "speed3");
		m_fAngSpeed0 = pSettings->r_float("demo_record", "ang_speed0");
		m_fAngSpeed1 = pSettings->r_float("demo_record", "ang_speed1");
		m_fAngSpeed2 = pSettings->r_float("demo_record", "ang_speed2");
		m_fAngSpeed3 = pSettings->r_float("demo_record", "ang_speed3");
	}
	else
	{
		fLifeTime = -1;
	}
	m_bOverlapped = FALSE;
}

CDemoRecord::~CDemoRecord()
{
	if (file)
	{
		IR_Release(); // release input

		FS.w_close(file);

		if (g_pGamePersistent)
		{
			g_pGamePersistent->SetBaseDof(m_vGlobalDepthOfFieldParameters);
			g_pGamePersistent->SetPickableEffectorDOF(false);
		}

		Console->Execute("r2_photo_grid off");
		Console->Execute("r2_cinema_borders off");
		Console->Execute("r2_watermark off");

		psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
		psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);
	}
}

//								+X,				-X,				+Y,				-Y,			+Z,				-Z
static Fvector cmNorm[6] = {{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f},
							{0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};
static Fvector cmDir[6] = {{1.f, 0.f, 0.f},	 {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f},
						   {0.f, -1.f, 0.f}, {0.f, 0.f, 1.f},  {0.f, 0.f, -1.f}};

static Flags32 s_hud_flag = {0};
static Flags32 s_dev_flags = {0};

void CDemoRecord::MakeScreenshotFace()
{
	switch (m_Stage)
	{
	case 0:
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);
		break;
	case 1:
		Render->Screenshot();
		psHUD_Flags.assign(s_hud_flag);
		m_bMakeScreenshot = FALSE;
		break;
	}
	m_Stage++;
}

INT g_bDR_LM_UsePointsBBox = 0;
INT g_bDR_LM_4Steps = 0;
INT g_iDR_LM_Step = 0;
Fvector g_DR_LM_Min, g_DR_LM_Max;

void GetLM_BBox(Fbox& bb, INT Step)
{
	float half_x = bb.min.x + (bb.max.x - bb.min.x) / 2;
	float half_z = bb.min.z + (bb.max.z - bb.min.z) / 2;
	switch (Step)
	{
	case 0: {
		bb.max.x = half_x;
		bb.min.z = half_z;
	}
	break;
	case 1: {
		bb.min.x = half_x;
		bb.min.z = half_z;
	}
	break;
	case 2: {
		bb.max.x = half_x;
		bb.max.z = half_z;
	}
	break;
	case 3: {
		bb.min.x = half_x;
		bb.max.z = half_z;
	}
	break;
	default: {
	}
	break;
	}
};

void CDemoRecord::MakeLevelMapProcess()
{
	switch (m_Stage)
	{
	case 0:
		s_dev_flags = psDeviceFlags;
		psDeviceFlags.zero();
		psDeviceFlags.set(rsClearBB | rsFullscreen | rsDrawStatic, TRUE);
		if (!psDeviceFlags.equal(s_dev_flags, rsFullscreen))
			Device.Reset();
		break;
	case DEVICE_RESET_PRECACHE_FRAME_COUNT + 1: {
		m_bOverlapped = TRUE;
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);

		Fbox bb = g_pGameLevel->ObjectSpace.GetBoundingVolume();

		if (g_bDR_LM_UsePointsBBox)
		{
			bb.max.x = g_DR_LM_Max.x;
			bb.max.z = g_DR_LM_Max.z;

			bb.min.x = g_DR_LM_Min.x;
			bb.min.z = g_DR_LM_Min.z;
		}
		if (g_bDR_LM_4Steps)
			GetLM_BBox(bb, g_iDR_LM_Step);
		// build camera matrix
		bb.getcenter(Device.vCameraPosition);

		Device.vCameraDirection.set(0.f, -1.f, 0.f);
		Device.vCameraTop.set(0.f, 0.f, 1.f);
		Device.vCameraRight.set(1.f, 0.f, 0.f);
		Device.mView.build_camera_dir(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);

		bb.xform(Device.mView);
		// build project matrix
		Device.mProject.build_projection_ortho(bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.min.z, bb.max.z);
	}
	break;
	case DEVICE_RESET_PRECACHE_FRAME_COUNT + 2: {
		m_bOverlapped = FALSE;
		string_path tmp;
		Fbox bb = g_pGameLevel->ObjectSpace.GetBoundingVolume();

		if (g_bDR_LM_UsePointsBBox)
		{
			bb.max.x = g_DR_LM_Max.x;
			bb.max.z = g_DR_LM_Max.z;

			bb.min.x = g_DR_LM_Min.x;
			bb.min.z = g_DR_LM_Min.z;
		}
		if (g_bDR_LM_4Steps)
			GetLM_BBox(bb, g_iDR_LM_Step);

		sprintf_s(tmp, sizeof(tmp), "%s_[%3.3f, %3.3f]-[%3.3f, %3.3f]", *g_pGameLevel->name(), bb.min.x, bb.min.z,
				  bb.max.x, bb.max.z);
		Render->Screenshot(IRender_interface::SM_FOR_LEVELMAP, tmp);
		psHUD_Flags.assign(s_hud_flag);
		BOOL bDevReset = !psDeviceFlags.equal(s_dev_flags, rsFullscreen);
		psDeviceFlags = s_dev_flags;
		if (bDevReset)
			Device.Reset();
		m_bMakeLevelMap = FALSE;
	}
	break;
	}
	m_Stage++;
}

void CDemoRecord::MakeCubeMapFace(Fvector& D, Fvector& N)
{
	string32 buf;
	switch (m_Stage)
	{
	case 0:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		Render->Screenshot(IRender_interface::SM_FOR_CUBEMAP, itoa(m_Stage, buf, 10));
		break;
	case 6:
		Render->Screenshot(IRender_interface::SM_FOR_CUBEMAP, itoa(m_Stage, buf, 10));
		N.set(m_Camera.j);
		D.set(m_Camera.k);
		psHUD_Flags.assign(s_hud_flag);
		m_bMakeCubeMap = FALSE;
		break;
	}
	m_Stage++;
}

void CDemoRecord::SwitchShowInputInfo()
{
	if (m_bShowInputInfo == true)
	{
		m_bShowInputInfo = false;
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchShowInputInfo - method change m_bShowInputInfo to state enabled");
#endif
	}
	else
	{
		m_bShowInputInfo = true;
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchShowInputInfo - method change m_bShowInputInfo to state disabled");
#endif
	}
}

void CDemoRecord::ShowInputInfo()
{
	pApp->pFontSystem->SetColor(color_rgba(255, 255, 255, 127));
	pApp->pFontSystem->SetAligment(CGameFont::alCenter);
	pApp->pFontSystem->OutSetI(0, -.05f);

	pApp->pFontSystem->OutNext("%s", "RECORDING");
	pApp->pFontSystem->OutNext("Key frames count: %d", iCount);

	pApp->pFontSystem->SetAligment(CGameFont::alLeft);
	pApp->pFontSystem->OutSetI(-0.2f, +.05f);
	pApp->pFontSystem->OutNext("J");
	pApp->pFontSystem->OutNext("SPACE");
	pApp->pFontSystem->OutNext("BACK");
	pApp->pFontSystem->OutNext("ESC");
	pApp->pFontSystem->OutNext("F11");
	pApp->pFontSystem->OutNext("F12");
	pApp->pFontSystem->OutNext("G + Mouse Wheel");
	pApp->pFontSystem->OutNext("F + Mouse Wheel");
	pApp->pFontSystem->OutNext("H");
	pApp->pFontSystem->OutNext("V");
	pApp->pFontSystem->OutNext("B");

	pApp->pFontSystem->SetAligment(CGameFont::alLeft);
	pApp->pFontSystem->OutSetI(0, +.05f);
	pApp->pFontSystem->OutNext("= Draw this help");
	pApp->pFontSystem->OutNext("= Append Key");
	pApp->pFontSystem->OutNext("= Cube Map");
	pApp->pFontSystem->OutNext("= Quit");
	pApp->pFontSystem->OutNext("= Level Map ScreenShot");
	pApp->pFontSystem->OutNext("= ScreenShot");
	pApp->pFontSystem->OutNext("= Depth of field");
	pApp->pFontSystem->OutNext("= Field of view");
	pApp->pFontSystem->OutNext("= Autofocus");
	pApp->pFontSystem->OutNext("= Grid");
	pApp->pFontSystem->OutNext("= Cinema borders");
}

BOOL CDemoRecord::Process(Fvector& P, Fvector& D, Fvector& N, float& fFov, float& fFar, float& fAspect)
{
	if (0 == file)
		return TRUE;

	if (m_bMakeScreenshot)
	{
		MakeScreenshotFace();
		// update camera
		N.set(m_Camera.j);
		D.set(m_Camera.k);
		P.set(m_Camera.c);
	}
	else if (m_bMakeLevelMap)
	{
		MakeLevelMapProcess();
	}
	else if (m_bMakeCubeMap)
	{
		MakeCubeMapFace(D, N);
		P.set(m_Camera.c);
		fAspect = 1.f;
	}
	else
	{
		if (m_bShowInputInfo == true)
			ShowInputInfo();

		m_vVelocity.lerp(m_vVelocity, m_vT, 0.3f);
		m_vAngularVelocity.lerp(m_vAngularVelocity, m_vR, 0.3f);

		float speed = m_fSpeed1, ang_speed = m_fAngSpeed1;
		if (IR_GetKeyState(DIK_LSHIFT))
		{
			speed = m_fSpeed0;
			ang_speed = m_fAngSpeed0;
		}
		else if (IR_GetKeyState(DIK_LALT))
		{
			speed = m_fSpeed2;
			ang_speed = m_fAngSpeed2;
		}
		else if (IR_GetKeyState(DIK_LCONTROL))
		{
			speed = m_fSpeed3;
			ang_speed = m_fAngSpeed3;
		}
		m_vT.mul(m_vVelocity, Device.fTimeDelta * speed);
		m_vR.mul(m_vAngularVelocity, Device.fTimeDelta * ang_speed);

		m_HPB.x -= m_vR.y;
		m_HPB.y -= m_vR.x;
		m_HPB.z += m_vR.z;

		// move
		Fvector vmove;

		vmove.set(m_Camera.k);
		vmove.normalize_safe();
		vmove.mul(m_vT.z);
		m_Position.add(vmove);

		vmove.set(m_Camera.i);
		vmove.normalize_safe();
		vmove.mul(m_vT.x);
		m_Position.add(vmove);

		vmove.set(m_Camera.j);
		vmove.normalize_safe();
		vmove.mul(m_vT.y);
		m_Position.add(vmove);

		m_Camera.setHPB(m_HPB.x, m_HPB.y, m_HPB.z);
		m_Camera.translate_over(m_Position);

		// update camera
		N.set(m_Camera.j);
		D.set(m_Camera.k);
		P.set(m_Camera.c);

		fLifeTime -= Device.fTimeDelta;

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);

		fFov = m_fFov;
	}
	return TRUE;
}

void CDemoRecord::SwitchAutofocusState()
{
	if (m_bAutofocusEnabled == false)
	{
		m_bAutofocusEnabled = true;
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchAutofocusState - method change m_bAutofocusEnabled to state enabled");
#endif
	}
	else
	{
		m_bAutofocusEnabled = false;
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchAutofocusState - method change m_bAutofocusEnabled to state disabled");
#endif
	}

	if (g_pGamePersistent)
	{
		g_pGamePersistent->SetPickableEffectorDOF(m_bAutofocusEnabled);
	}
	else
	{
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchAutofocusState - method called before g_pGamePersistent was created. Abort.");
#endif
	}
}

// ������� ����������� ����� ������� ����������, � ������� ����� �������� ����� ����� ����� ������� �� ����� ����� ����
// ����� ������ ��� ���� ��������
void CDemoRecord::SwitchGridState()
{
	if (m_bGridEnabled == false)
	{
		m_bGridEnabled = true;
		Console->Execute("r2_photo_grid on");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchGridState - method change m_bGridEnabled to state enabled and activate r2_photo_grid");
#endif
	}
	else
	{
		m_bGridEnabled = false;
		Console->Execute("r2_photo_grid off");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchGridState - method change m_bGridEnabled to state disabled and deactivate "
			"r2_photo_grid");
#endif
	}
}

// ������� ����������� ����� ������� ����������, � ������� ����� �������� ����� ����� ����� ������� �� ����� ����� ����
// ����� ������ ��� ���� ��������
void CDemoRecord::SwitchCinemaBordersState()
{
	if (m_bBordersEnabled == false)
	{
		m_bBordersEnabled = true;
		Console->Execute("r2_cinema_borders on");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchCinemaBordersState - method change m_bBordersEnabled to state enabled and activate "
			"r2_cinema_borders");
#endif
	}
	else
	{
		m_bBordersEnabled = false;
		Console->Execute("r2_cinema_borders off");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchCinemaBordersState - method change m_bBordersEnabled to state disabled and deactivate "
			"r2_cinema_borders");
#endif
	}
}

void CDemoRecord::SwitchWatermarkVisibility()
{
	if (m_bWatermarkEnabled == false)
	{
		m_bWatermarkEnabled = true;
		Console->Execute("r2_watermark on");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchWatermarkVisibility - method change m_bWatermarkEnabled to state enabled and activate "
			"r2_watermark");
#endif
	}
	else
	{
		m_bWatermarkEnabled = false;
		Console->Execute("r2_watermark off");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchWatermarkVisibility - method change m_bWatermarkEnabled to state disabled and "
			"deactivate r2_watermark");
#endif
	}
}

void CDemoRecord::IR_OnKeyboardPress(int dik)
{
	if (dik == DIK_GRAVE)
		Console->Show();

	if (dik == DIK_SPACE)
		RecordKey();
	if (dik == DIK_BACK)
		MakeCubemap();
	if (dik == DIK_F11)
		MakeLevelMapScreenshot();
	if (dik == DIK_F12)
		MakeScreenshot();
	if (dik == DIK_ESCAPE)
		fLifeTime = -1;
	if (dik == DIK_H)
		SwitchAutofocusState();
	if (dik == DIK_V)
		SwitchGridState();
	if (dik == DIK_B)
		SwitchCinemaBordersState();
	if (dik == DIK_J)
		SwitchShowInputInfo();
	if (dik == DIK_N)
		SwitchWatermarkVisibility();
	if (dik == DIK_RETURN)
	{
		if (g_pGameLevel->CurrentEntity())
		{
#ifndef NDEBUG
			g_pGameLevel->CurrentEntity()->ForceTransform(m_Camera);
#endif
			fLifeTime = -1;
		}
	}
	if (dik == DIK_PAUSE)
		Device.Pause(!Device.Paused(), TRUE, TRUE, "demo_record");
}

void CDemoRecord::IR_OnKeyboardHold(int dik)
{
	switch (dik)
	{
	case DIK_A:
	case DIK_NUMPAD1:
	case DIK_LEFT:
		m_vT.x -= 1.0f;
		break; // Slide Left
	case DIK_D:
	case DIK_NUMPAD3:
	case DIK_RIGHT:
		m_vT.x += 1.0f;
		break; // Slide Right
	case DIK_S:
		m_vT.y -= 1.0f;
		break; // Slide Down
	case DIK_W:
		m_vT.y += 1.0f;
		break; // Slide Up
	// rotate
	case DIK_NUMPAD2:
		m_vR.x -= 1.0f;
		break; // Pitch Down
	case DIK_NUMPAD8:
		m_vR.x += 1.0f;
		break; // Pitch Up
	case DIK_E:
	case DIK_NUMPAD6:
		m_vR.y += 1.0f;
		break; // Turn Left
	case DIK_Q:
	case DIK_NUMPAD4:
		m_vR.y -= 1.0f;
		break; // Turn Right
	case DIK_NUMPAD9:
		m_vR.z -= 2.0f;
		break; // Turn Right
	case DIK_NUMPAD7:
		m_vR.z += 2.0f;
		break; // Turn Right
	}
}

void CDemoRecord::IR_OnMouseMove(int dx, int dy)
{
	float scale = .5f; // psMouseSens;
	if (dx || dy)
	{
		m_vR.y += float(dx) * scale;													// heading
		m_vR.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
	}
}

void CDemoRecord::IR_OnMouseHold(int btn)
{
	switch (btn)
	{
	case 0:
		m_vT.z += 1.0f;
		break; // Move Backward
	case 1:
		m_vT.z -= 1.0f;
		break; // Move Forward
	}
}

void CDemoRecord::ChangeDepthOfField(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

	if (g_pGamePersistent)
	{
		g_pGamePersistent->GetCurrentDof(dof_params_old);

		dof_params_actual = dof_params_old;

		if (direction > 0)
			dof_params_actual.z = dof_params_old.z + 10.0f;
		else
			dof_params_actual.z = dof_params_old.z - 10.0f;

		if (dof_params_actual.z <= 4.999f)
		{
#ifdef DEBUG_DEMO_RECORD
			Msg("CDemoRecord::ChangeDepthOfField - far parameter < 5");
#endif
			dof_params_actual.z = 5.0f;
		}

		g_pGamePersistent->SetBaseDof(dof_params_actual);

#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::ChangeDepthOfField - method successfully change depth of field parameters");
		Msg("CDemoRecord::ChangeDepthOfField - depth of field parameters old: near = %d, focus = %d, far = %d",
			dof_params_old.x, dof_params_old.y, dof_params_old.z);
		Msg("CDemoRecord::ChangeDepthOfField - depth of field parameters actual: near = %d, focus = %d, far = %d",
			dof_params_actual.x, dof_params_actual.y, dof_params_actual.z);
#endif
	}
#ifdef DEBUG_DEMO_RECORD
	else
	{
		Msg("CDemoRecord::ChangeDepthOfField - method was called before create IGame_Persistent. Going next");
	}
#endif
}

void CDemoRecord::ChangeFieldOfView(int direction)
{
	float m_fFov_old = Device.fFOV;
	float m_fFov_actual;

	if (direction > 0)
		m_fFov_actual = m_fFov_old + 0.5f;
	else
		m_fFov_actual = m_fFov_old - 0.5f;

	if (m_fFov_actual <= 2.28f)
	{
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::ChangeFieldOfView - field of view parameter < 2.29 degrees. Set 2.29 degrees");
#endif
		m_fFov_actual = 2.28f;
	}
	else if (m_fFov_actual >= 113.001f)
	{
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::ChangeFieldOfView - field of view parameter > 113 degrees. Set 113 degrees");
#endif
		m_fFov_actual = 113.0f;
	}

	if (m_fFov < m_fFov_actual)
	{
		while (m_fFov < m_fFov_actual)
		{
			m_fFov += 0.00001f;
		}
	}

	if (m_fFov > m_fFov_actual)
	{
		while (m_fFov > m_fFov_actual)
		{
			m_fFov -= 0.00001f;
		}
	}
#ifdef DEBUG_DEMO_RECORD
	Msg("CDemoRecord::ChangeFieldOfView - method successfully change field of view parameter");
	Msg("CDemoRecord::ChangeFieldOfView - field of view value old: %d", m_fFov_old);
	Msg("CDemoRecord::ChangeFieldOfView - field of view value actual: %d", m_fFov_actual);
#endif
}

void CDemoRecord::IR_OnMouseWheel(int direction)
{
	if (IR_GetKeyState(DIK_G))
	{
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::IR_OnMouseWheel - Whell mode is DepthOfFieldChanger");
#endif
		ChangeDepthOfField(direction);
	}
	else if (IR_GetKeyState(DIK_F))
	{
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::IR_OnMouseWheel - Whell mode is FieldOfViewChanger");
#endif
		ChangeFieldOfView(direction);
	}
}

void CDemoRecord::RecordKey()
{
	Fmatrix g_matView;

	g_matView.invert(m_Camera);
	file->w(&g_matView, sizeof(Fmatrix));
	iCount++;
}

void CDemoRecord::MakeCubemap()
{
	m_bMakeCubeMap = TRUE;
	m_Stage = 0;
}

void CDemoRecord::MakeScreenshot()
{
	m_bMakeScreenshot = TRUE;
	m_Stage = 0;
}

void CDemoRecord::MakeLevelMapScreenshot()
{
	m_bMakeLevelMap = TRUE;
	m_Stage = 0;
}
