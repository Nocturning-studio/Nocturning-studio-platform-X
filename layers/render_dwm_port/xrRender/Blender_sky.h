#pragma once

enum SE_SKY
{
	SE_SKYBOX,
	SE_CLOUDS,
	SE_SKYBOX1,
	SE_CLOUDS1,
};

class CBlender_sky : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "skybox & clouds blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual void Compile(CBlender_Compile& C);
};