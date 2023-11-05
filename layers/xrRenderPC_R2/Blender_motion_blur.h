#pragma once

class CBlender_motion_blur : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Motion blur";
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

	CBlender_motion_blur();
	virtual ~CBlender_motion_blur();
};
