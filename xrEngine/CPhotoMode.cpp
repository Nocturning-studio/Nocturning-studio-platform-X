// CPhotoMode.cpp: implementation of the CPhotoMode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "igame_level.h"
#include "x_ray.h"

#include "gamefont.h"
#include "CPhotoMode.h"
#include "xr_ioconsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "render.h"
#include "CustomHUD.h"
#include "IGame_Persistent.h"
//////////////////////////////////////////////////////////////////////
CPhotoMode* xrPhotoMode = NULL;

CPhotoMode::force_position 
CPhotoMode::g_position = 
{
	false, 
	{0.0f, 0.0f, 0.0f}
};

// +X, -X, +Y, -Y, +Z, -Z
Fvector CPhotoMode::cmNorm[6] = 
{
	{0.f, 1.f, 0.f}, 
	{0.f, 1.f, 0.f}, 
	{0.f, 0.f, -1.f},	 
	{0.f, 0.f, 1.f}, 
	{0.f, 1.f, 0.f}, 
	{0.f, 1.f, 0.f}
};

Fvector CPhotoMode::cmDir[6] = 
{
	{1.f, 0.f, 0.f},  
	{-1.f, 0.f, 0.f}, 
	{0.f, 1.f, 0.f},
	{0.f, -1.f, 0.f}, 
	{0.f, 0.f, 1.f},	
	{0.f, 0.f, -1.f}
};

Flags32 CPhotoMode::s_hud_flag = { NULL };
Flags32 CPhotoMode::s_dev_flags = { NULL };
//////////////////////////////////////////////////////////////////////
void CPhotoMode::update_whith_timescale(Fvector& v, const Fvector& v_delta)
{
	VERIFY(!fis_zero(Device.time_factor()));
	float scale = 1.f / Device.time_factor();
	v.mad(v, v_delta, scale);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////s
CPhotoMode::CPhotoMode(float life_time)// : CEffectorCam(cefDemo, life_time)
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
	m_ActorPosition.set(m_Position);

	m_vVelocity.set(0, 0, 0);
	m_vAngularVelocity.set(0, 0, 0);

	m_fGlobalFov = Device.fFOV;
	m_fFov = m_fGlobalFov;

	m_fGlobalTimeFactor = Device.time_factor();
	Device.stop_time();

	g_pGamePersistent->GetCurrentDof(m_vGlobalDepthOfFieldParameters);
	m_fDOF.set(m_vGlobalDepthOfFieldParameters);
	m_fDOF.z = 100.0f;
	g_pGamePersistent->SetBaseDof(m_fDOF);

	g_pGamePersistent->GetDofDiaphragm(m_fGlobalDiaphragm);
	m_fDiaphragm = 2.0f;

	m_bAutofocusEnabled = false;
	m_bGridEnabled = false;
	m_bBordersEnabled = false;
	m_bShowInputInfo = true;
	m_bWatermarkEnabled = false;

	m_bGlobalHudDraw = psHUD_Flags.test(HUD_DRAW);
	psHUD_Flags.set(HUD_DRAW, false);

	m_bGlobalCrosshairDraw = psHUD_Flags.test(HUD_CROSSHAIR);
	psHUD_Flags.set(HUD_CROSSHAIR, false);

	s_hud_flag.assign(psHUD_Flags);
	psHUD_Flags.assign(0);

	m_vT.set(0, 0, 0);
	m_vR.set(0, 0, 0);
	m_bMakeCubeMap = FALSE;
	m_bMakeScreenshot = FALSE;

	m_fSpeed0 = pSettings->r_float("demo_record", "speed0");
	m_fSpeed1 = pSettings->r_float("demo_record", "speed1");
	m_fSpeed2 = pSettings->r_float("demo_record", "speed2");
	m_fSpeed3 = pSettings->r_float("demo_record", "speed3");
	m_fAngSpeed0 = pSettings->r_float("demo_record", "ang_speed0");
	m_fAngSpeed1 = pSettings->r_float("demo_record", "ang_speed1");
	m_fAngSpeed2 = pSettings->r_float("demo_record", "ang_speed2");
	m_fAngSpeed3 = pSettings->r_float("demo_record", "ang_speed3");

	m_Stage = 0;
	m_iLMScreenshotFragment = 0;

	//TODO - replace with UI Sounds
	music.create("photo_mode_dbg_sound", st_Music, sg_Undefined);
	music.play_at_pos(0, Device.vCameraPosition, sm_NoPitch);

	Actor = g_pGameLevel->CurrentViewEntity();
	m_bActorShowState = true;
	Actor->setVisible(TRUE);
}

CPhotoMode::~CPhotoMode()
{
	IR_Release();

	music.stop();
	music.destroy();

	ResetParameters();

	psHUD_Flags.assign(s_hud_flag);

	psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
	psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);

	Device.time_factor(m_fGlobalTimeFactor);
}

void CPhotoMode::ResetParameters()
{
	m_fDOF = m_vGlobalDepthOfFieldParameters;
	g_pGamePersistent->SetBaseDof(m_fDOF);
	g_pGamePersistent->SetPickableEffectorDOF(false);
	g_pGamePersistent->SetDofDiaphragm(m_fGlobalDiaphragm);

	m_fFov = m_fGlobalFov;

	Console->Execute("r_photo_grid off");
	Console->Execute("r_cinema_borders off");
	Console->Execute("r_watermark off");

#ifndef MASTER_GOLD
	Console->Execute("r_debug_render disabled");
#endif
}

void CPhotoMode::MakeScreenshotFace()
{
	switch (m_Stage)
	{
	case 0:
		break;
	case 1:
		Render->Screenshot();
		m_bMakeScreenshot = FALSE;
		break;
	}
	m_Stage++;
}

void CPhotoMode::MakeCubeMapFace(Fvector& D, Fvector& N)
{
	string32 buf;
	switch (m_Stage)
	{
	case 0:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
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
		m_bMakeCubeMap = FALSE;
		break;
	}
	m_Stage++;
}

void CPhotoMode::SwitchShowInputInfo()
{
	if (m_bShowInputInfo == true)
		m_bShowInputInfo = false;
	else
		m_bShowInputInfo = true;
}

void CPhotoMode::ShowInputInfo()
{
	pApp->pFontSystem->SetColor(color_rgba(215, 195, 170, 175));
	pApp->pFontSystem->SetAligment(CGameFont::alCenter);
	pApp->pFontSystem->OutSetI(0, -.22f);

	pApp->pFontSystem->OutNext("%s", "Photo mode");
	pApp->pFontSystem->OutNext("Depth of field focus distance: %f", m_fDOF.z);
	pApp->pFontSystem->OutNext("Depth of field diaphragm size: %f", m_fDiaphragm);
	pApp->pFontSystem->OutNext("Field of view: %f", m_fFov);

	pApp->pFontSystem->SetAligment(CGameFont::alLeft);
	pApp->pFontSystem->OutSetI(-0.2f, +.05f);
	pApp->pFontSystem->OutNext("J");
	pApp->pFontSystem->OutNext("BACK");
	pApp->pFontSystem->OutNext("ESC");
	pApp->pFontSystem->OutNext("F12");
	pApp->pFontSystem->OutNext("G + Mouse Wheel");
	pApp->pFontSystem->OutNext("T + Mouse Wheel");
	pApp->pFontSystem->OutNext("F + Mouse Wheel");
	pApp->pFontSystem->OutNext("H");
	pApp->pFontSystem->OutNext("V");
	pApp->pFontSystem->OutNext("B");
	pApp->pFontSystem->OutNext("P");
	pApp->pFontSystem->OutNext("DELETE");

#ifndef MASTER_GOLD
	pApp->pFontSystem->OutNext("1, 2, ..., 0");
	pApp->pFontSystem->OutNext("ENTER");
#endif

	pApp->pFontSystem->SetAligment(CGameFont::alLeft);
	pApp->pFontSystem->OutSetI(0, +.05f);
	pApp->pFontSystem->OutNext("= Draw this help");
	pApp->pFontSystem->OutNext("= Cube Map");
	pApp->pFontSystem->OutNext("= Quit");
	pApp->pFontSystem->OutNext("= ScreenShot");
	pApp->pFontSystem->OutNext("= Depth of field");
	pApp->pFontSystem->OutNext("= Depth of field diaphragm size");
	pApp->pFontSystem->OutNext("= Field of view");
	pApp->pFontSystem->OutNext("= Autofocus");
	pApp->pFontSystem->OutNext("= Grid");
	pApp->pFontSystem->OutNext("= Cinema borders");
	pApp->pFontSystem->OutNext("= Switch show actor");
	pApp->pFontSystem->OutNext("= Reset parameters");

#ifndef MASTER_GOLD
	pApp->pFontSystem->OutNext("= Render debugging");
	pApp->pFontSystem->OutNext("= Actor teleportation");
#endif
}

BOOL CPhotoMode::ProcessCam(SCamEffectorInfo& info)
{
	if (m_bMakeScreenshot)
	{
		MakeScreenshotFace();

		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);
		info.fFov = m_fFov;
	}
	else if (m_bMakeCubeMap)
	{
		info.fFov = 90.0f;
		info.p.set(m_Camera.c);
		info.fAspect = 1.f;
		MakeCubeMapFace(info.d, info.n);
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

		m_Camera.setHPB(m_HPB.x, m_HPB.y, m_HPB.z);

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

		#pragma todo(NSDeathman to NSDeathman: �������� ����������� ��������� ������ ��� ��������� �������)
		//Fvector CameraToActorDistance;
		//CameraToActorDistance.set(m_Position).sub(m_ActorPosition);

		//Fvector AllowFlyDistance = {20.0f, 20.0f, 20.0f};
		//if (CameraToActorDistance.max(AllowFlyDistance).similar(AllowFlyDistance))
		//{
		//	m_Camera.translate_over(m_Position);
		//}

		m_Camera.translate_over(m_Position);

		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);

		info.fFov = m_fFov;
	}

	return TRUE;
}

void CPhotoMode::SwitchAutofocusState()
{
	if (m_bAutofocusEnabled == false)
		m_bAutofocusEnabled = true;
	else
		m_bAutofocusEnabled = false;

	g_pGamePersistent->SetPickableEffectorDOF(m_bAutofocusEnabled);
}

// ������� ����������� ����� ������� ����������, � ������� ����� �������� ����� ����� ����� ������� �� ����� ����� ����
// ����� ������ ��� ���� ��������
void CPhotoMode::SwitchGridState()
{
	if (m_bGridEnabled == false)
	{
		m_bGridEnabled = true;
		Console->Execute("r_photo_grid on");
	}
	else
	{
		m_bGridEnabled = false;
		Console->Execute("r_photo_grid off");
	}
}

// ������� ����������� ����� ������� ����������, � ������� ����� �������� ����� ����� ����� ������� �� ����� ����� ����
// ����� ������ ��� ���� ��������
void CPhotoMode::SwitchCinemaBordersState()
{
	if (m_bBordersEnabled == false)
	{
		m_bBordersEnabled = true;
		Console->Execute("r_cinema_borders on");
	}
	else
	{
		m_bBordersEnabled = false;
		Console->Execute("r_cinema_borders off");
	}
}

void CPhotoMode::SwitchWatermarkVisibility()
{
	if (m_bWatermarkEnabled == false)
	{
		m_bWatermarkEnabled = true;
		Console->Execute("r_watermark on");
	}
	else
	{
		m_bWatermarkEnabled = false;
		Console->Execute("r_watermark off");
	}
}

void CPhotoMode::SwitchActorVisibility()
{
	if (m_bActorShowState == false)
	{
		m_bActorShowState = true;
		Actor->setVisible(TRUE);
	}
	else
	{
		m_bActorShowState = false;
		Actor->setVisible(FALSE);
	}
}

void CPhotoMode::ChangeDepthOfFieldFar(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	if (direction > 0)
		dof_params_actual.z = dof_params_old.z + 10.0f;
	else
		dof_params_actual.z = dof_params_old.z - 10.0f;

	if (dof_params_actual.z < dof_params_actual.x)
		dof_params_actual.z = dof_params_actual.x + 1.0f;

	if (dof_params_actual.z <= 4.999f)
		dof_params_actual.z = 5.0f;

	m_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(m_fDOF);
}

void CPhotoMode::ChangeDepthOfFieldNear(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	if (direction > 0)
		dof_params_actual.x = dof_params_old.x + 5.0f;
	else
		dof_params_actual.x = dof_params_old.x - 5.0f;

	dof_params_actual.y = dof_params_actual.x + 10.0f;

	//if (dof_params_actual.x <= 0.1f)
	//	dof_params_actual.x = 0.1f;

	m_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(m_fDOF);
}

void CPhotoMode::ChangeDepthOfFieldFocus(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	if (direction > 0)
		dof_params_actual.y = dof_params_old.y + 1.f;
	else
		dof_params_actual.y = dof_params_old.y - 1.0f;

	//if (dof_params_actual.y <= 0.1f)
	//	dof_params_actual.y = 0.1f;

	m_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(m_fDOF);
}

void CPhotoMode::ChangeFieldOfView(int direction)
{
	float m_fFov_actual = Device.fFOV;

	if (direction > 0)
		m_fFov = m_fFov_actual + 0.5f;
	else
		m_fFov = m_fFov_actual - 0.5f;

	if (m_fFov <= 2.28f)
		m_fFov = 2.28f;
	else if (m_fFov >= 113.001f)
		m_fFov = 113.0f;
}

void CPhotoMode::ChangeDiaphragm(int direction)
{
	float m_fDiaphragm_old = 1.0f;
	g_pGamePersistent->GetDofDiaphragm(m_fDiaphragm_old);
	float m_fDiaphragm_actual = m_fDiaphragm_old;

	if (direction > 0)
		m_fDiaphragm_actual = m_fDiaphragm_old + 0.5f;
	else
		m_fDiaphragm_actual = m_fDiaphragm_old - 0.5f;

	if (m_fDiaphragm_actual < 1.0f)
		m_fDiaphragm_actual = 1.0f;
	else if (m_fDiaphragm_actual > 20.0f)
		m_fDiaphragm_actual = 20.0f;

	m_fDiaphragm = m_fDiaphragm_actual;

	g_pGamePersistent->SetDofDiaphragm(m_fDiaphragm);
}

void CPhotoMode::MakeCubemap()
{
	m_bMakeCubeMap = TRUE;
	m_Stage = 0;
}

void CPhotoMode::MakeScreenshot()
{
	m_bMakeScreenshot = TRUE;
	m_Stage = 0;
}

void CPhotoMode::IR_OnKeyboardPress(int dik)
{
	if (dik == DIK_GRAVE)
		Console->Show();

	if (dik == DIK_BACK)
		MakeCubemap();

	if (dik == DIK_F12)
		MakeScreenshot();

	if (dik == DIK_ESCAPE)
		fLifeTime = -1;

	if (dik == DIK_DELETE)
		ResetParameters();

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

	if (dik == DIK_P)
		SwitchActorVisibility();

	if (dik == DIK_RETURN)
	{
#ifndef MASTER_GOLD
			Actor->ForceTransform(m_Camera);
#endif
			fLifeTime = -1;
	}

	//if (dik == DIK_PAUSE)
	//	Device.Pause(!Device.Paused(), TRUE, TRUE, "photo_mode");

#ifndef MASTER_GOLD
#pragma todo("NSDeathman to all: ���������� ������� ������� ������� ��� ������� ���")
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

void CPhotoMode::IR_OnKeyboardHold(int dik)
{
	Fvector vT_delta{}, vR_delta{};
	switch (dik)
	{
	case DIK_A:
	case DIK_NUMPAD1:
		vT_delta.x -= 1.0f;
		break; // Slide Left
	case DIK_D:
	case DIK_NUMPAD3:
		vT_delta.x += 1.0f;
		break; // Slide Right
	case DIK_DOWN:
	case DIK_Q:
		vT_delta.y -= 1.0f;
		break; // Slide Down
	case DIK_UP:
	case DIK_E:
		vT_delta.y += 1.0f;
		break; // Slide Up
	// rotate
	case DIK_NUMPAD2:
		vR_delta.x -= 1.0f;
		break; // Pitch Down
	case DIK_NUMPAD8:
		vR_delta.x += 1.0f;
		break; // Pitch Up
	case DIK_NUMPAD6:
	case DIK_LEFT:
		vR_delta.y += 1.0f;
		break; // Turn Left
	case DIK_NUMPAD4:
	case DIK_RIGHT:
		vR_delta.y -= 1.0f;
		break; // Turn Right
	case DIK_NUMPAD9:
		vR_delta.z -= 2.0f;
		break; // Turn Right
	case DIK_NUMPAD7:
		vR_delta.z += 2.0f;
		break; // Turn Right
	case DIK_W:
		vT_delta.z += 1.0f;
		break; // Move Backward
	case DIK_S:
		vT_delta.z -= 1.0f;
		break; // Move Forward
	}
	update_whith_timescale(m_vT, vT_delta);
	update_whith_timescale(m_vR, vR_delta);
}

void CPhotoMode::IR_OnMouseMove(int dx, int dy)
{
	float scale = .5f; // psMouseSens;
	Fvector vR_delta{};
	if (dx || dy)
	{
		vR_delta.y += float(dx) * scale;													// heading
		vR_delta.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
	}
	update_whith_timescale(m_vR, vR_delta);
}

void CPhotoMode::IR_OnMouseHold(int btn)
{

}

void CPhotoMode::IR_OnMouseWheel(int direction)
{
	if (IR_GetKeyState(DIK_G))
	{
		ChangeDepthOfFieldFar(direction);
	}
	else if (IR_GetKeyState(DIK_U))
	{
		ChangeDepthOfFieldNear(direction);
	}
	else if (IR_GetKeyState(DIK_I))
	{
		ChangeDepthOfFieldFocus(direction);
	}
	else if (IR_GetKeyState(DIK_F))
	{
		ChangeFieldOfView(direction);
	}
	else if (IR_GetKeyState(DIK_T))
	{
		ChangeDiaphragm(direction);
	}
}


