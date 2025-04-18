#pragma once

struct ENGINE_API SBaseEffector
{
	typedef fastdelegate::FastDelegate0<> CB_ON_B_REMOVE;
	CB_ON_B_REMOVE m_on_b_remove_callback;
	virtual ~SBaseEffector()
	{
	}
};

enum ECameraStyle
{
	csCamDebug,
	csFirstEye,
	csLookAt,
	csMax,
	csFixed,
	cs_forcedword = u32(-1)
};

enum ECamEffectorType
{
	cefDemo = 0,
	cefNext
};

struct ENGINE_API SCamEffectorInfo
{
	Fvector p;
	Fvector d;
	Fvector n;
	Fvector r;
	float fFov;
	float fFar;
	float fAspect;
	bool dont_apply;
	bool affected_on_hud;
	ECameraStyle style;
	CObject* parent;
	SCamEffectorInfo();
	SCamEffectorInfo& operator=(const SCamEffectorInfo& other)
	{
		p = other.p;
		d = other.d;
		n = other.n;
		r = other.r;
		fFov = other.fFov;
		fFar = other.fFar;
		fAspect = other.fAspect;
		dont_apply = other.dont_apply;
		affected_on_hud = other.affected_on_hud;
		style = other.style;
		parent = other.parent;
		return *this;
	}
};

enum EEffectorPPType
{
	ppeNext = 0,
};

// refs
class ENGINE_API CCameraBase;
class ENGINE_API CEffectorCam;
class ENGINE_API CEffectorPP;
