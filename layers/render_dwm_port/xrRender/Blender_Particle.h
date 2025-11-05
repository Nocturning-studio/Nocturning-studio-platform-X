// Blender_Screen_SET.h: interface for the Blender_Screen_SET class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CBlender_Particle		: public IBlender  
{
	xrP_TOKEN	oBlend;
	xrP_Integer	oAREF;
	xrP_BOOL	oClamp;
public:
	virtual		LPCSTR		getComment()	{ return "particles";	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;			}
	
	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);
	
	virtual		void		Compile			(CBlender_Compile& C);
	
	CBlender_Particle();
	virtual ~CBlender_Particle();
};