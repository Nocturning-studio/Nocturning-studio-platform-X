#include "stdafx.h"

void CRenderTarget::phase_rain()
{
    u_setrt(rt_Color);
	if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
	else						u_setzb(HW.pBaseDepthReadWriteDSV); // TODO: Fix me if need
	RImplementation.Target->enable_SSAA();
	RImplementation.rmNormal();
}