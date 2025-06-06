// WeaponDispersion.cpp: ������ ��� ��������
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "weapon.h"
#include "inventoryowner.h"
#include "actor.h"
#include "inventory_item_impl.h"

#include "actoreffector.h"
#include "effectorshot.h"
#include "EffectorShotX.h"

// ���������� 1, ���� ������ � �������� ��������� � >1 ���� ����������
float CWeapon::GetConditionDispersionFactor() const
{
	return (1.f + fireDispersionConditionFactor * (1.f - GetCondition()));
}

float CWeapon::GetFireDispersion(bool with_cartridge)
{
	if (!with_cartridge)
		return GetFireDispersion(1.0f);
	if (!m_magazine.empty())
		m_fCurrentCartirdgeDisp = m_magazine.back().m_kDisp;
	return GetFireDispersion(m_fCurrentCartirdgeDisp);
}

// ������� ��������� (� ��������) ������ � ������ ������������� �������
float CWeapon::GetFireDispersion(float cartridge_k)
{
	// ���� ������� ���������, ��������� ������ � �������� �������
	float fire_disp;
	if (ParentIsActor() && Actor() && Actor()->IsZoomAimingMode()) // ���� ������ ����������� ��, Actor() �� ������ 0 � ����� ��������� � ������ ������������ (������� Maks0 � Skyloader �� ������)
		fire_disp = zoom_fireDispersionBase; // �� ���������� �������� �������� � ������������
	else											// �����
		fire_disp = fireDispersionBase * cartridge_k * GetConditionDispersionFactor(); // ���������� ������� �������

	if (ParentIsActor() &&
		g_SingleGameDifficulty == egdNovice) // ���� ������ ����������� �� � ������� ��������� ����������
		fire_disp *= 0.25;					 // �� ����� ������� �� 1.5 ����� ��������� ������ ��������
	// ����� ��� ����... NSDeathman

	// ��������� ���������, �������� ����� ��������
	const CInventoryOwner* pOwner = smart_cast<const CInventoryOwner*>(H_Parent());
	VERIFY(pOwner);

	float parent_disp = pOwner->GetWeaponAccuracy();
	fire_disp += parent_disp;

	return fire_disp;
}

//////////////////////////////////////////////////////////////////////////
// ��� ������� ������ ������
void CWeapon::AddShotEffector()
{
	inventory_owner().on_weapon_shot_start(this);
	/**
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if(pActor){
			CCameraShotEffector* S	= smart_cast<CCameraShotEffector*>
	(pActor->EffectorManager().GetEffector(eCEShot)); if (!S)	S				=
	(CCameraShotEffector*)pActor->EffectorManager().AddEffector(xr_new<CCameraShotEffectorX> (camMaxAngle,camRelaxSpeed,
	camMaxAngleHorz, camStepAngleHorz, camDispertionFrac)); R_ASSERT				(S);
			S->SetRndSeed(pActor->GetShotRndSeed());
			S->SetActor(pActor);
			S->Shot					(camDispersion+camDispersionInc*float(ShotsFired()));
		}
	/**/
}

void CWeapon::RemoveShotEffector()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_shot_stop(this);
}

void CWeapon::ClearShotEffector()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_hide(this);
};

/**
const Fvector& CWeapon::GetRecoilDeltaAngle()
{
	CActor* pActor		= smart_cast<CActor*>(H_Parent());

	CCameraShotEffector* S = NULL;
	if(pActor)
		S = smart_cast<CCameraShotEffector*>(pActor->EffectorManager().GetEffector(eCEShot));

	if(S)
	{
		S->GetDeltaAngle(m_vRecoilDeltaAngle);
		return m_vRecoilDeltaAngle;
	}
	else
	{
		m_vRecoilDeltaAngle.set(0.f,0.f,0.f);
		return m_vRecoilDeltaAngle;
	}
}
/**/
