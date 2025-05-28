///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_DOWNSAMPLE_PART_1,
	SE_PASS_DOWNSAMPLE_PART_2,
	SE_PASS_SAVE_LUMINANCE,
	SE_PASS_APPLY_EXPOSURE,
	SE_PASS_DUMMY
};
///////////////////////////////////////////////////////////////////////////////////
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

	CBlender_autoexposure() = default;
	~CBlender_autoexposure() = default;
};
///////////////////////////////////////////////////////////////////////////////////
