#pragma once

#include "inventory_item.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem
{
  private:
	typedef CInventoryItem inherited;

  private:
	CPhysicItem* m_physic_item;

	ref_sound m_use_sound;

  public:
	CEatableItem();
	virtual ~CEatableItem();
	virtual DLL_Pure* _construct();
	virtual CEatableItem* cast_eatable_item()
	{
		return this;
	}

	virtual void Load(LPCSTR section);
	virtual bool Useful() const;

	virtual BOOL net_Spawn(CSE_Abstract* DC);

	virtual void OnH_B_Independent(bool just_before_destroy);
	virtual void UseBy(CEntityAlive* npc);
	bool Empty() const
	{
		return m_iPortionsNum == 0;
	};

  protected:
	// ������� ��� �������� ���� �� ��������� ������
	float m_fHealthInfluence;
	float m_fPowerInfluence;
	float m_fSatietyInfluence;
	float m_fRadiationInfluence;
	float m_fMaxPowerUpInfluence;
	// ���������� ��� �� ���-�� ���������
	float m_fWoundsHealPerc;

	// ���������� ������ ���,
	//-1 - ������ ���� � ������ �� ������ (���� �� �������� ������� � ����)
	int m_iPortionsNum;
	int m_iStartPortionsNum;
};
