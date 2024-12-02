#pragma once

class CBlender_output_to_screen : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: output to screen";
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

	CBlender_output_to_screen();
	virtual ~CBlender_output_to_screen();
};
