#pragma once

class CBlender_msaa_mark_edges : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "msaa blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual void Compile(CBlender_Compile& C);
};