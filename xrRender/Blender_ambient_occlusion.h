///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_AO_SSAO,
	SE_AO_SSAO_PLUS,
	SE_AO_HBAO_PLUS,
	SE_AO_GTAO,
};
class CBlender_ambient_occlusion : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Ambient occlusion";
	}

	virtual BOOL canBeDetailed()
	{
		return FALSE;
	}

	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	virtual void Compile(CBlender_Compile& C);

	CBlender_ambient_occlusion();
	virtual ~CBlender_ambient_occlusion();
};
///////////////////////////////////////////////////////////////////////////////////
