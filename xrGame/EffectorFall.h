#pragma once

#include "../xrEngine/Effector.h"

// ���������� ����� �������
class CEffectorFall : public CEffectorCam
{
	float fPower;
	float fPhase;

  public:
	virtual BOOL ProcessCam(SCamEffectorInfo& info);

	CEffectorFall(float power, float life_time = 1);
	virtual ~CEffectorFall();
};

class CEffectorDOF : public CEffectorCam
{
	float m_fPhase;

  public:
	CEffectorDOF(const Fvector4& dof);
	virtual BOOL ProcessCam(SCamEffectorInfo& info);
};
