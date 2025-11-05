#pragma once

class CBlender_bloom_build : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "bloom blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }
	
	virtual void Compile(CBlender_Compile& C); 
};