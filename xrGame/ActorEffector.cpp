﻿#include "stdafx.h"

#include "..\xrEngine\igame_persistent.h"
#include "ActorEffector.h"
#include "PostprocessAnimator.h"
#include "../xrEngine/effectorPP.h"
#include "../xrEngine/ObjectAnimator.h"
#include "object_broker.h"
#include "actor.h"

void AddEffector(CActor* A, int type, const shared_str& sect_name)
{
	if (pSettings->line_exist(sect_name, "pp_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
		CPostprocessAnimator* pp_anm = xr_new<CPostprocessAnimator>();
		pp_anm->SetType((EEffectorPPType)type);
		pp_anm->SetCyclic(bCyclic);

		LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
		pp_anm->Load(fn);
		A->Cameras().AddPPEffector(pp_anm);
	}
	if (pSettings->line_exist(sect_name, "cam_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
		CAnimatorCamEffector* cam_anm = xr_new<CAnimatorCamEffector>();
		cam_anm->SetType((ECamEffectorType)type);
		cam_anm->SetCyclic(bCyclic);
		LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
		cam_anm->Start(fn);
		A->Cameras().AddCamEffector(cam_anm);
	}
}

void AddEffector(CActor* A, int type, const shared_str& sect_name, CEffectorController* ec)
{
	if (pSettings->line_exist(sect_name, "pp_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
		CPostprocessAnimatorControlled* pp_anm = xr_new<CPostprocessAnimatorControlled>(ec);
		pp_anm->SetType((EEffectorPPType)type);
		pp_anm->SetCyclic(bCyclic);
		LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
		pp_anm->Load(fn);
		A->Cameras().AddPPEffector(pp_anm);
	}
	if (pSettings->line_exist(sect_name, "cam_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
		CCameraEffectorControlled* cam_anm = xr_new<CCameraEffectorControlled>(ec);
		cam_anm->SetType((ECamEffectorType)type);
		cam_anm->SetCyclic(bCyclic);
		LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
		cam_anm->Start(fn);
		A->Cameras().AddCamEffector(cam_anm);
	}
}

void AddEffector(CActor* A, int type, const shared_str& sect_name, GET_KOEFF_FUNC k_func)
{
	if (pSettings->line_exist(sect_name, "pp_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
		CPostprocessAnimatorLerp* pp_anm = xr_new<CPostprocessAnimatorLerp>();
		pp_anm->SetType((EEffectorPPType)type);
		pp_anm->SetCyclic(bCyclic);
		LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
		pp_anm->SetFactorFunc(k_func);
		pp_anm->Load(fn);
		A->Cameras().AddPPEffector(pp_anm);
	}
	if (pSettings->line_exist(sect_name, "cam_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
		CAnimatorCamLerpEffector* cam_anm = xr_new<CAnimatorCamLerpEffector>();
		cam_anm->SetFactorFunc(k_func);
		cam_anm->SetType((ECamEffectorType)type);
		cam_anm->SetCyclic(bCyclic);
		LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
		cam_anm->Start(fn);
		A->Cameras().AddCamEffector(cam_anm);
	}
};

void AddEffector(CActor* A, int type, const shared_str& sect_name, float factor)
{
	clamp(factor, 0.001f, 1.5f);
	if (pSettings->line_exist(sect_name, "pp_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
		CPostprocessAnimatorLerpConst* pp_anm = xr_new<CPostprocessAnimatorLerpConst>();
		pp_anm->SetType((EEffectorPPType)type);
		pp_anm->SetCyclic(bCyclic);
		pp_anm->SetPower(factor);
		LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
		pp_anm->Load(fn);
		A->Cameras().AddPPEffector(pp_anm);
	}
	if (pSettings->line_exist(sect_name, "cam_eff_name"))
	{
		bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
		CAnimatorCamLerpEffectorConst* cam_anm = xr_new<CAnimatorCamLerpEffectorConst>();
		cam_anm->SetFactor(factor);
		cam_anm->SetType((ECamEffectorType)type);
		cam_anm->SetCyclic(bCyclic);
		LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
		cam_anm->Start(fn);
		A->Cameras().AddCamEffector(cam_anm);
	}
};

void RemoveEffector(CActor* A, int type)
{
	A->Cameras().RemoveCamEffector((ECamEffectorType)type);
	A->Cameras().RemovePPEffector((EEffectorPPType)type);
}

CEffectorController::~CEffectorController()
{
	R_ASSERT(!m_ce && !m_pe);
}

CAnimatorCamEffector::CAnimatorCamEffector()
{
	m_bCyclic = true;
	m_objectAnimator = xr_new<CObjectAnimator>();
	m_bAbsolutePositioning = false;
}

CAnimatorCamEffector::~CAnimatorCamEffector()
{
	delete_data(m_objectAnimator);
}

void CAnimatorCamEffector::Start(LPCSTR fn)
{
	m_objectAnimator->Load(fn);
	m_objectAnimator->Play(Cyclic());
	fLifeTime = m_objectAnimator->GetLength();
}

BOOL CAnimatorCamEffector::Valid()
{
	if (Cyclic())
		return TRUE;
	return inherited::Valid();
}

BOOL CAnimatorCamEffector::ProcessCam(SCamEffectorInfo& info)
{
	if (!inherited::ProcessCam(info))
		return FALSE;

	const Fmatrix& m = m_objectAnimator->XFORM();
	m_objectAnimator->Update(Device.fTimeDelta);

	if (!m_bAbsolutePositioning)
	{
		Fmatrix Mdef;
		Mdef.identity();
		Mdef.j = info.n;
		Mdef.k = info.d;
		Mdef.i.crossproduct(info.n, info.d);
		Mdef.c = info.p;

		Fmatrix mr;
		mr.mul(Mdef, m);
		info.d = mr.k;
		info.n = mr.j;
		info.p = mr.c;
	}
	else
	{
		info.d = m.k;
		info.n = m.j;
		info.p = m.c;
	};
	return TRUE;
}

BOOL CAnimatorCamLerpEffector::ProcessCam(SCamEffectorInfo& info)
{
	if (!inherited::inherited::ProcessCam(info))
		return FALSE;

	const Fmatrix& m = m_objectAnimator->XFORM();
	m_objectAnimator->Update(Device.fTimeDelta);

	Fmatrix Mdef;
	Mdef.identity();
	Mdef.j = info.n;
	Mdef.k = info.d;
	Mdef.i.crossproduct(info.n, info.d);
	Mdef.c = info.p;

	Fmatrix mr;
	mr.mul(Mdef, m);

	Fquaternion q_src, q_dst, q_res;
	q_src.set(Mdef);
	q_dst.set(mr);

	float t = m_func();
	clamp(t, 0.0f, 1.0f);

	VERIFY(t >= 0.f && t <= 1.f);
	q_res.slerp(q_src, q_dst, t);

	Fmatrix res;
	res.rotation(q_res);
	res.c.lerp(info.p, mr.c, t);

	info.d = res.k;
	info.n = res.j;
	info.p = res.c;

	return TRUE;
}

CAnimatorCamLerpEffectorConst::CAnimatorCamLerpEffectorConst() : m_factor(0.0f)
{
	SetFactorFunc(GET_KOEFF_FUNC(this, &CAnimatorCamLerpEffectorConst::GetFactor));
}

CCameraEffectorControlled::CCameraEffectorControlled(CEffectorController* c) : m_controller(c)
{
	m_controller->SetCam(this);
	SetFactorFunc(GET_KOEFF_FUNC(m_controller, &CEffectorController::GetFactor));
}

CCameraEffectorControlled::~CCameraEffectorControlled()
{
	m_controller->SetCam(NULL);
}

BOOL CCameraEffectorControlled::Valid()
{
	return m_controller->Valid();
}

#define SND_MIN_VOLUME_FACTOR (0.1f)

SndShockEffector::SndShockEffector()
{
	m_snd_length = 0.0f;
	m_cur_length = 0.0f;
	m_stored_volume = -1.0f;
	m_actor = NULL;
}

SndShockEffector::~SndShockEffector()
{
	psSoundVFactor = m_stored_volume;
	if (m_actor && (m_ce || m_pe))
		RemoveEffector(m_actor, effHit);

	R_ASSERT(!m_ce && !m_pe);
}

BOOL SndShockEffector::Valid()
{
	return (m_cur_length <= m_snd_length);
}

BOOL SndShockEffector::InWork()
{
	return inherited::Valid();
}

float SndShockEffector::GetFactor()
{
	float f = (m_end_time - Device.fTimeGlobal) / m_life_time;

	float ff = f * m_life_time / 8.0f;
	return clampr(ff, 0.0f, 1.0f);
}

void SndShockEffector::Start(CActor* A, float snd_length, float power)
{
	clamp(power, 0.1f, 1.5f);
	m_actor = A;
	m_snd_length = snd_length;

	if (m_stored_volume < 0.0f)
		m_stored_volume = psSoundVFactor;

	m_cur_length = 0;
	psSoundVFactor = m_stored_volume * SND_MIN_VOLUME_FACTOR;

	static float xxx = 6.0f / 1.50f; // 6sec on max power(1.5)

	m_life_time = power * xxx;
	m_end_time = Device.fTimeGlobal + m_life_time;

	AddEffector(A, effHit, "snd_shock_effector", this);
}

void SndShockEffector::Update()
{
	m_cur_length += Device.dwTimeDelta;
	float x = float(m_cur_length) / m_snd_length;
	float y = 2.f * x - 1;
	if (y > 0.f)
	{
		psSoundVFactor =
			y * (m_stored_volume - m_stored_volume * SND_MIN_VOLUME_FACTOR) + m_stored_volume * SND_MIN_VOLUME_FACTOR;
	}
}

//////////////////////////////////////////////////////////////////////////

DeathEffector::DeathEffector()
{
	m_snd_length = 0.0f;
	m_cur_length = 0.0f;
	m_stored_volume = -1.0f;
	m_actor = NULL;
}

DeathEffector::~DeathEffector()
{
	psSoundVFactor = m_stored_volume;
	if (m_actor && (m_ce || m_pe))
		RemoveEffector(m_actor, effHit);

	R_ASSERT(!m_ce && !m_pe);
}

BOOL DeathEffector::Valid()
{
	return true;
}

BOOL DeathEffector::InWork()
{
	return inherited::Valid();
}

float DeathEffector::GetFactor()
{
	float f = (m_end_time - Device.fTimeGlobal) / m_life_time;

	float ff = f * m_life_time / 8.0f;
	return clampr(ff, 0.0f, 1.0f);
}

void DeathEffector::Start(CActor* A)
{
	m_actor = A;
	m_snd_length = 100000.0f;

	if (m_stored_volume < 0.0f)
		m_stored_volume = psSoundVFactor;

	m_cur_length = 0;

	m_life_time = 100000.0f;
	m_end_time = Device.fTimeGlobal + m_life_time;
}

void DeathEffector::Update()
{
	bool bMenu = g_pGamePersistent->OnRenderPPUI_query();
	if (!bMenu)
	{
		float FadeOutTime = 5.0f;
		float Delta = Device.dwTimeDelta / FadeOutTime;
		if (psSoundVFactor > Delta)
			psSoundVFactor -= Delta;
		else
			psSoundVFactor = 0.0f;
	}
	else
	{
		psSoundVFactor = m_stored_volume;
	}
}

//////////////////////////////////////////////////////////////////////////

#define ACTOR_EFF_DELTA_ANGLE_X 0.5f * PI / 180
#define ACTOR_EFF_DELTA_ANGLE_Y 0.5f * PI / 180
#define ACTOR_EFF_DELTA_ANGLE_Z 0.5f * PI / 180
#define ACTOR_EFF_ANGLE_SPEED 1.5f

CControllerPsyHitCamEffector::CControllerPsyHitCamEffector(ECamEffectorType type, const Fvector& src_pos,
														   const Fvector& target_pos, float time)
	: inherited(eCEControllerPsyHit, flt_max)
{
	m_time_total = time;
	m_time_current = 0;
	m_dangle_target.set(angle_normalize(Random.randFs(ACTOR_EFF_DELTA_ANGLE_X)),
						angle_normalize(Random.randFs(ACTOR_EFF_DELTA_ANGLE_Y)),
						angle_normalize(Random.randFs(ACTOR_EFF_DELTA_ANGLE_Z)));
	m_dangle_current.set(0.f, 0.f, 0.f);
	m_position_source = src_pos;
	m_direction.sub(target_pos, src_pos);
	m_distance = m_direction.magnitude();
	m_direction.normalize();
}

const float _base_fov = 170.f;
const float _max_fov_add = 160.f;

BOOL CControllerPsyHitCamEffector::ProcessCam(SCamEffectorInfo& info)
{
	Fmatrix Mdef;
	Mdef.identity();
	Mdef.j.set(info.n);
	Mdef.k.set(m_direction);
	Mdef.i.crossproduct(info.n, m_direction);
	Mdef.c.set(info.p);

	//////////////////////////////////////////////////////////////////////////

	if (angle_lerp(m_dangle_current.x, m_dangle_target.x, ACTOR_EFF_ANGLE_SPEED, Device.fTimeDelta))
	{
		m_dangle_target.x = angle_normalize(Random.randFs(ACTOR_EFF_DELTA_ANGLE_X));
	}

	if (angle_lerp(m_dangle_current.y, m_dangle_target.y, ACTOR_EFF_ANGLE_SPEED, Device.fTimeDelta))
	{
		m_dangle_target.y = angle_normalize(Random.randFs(ACTOR_EFF_DELTA_ANGLE_Y));
	}

	if (angle_lerp(m_dangle_current.z, m_dangle_target.z, ACTOR_EFF_ANGLE_SPEED, Device.fTimeDelta))
	{
		m_dangle_target.z = angle_normalize(Random.randFs(ACTOR_EFF_DELTA_ANGLE_Z));
	}

	//////////////////////////////////////////////////////////////////////////

	if (m_time_current > m_time_total)
		m_time_current = m_time_total;

	float perc_past = m_time_current / m_time_total;
	float cur_dist = m_distance * perc_past;

	Mdef.c.mad(m_position_source, m_direction, cur_dist);
	info.fFov = _base_fov - _max_fov_add * perc_past;

	m_time_current += Device.fTimeDelta;

	//////////////////////////////////////////////////////////////////////////

	// Óñòàíîâèòü óãëû ñìåùåíèÿ
	Fmatrix R;
	if (m_time_current > m_time_total)
		R.identity();
	else
		R.setHPB(m_dangle_current.x, m_dangle_current.y, m_dangle_current.z);

	Fmatrix mR;
	mR.mul(Mdef, R);

	info.d.set(mR.k);
	info.n.set(mR.j);
	info.p.set(mR.c);

	return TRUE;
}
bool similar_cam_info(const SCamEffectorInfo& c1, const SCamEffectorInfo& c2)
{
	return (c1.p.similar(c2.p, EPS_L) && c1.d.similar(c2.d, EPS_L) && c1.n.similar(c2.n, EPS_L) &&
			c1.r.similar(c2.r, EPS_L));
}
void CActorCameraManager::UpdateCamEffectors()
{
	m_cam_info_hud = m_cam_info;
	inherited::UpdateCamEffectors();

	m_cam_info_hud.d.normalize();
	m_cam_info_hud.n.normalize();
	m_cam_info_hud.r.crossproduct(m_cam_info_hud.n, m_cam_info_hud.d);
	m_cam_info_hud.n.crossproduct(m_cam_info_hud.d, m_cam_info_hud.r);
}

void cam_effector_sub(const SCamEffectorInfo& c1, const SCamEffectorInfo& c2, SCamEffectorInfo& dest)
{
	dest.p.sub(c1.p, c2.p);
	dest.d.sub(c1.d, c2.d);
	dest.n.sub(c1.n, c2.n);
	dest.r.sub(c1.r, c2.r);
}

void cam_effector_add(const SCamEffectorInfo& diff, SCamEffectorInfo& dest)
{
	dest.p.add(diff.p);
	dest.d.add(diff.d);
	dest.n.add(diff.n);
	dest.r.add(diff.r);
}

bool CActorCameraManager::ProcessCameraEffector(CEffectorCam* eff)
{
	SCamEffectorInfo prev = m_cam_info;

	bool res = inherited::ProcessCameraEffector(eff);
	if (res)
	{
		if (eff->GetHudAffect())
		{
			SCamEffectorInfo affected = m_cam_info;
			SCamEffectorInfo diff;

			cam_effector_sub(affected, prev, diff);

			cam_effector_add(diff, m_cam_info_hud); // m_cam_info_hud += difference
		}

		m_cam_info_hud.fFov = m_cam_info.fFov;
		m_cam_info_hud.fFar = m_cam_info.fFar;
		m_cam_info_hud.fAspect = m_cam_info.fAspect;
	}
	return res;
}
