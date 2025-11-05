#pragma once

class CBlender_autoexposure : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: autoexposure estimate";
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

	CBlender_autoexposure();
	virtual ~CBlender_autoexposure();
};
