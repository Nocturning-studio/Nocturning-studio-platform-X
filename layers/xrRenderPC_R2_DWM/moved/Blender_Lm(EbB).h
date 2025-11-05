// BlenderDefault.h: interface for the CBlenderDefault class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CBlender_LmEbB : public IBlender  
{
public:
	string64	oT2_Name;		// name of secondary texture
	string64	oT2_xform;		// xform for secondary texture
	xrP_BOOL	oBlend;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: lmap*(env^base)";	}
	virtual		BOOL		canBeLMAPped()	{ return TRUE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_LmEbB();
	virtual ~CBlender_LmEbB();
};