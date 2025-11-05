#pragma once

class CBlender_frame_overlay : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Frame overlay";
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

	CBlender_frame_overlay();
	virtual ~CBlender_frame_overlay();
};
