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
//////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#define DEBUG_DEMO_RECORD
#endif
//////////////////////////////////////////////////////////////////////
CDemoRecord* xrDemoRecord = 0;
CDemoRecord::force_position CDemoRecord::g_position = {false, {0, 0, 0}};
//////////////////////////////////////////////////////////////////////
void CDemoRecord::update_whith_timescale(Fvector& v, const Fvector& v_delta)
{
	VERIFY(!fis_zero(Device.time_factor()));
	float scale = 1.f / Device.time_factor();
	v.mad(v, v_delta, scale);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDemoRecord::CDemoRecord(const char* name, float life_time) : CEffectorCam(cefDemo, life_time /*,FALSE*/)
{
	_unlink(name);
	file = FS.w_open(name);
	if (file)
	{
		g_position.set_position = false;
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

		m_vGlobalDepthOfFieldParameters.set(0, 0, 0);

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

		Console->Execute("r_photo_grid off");
		Console->Execute("r_cinema_borders off");
		Console->Execute("r_watermark off");

#ifndef MASTER_GOLD
		Console->Execute("r_debug_render disabled");
#endif

		psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
		psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);
	}
}

//								+X,				-X,				+Y,				-Y,			+Z,				-Z
Fvector CDemoRecord::cmNorm[6] = {{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f},
							{0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};
Fvector CDemoRecord::cmDir[6] = {{1.f, 0.f, 0.f},  {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f},
						   {0.f, -1.f, 0.f}, {0.f, 0.f, 1.f},  {0.f, 0.f, -1.f}};

Flags32 CDemoRecord::s_hud_flag = {0};
Flags32 CDemoRecord::s_dev_flags = {0};

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
		psDeviceFlags.set(rsClearBB | rsDrawStatic, TRUE);
		psDeviceFlags.set(rsFullscreen, s_dev_flags.test(rsFullscreen));
		break;
	case DEVICE_RESET_PRECACHE_FRAME_COUNT + 1: {
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
		psDeviceFlags = s_dev_flags;
		m_bMakeLevelMap = FALSE;
	}
	break;
	}
	m_Stage++;
}

void CDemoRecord::MakeCubeMapFace(Fvector& D, Fvector& N)
{
	Console->Execute("r_disable_postprocess on");

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

	Console->Execute("r_disable_postprocess off");
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
	pApp->pFontSystem->SetColor(color_rgba(215, 195, 170, 175));
	pApp->pFontSystem->SetAligment(CGameFont::alCenter);
	pApp->pFontSystem->OutSetI(0, -.22f);

	pApp->pFontSystem->OutNext("%s", "RECORDING");
	pApp->pFontSystem->OutNext("Key frames count: %d", iCount);
	pApp->pFontSystem->OutNext("Depth of field focus distance: %f", m_fDOF.z);
	pApp->pFontSystem->OutNext("Field of view: %f", m_fFov);

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

#ifndef MASTER_GOLD
	pApp->pFontSystem->OutNext("1, 2, ..., 0");
	pApp->pFontSystem->OutNext("ENTER");
#endif

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

#ifndef MASTER_GOLD
	pApp->pFontSystem->OutNext("= Render debugging");
	pApp->pFontSystem->OutNext("= Actor teleportation");
#endif
}

BOOL CDemoRecord::ProcessCam(SCamEffectorInfo& info)
{
	if (0 == file)
		return TRUE;

	if (m_bMakeScreenshot)
	{
		MakeScreenshotFace();
		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);
		info.fFov = m_fFov;
	}
	else if (m_bMakeLevelMap)
	{
		MakeLevelMapProcess();
	}
	else if (m_bMakeCubeMap)
	{
		info.fFov = 90.0f;
		MakeCubeMapFace(info.d, info.n);
		info.p.set(m_Camera.c);
		info.fAspect = 1.f;
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

		if (g_position.set_position)
		{
			m_Position.set(g_position.p);
			g_position.set_position = false;
		}
		else
		{
			g_position.p.set(m_Position);
		}

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
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);

		fLifeTime -= Device.fTimeDelta;

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);

		info.fFov = m_fFov;
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

// Решение действовать через консоль чудовищное, в будущем нужно заменить смену флага через консоль на смену через флаг
// общих команд для всех рендеров
void CDemoRecord::SwitchGridState()
{
	if (m_bGridEnabled == false)
	{
		m_bGridEnabled = true;
		Console->Execute("r_photo_grid on");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchGridState - method change m_bGridEnabled to state enabled and activate r_photo_grid");
#endif
	}
	else
	{
		m_bGridEnabled = false;
		Console->Execute("r_photo_grid off");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchGridState - method change m_bGridEnabled to state disabled and deactivate "
			"r_photo_grid");
#endif
	}
}

// Решение действовать через консоль чудовищное, в будущем нужно заменить смену флага через консоль на смену через флаг
// общих команд для всех рендеров
void CDemoRecord::SwitchCinemaBordersState()
{
	if (m_bBordersEnabled == false)
	{
		m_bBordersEnabled = true;
		Console->Execute("r_cinema_borders on");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchCinemaBordersState - method change m_bBordersEnabled to state enabled and activate "
			"r_cinema_borders");
#endif
	}
	else
	{
		m_bBordersEnabled = false;
		Console->Execute("r_cinema_borders off");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchCinemaBordersState - method change m_bBordersEnabled to state disabled and deactivate "
			"r_cinema_borders");
#endif
	}
}

void CDemoRecord::SwitchWatermarkVisibility()
{
	if (m_bWatermarkEnabled == false)
	{
		m_bWatermarkEnabled = true;
		Console->Execute("r_watermark on");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchWatermarkVisibility - method change m_bWatermarkEnabled to state enabled and activate "
			"r_watermark");
#endif
	}
	else
	{
		m_bWatermarkEnabled = false;
		Console->Execute("r_watermark off");
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchWatermarkVisibility - method change m_bWatermarkEnabled to state disabled and "
			"deactivate r_watermark");
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
#ifndef MASTER_GOLD
			g_pGameLevel->CurrentEntity()->ForceTransform(m_Camera);
#endif
			fLifeTime = -1;
		}
	}

	if (dik == DIK_PAUSE)
		Device.Pause(!Device.Paused(), TRUE, TRUE, "demo_record");

#ifndef MASTER_GOLD
#pragma todo("NSDeathman to all: Переделать быструю отладку рендера под удобный вид")
	if (dik == DIK_1)
		Console->Execute("r_debug_render gbuffer_albedo");
	if (dik == DIK_2)
		Console->Execute("r_debug_render gbuffer_position");
	if (dik == DIK_3)
		Console->Execute("r_debug_render gbuffer_normal");
	if (dik == DIK_4)
		Console->Execute("r_debug_render gbuffer_roughness");
	if (dik == DIK_5)
		Console->Execute("r_debug_render gbuffer_matallness");
	if (dik == DIK_6)
		Console->Execute("r_debug_render gbuffer_lightmap_ao");
	if (dik == DIK_7)
		Console->Execute("r_debug_render direct_light");
	if (dik == DIK_8)
		Console->Execute("r_debug_render indirect_light");
	if (dik == DIK_9)
		Console->Execute("r_debug_render real_time_ao");
	if (dik == DIK_0)
		Console->Execute("r_debug_render disabled");
#endif
}

void CDemoRecord::IR_OnKeyboardHold(int dik)
{
	Fvector vT_delta{}, vR_delta{};
	switch (dik)
	{
	case DIK_A:
	case DIK_NUMPAD1:
	case DIK_LEFT:
		vT_delta.x -= 1.0f;
		break; // Slide Left
	case DIK_D:
	case DIK_NUMPAD3:
	case DIK_RIGHT:
		vT_delta.x += 1.0f;
		break; // Slide Right
	case DIK_S:
		vT_delta.y -= 1.0f;
		break; // Slide Down
	case DIK_W:
		vT_delta.y += 1.0f;
		break; // Slide Up
	// rotate
	case DIK_NUMPAD2:
		vR_delta.x -= 1.0f;
		break; // Pitch Down
	case DIK_NUMPAD8:
		vR_delta.x += 1.0f;
		break; // Pitch Up
	case DIK_E:
	case DIK_NUMPAD6:
		vR_delta.y += 1.0f;
		break; // Turn Left
	case DIK_Q:
	case DIK_NUMPAD4:
		vR_delta.y -= 1.0f;
		break; // Turn Right
	case DIK_NUMPAD9:
		vR_delta.z -= 2.0f;
		break; // Turn Right
	case DIK_NUMPAD7:
		vR_delta.z += 2.0f;
		break; // Turn Right
	}
	update_whith_timescale(m_vT, vT_delta);
	update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseMove(int dx, int dy)
{
	float scale = .5f; // psMouseSens;
	Fvector vR_delta{};
	if (dx || dy)
	{
		vR_delta.y += float(dx) * scale;												// heading
		vR_delta.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
	}
	update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseHold(int btn)
{
	Fvector vT_delta{};
	switch (btn)
	{
	case 0:
		vT_delta.z += 1.0f;
		break; // Move Backward
	case 1:
		vT_delta.z -= 1.0f;
		break; // Move Forward
	}
	update_whith_timescale(m_vT, vT_delta);
}

void CDemoRecord::ChangeDepthOfFieldFar(int direction)
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
			Msg("CDemoRecord::ChangeDepthOfFieldFar - far parameter < 5");
#endif
			dof_params_actual.z = 5.0f;
		}

		m_fDOF = dof_params_actual;
		g_pGamePersistent->SetBaseDof(m_fDOF);

#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::ChangeDepthOfFieldFar - method successfully change depth of field parameters");
		Msg("CDemoRecord::ChangeDepthOfFieldFar - depth of field parameters old: near = %d, focus = %d, far = %d",
			dof_params_old.x, dof_params_old.y, dof_params_old.z);
		Msg("CDemoRecord::ChangeDepthOfFieldFar - depth of field parameters actual: near = %d, focus = %d, far = %d",
			dof_params_actual.x, dof_params_actual.y, dof_params_actual.z);
#endif
	}
#ifdef DEBUG_DEMO_RECORD
	else
	{
		Msg("CDemoRecord::ChangeDepthOfFieldFar - method was called before create IGame_Persistent. Going next");
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
		ChangeDepthOfFieldFar(direction);
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
