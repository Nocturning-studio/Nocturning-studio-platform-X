#pragma once

extern u32 TAA_samples;

class CBlender_TAA : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Temporal antialiasing blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual void Compile(CBlender_Compile& C);

	CBlender_TAA();
	virtual ~CBlender_TAA();
};