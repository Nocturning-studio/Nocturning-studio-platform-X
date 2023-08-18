#pragma once

class CBlender_depth_of_field : public IBlender
{
public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Depth of field";
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

	CBlender_depth_of_field();
	virtual ~CBlender_depth_of_field();
};
