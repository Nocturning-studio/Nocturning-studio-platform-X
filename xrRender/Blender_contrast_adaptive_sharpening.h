///////////////////////////////////////////////////////////////////////////////////
//	Created		: 18.12.2023
//	Author		: Deathman
//  Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
class CBlender_contrast_adaptive_sharpening : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Contrast adaptive sharpening";
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

	CBlender_contrast_adaptive_sharpening();
	virtual ~CBlender_contrast_adaptive_sharpening();
};
///////////////////////////////////////////////////////////////////////////////////