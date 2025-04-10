#include "stdafx.h"
#include "zone_effector.h"
#include "level.h"
#include "clsid_game.h"
#include "../xrEngine/xr_object.h"
#include "../xrEngine/cameramanager.h"
#include "actor.h"
#include "actoreffector.h"
#include "postprocessanimator.h"

CZoneEffector::CZoneEffector()
{
	radius = 1;
	m_pp_effector = NULL;
	m_pActor = NULL;
	m_factor = 0.1f;
}

CZoneEffector::~CZoneEffector()
{
	Stop();
}

void CZoneEffector::Load(LPCSTR section)
{
	VERIFY2(pSettings->line_exist(section, "pp_eff_name"), section);
	m_pp_fname = pSettings->r_string(section, "pp_eff_name");
	r_min_perc = pSettings->r_float(section, "radius_min");
	r_max_perc = pSettings->r_float(section, "radius_max");

	if (pSettings->line_exist(section, "radiation_intensity"))
		m_radiation_intensity = pSettings->r_float(section, "radiation_intensity");
	else
		m_radiation_intensity = 0.0f;

	VERIFY(r_min_perc <= r_max_perc);
}

void CZoneEffector::Activate()
{
	m_pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!m_pActor)
		return;
	m_pp_effector = xr_new<CPostprocessAnimatorLerp>();
	m_pp_effector->SetType(EEffectorPPType(u32(u64(this) & u32(-1))));
	m_pp_effector->SetCyclic(true);
	m_pp_effector->SetFactorFunc(GET_KOEFF_FUNC(this, &CZoneEffector::GetFactor));
	m_pp_effector->Load(*m_pp_fname);
	m_pp_effector->SetRadiationIntensity(m_radiation_intensity);
	m_pActor->Cameras().AddPPEffector(m_pp_effector);
}

void CZoneEffector::Stop()
{
	if (!m_pp_effector)
		return;

	m_pActor->Cameras().RemovePPEffector(EEffectorPPType(u32(u64(this) & u32(-1))));
	m_pp_effector = NULL;
	m_pActor = NULL;
};

void CZoneEffector::Update(float dist)
{
	float min_r = radius * r_min_perc;
	float max_r = radius * r_max_perc;

	bool camera_on_actor = (Level().CurrentEntity() && (Level().CurrentEntity()->CLS_ID == CLSID_OBJECT_ACTOR));

	if (m_pp_effector)
	{
		if ((dist > max_r) || !camera_on_actor || (m_pActor && !m_pActor->g_Alive()))
			Stop();
	}
	else
	{
		if ((dist < max_r) && camera_on_actor)
			Activate();
	}

	if (m_pp_effector)
	{
		m_factor = (max_r - dist) / (max_r - min_r);
		clamp(m_factor, 0.01f, 1.0f);
	}
}

float CZoneEffector::GetFactor()
{
	return m_factor;
}

void CZoneEffector::SetRadius(float r)
{
	radius = r;
}
