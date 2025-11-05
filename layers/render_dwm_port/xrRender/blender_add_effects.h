#pragma once

// SSAO
class CBlender_ssao : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "ssao blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual void Compile(CBlender_Compile& C);
};

// Screen Space Sunshafts
class CBlender_sunshafts : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "sunshafts blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual void Compile(CBlender_Compile& C); 
};

// Screen Space Planar Reflections
class CBlender_SSPR : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "planar ssr blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }
	
	virtual void Compile(CBlender_Compile& C); 
};

// Antialiasing
class CBlender_TAA : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "taa blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }
	
	virtual void Compile(CBlender_Compile& C); 
};

class CBlender_AA : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "antialiasing blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }
	
	virtual void Compile(CBlender_Compile& C);
};

// RSM
class CBlender_RSM : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "rain blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual void Compile(CBlender_Compile& C); 
	void PerformRSM(CBlender_Compile& C, LPCSTR ps, LPCSTR vs);
};

// SSAA
enum SE_SSAA
{
	SE_CAS,
	SE_CAS_PORT,
	SE_FSR_PORT_EASU,
	SE_FSR_PORT_EASU_16,
	SE_FSR_PORT_RCAS,
	SE_FSR_PORT_RCAS_16,
};

class CBlender_ssaa : public IBlender 
{
public:
	virtual		LPCSTR		getComment() { return "ssaa blender"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }
	
	virtual void Compile(CBlender_Compile& C); 
};
