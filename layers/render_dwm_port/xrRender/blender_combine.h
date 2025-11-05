#pragma once

class CBlender_combine : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "combine blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }
	
	virtual void Compile(CBlender_Compile& C); 
};