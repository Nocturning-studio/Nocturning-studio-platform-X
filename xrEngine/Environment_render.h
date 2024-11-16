#pragma once
#include "blenders\blender.h"

class CBlender_skybox : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combiner";
    }
    
	virtual BOOL canBeDetailed()
    {
        return FALSE;
    }

    virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	virtual void Compile(CBlender_Compile& C)
    {
        C.r_Pass("sky2", "sky2", FALSE, TRUE, FALSE);
		C.r_Sampler("s_sky0", "$null", false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
	    C.r_Sampler("s_sky1", "$null", false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
        C.r_Sampler_rtf("s_autoexposure", "$user$autoexposure");
        C.r_End();
    }
};