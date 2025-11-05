#pragma once

class CBlender_rain : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "rain blender";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};