// Blender_Vertex_aref.h: interface for the CBlender_Vertex_aref class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CBlender_Detail_Still : public IBlender  
{
public:
	xrP_BOOL	oBlend;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: detail objects";	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Detail_Still();
	virtual ~CBlender_Detail_Still();
};
