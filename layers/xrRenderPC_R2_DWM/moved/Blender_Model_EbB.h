// BlenderDefault.h: interface for the CBlenderDefault class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CBlender_Model_EbB : public IBlender  
{
public:
	string64	oT2_Name;		// name of secondary texture
	string64	oT2_xform;		// xform for secondary texture
	xrP_BOOL	oBlend;
public:
	virtual		LPCSTR		getComment()	{ return "MODEL: env^base";	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }

	virtual		void		Save			(IWriter&	fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Model_EbB();
	virtual ~CBlender_Model_EbB();
};