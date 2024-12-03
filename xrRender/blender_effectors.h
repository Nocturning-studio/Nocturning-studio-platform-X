#pragma once

class CBlender_effectors : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: resolve effectors";
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

	CBlender_effectors();
	virtual ~CBlender_effectors();
};
