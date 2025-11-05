#include "stdafx.h"

#include "render.h"
#include "rendertarget.h"

#include "render_taa.h"

#pragma todo(Fix TAA matrices & backend)

// больше - больше мерцаний, но м€гче сглаживание
//#define TAA_FRAMES_COUNT 8 // 8 or 16
#define TAA_PATTERN_OFFSET 1 // 0.5 to 1.0

CTAA::CTAA()
{
	//M.m_V		= Device.mView;
	//M.m_P		= Device.mProject;
	M.m_VP = Device.mFullTransform;
	//M.m_W		= Fidentity;
	//M.m_invW	= Fidentity;
	//M.m_WV		= M.m_V;
	//M.m_WVP		= M.m_VP;
}

// Set xforms into backend
void CTAA::set_xforms(const Fmatrix& World, const Fmatrix& View, const Fmatrix& Project)
{
	//M.m_V		= View;
	//M.m_P		= Project;
	M.m_VP.mul(Project, View);
	//M.m_W		= World;
	//M.m_invW	.invert_b(World);
	//M.m_WV		.mul_43(View, World);
	//M.m_WVP		.mul(View, M.m_WV);

	//RCache.xforms_taa.m_v		= tM.m_V;
	//RCache.xforms_taa.m_p		= tM.m_P;
	//RCache.xforms_taa.m_vp		= tM.m_VP;
	//RCache.xforms_taa.m_w		= tM.m_W;
	//RCache.xforms_taa.m_invw	= tM.m_invW;
	//RCache.xforms_taa.m_wv		= tM.m_WV;
	//RCache.xforms_taa.m_wvp		= tM.m_WVP;
}

// Set xforms into backend
void CTAA::set_xforms(const taa_matrices& iM)
{
	M = iM;

	//RCache.xforms_taa.m_v		= tM.m_V;
	//RCache.xforms_taa.m_p		= tM.m_P;
	//RCache.xforms_taa.m_vp		= tM.m_VP;
	//RCache.xforms_taa.m_w		= tM.m_W;
	//RCache.xforms_taa.m_invw	= tM.m_invW;
	//RCache.xforms_taa.m_wv		= tM.m_WV;
	//RCache.xforms_taa.m_wvp		= tM.m_WVP;
}

// Get matrices from backend
taa_matrices CTAA::get_xforms()
{
	taa_matrices oM = {
		//RCache.xforms_taa.m_v,
		//RCache.xforms_taa.m_p,
		//RCache.xforms_taa.m_vp,
		//RCache.xforms_taa.m_w,
		//RCache.xforms_taa.m_invw,
		//RCache.xforms_taa.m_wv,
		//RCache.xforms_taa.m_wvp
	};

	oM.m_VP = tM.m_VP; // Hack

	return oM;
}

// Save last matrices from backend
void CTAA::fix_xforms()
{
	tM = M;
}

float CTAA::calc_jitter(const int sequence[], int count, int resolution)
{
	float offset = (float)sequence[Device.dwFrame % count] / (4.0f * resolution);
	return offset * 0.5 * TAA_PATTERN_OFFSET * RImplementation.Target->get_SSAA_params().amount;
}

float CTAA::calc_jitter_x()
{
	static const int sequence_1x[2] = { 0, 8 }; // Quincunx pattern
	static const int sequence_2x[2] = { 4, -4 }; // MSAA 2x pattern
	static const int sequence_4x[4] = { -2, 6, -6, 2 }; // MSAA 4x pattern
	static const int sequence_8x[8] = { 1, -1, 5, -3, -5, -7, 3, 7 }; // MSAA 8x pattern
	static const int sequence_16x[16] = { 1, -1, -3,  4, -5, 2, 5,  3, -2,  0, -4, -6, -8,  7, 6, -7 }; // MSAA 16x pattern

	//int scale = RImplementation.Target->get_SSAA_params().w;
	int scale = Device.dwWidth;

	switch (r__taa_jitter_mode)
	{
	case taa_1x: return calc_jitter(sequence_1x, 2, scale); break;
	case taa_2x: return calc_jitter(sequence_2x, 2, scale); break;
	case taa_4x: return calc_jitter(sequence_4x, 4, scale); break;
	case taa_8x: return calc_jitter(sequence_8x, 8, scale); break;
	case taa_16x: return calc_jitter(sequence_16x, 16, scale); break;
	default: return 0; break;
	}
}

float CTAA::calc_jitter_y()
{
	static const int sequence_1x[2] = { 0, 8 }; // Quincunx pattern
	static const int sequence_2x[2] = { 4, -4 }; // MSAA 2x pattern
	static const int sequence_4x[4] = { -6, -2, 2, 6 }; // MSAA 4x pattern
	static const int sequence_8x[8] = { -3, 3, 1, -5, 5, -1, 7, -7 }; // MSAA 8x pattern
	static const int sequence_16x[16] = { 1, -3,  2, -1, -2, 5, 3, -5,  6, -7, -6,  4,  0, -4, 7, -8 }; // MSAA 16x pattern

	//int scale = RImplementation.Target->get_SSAA_params().h;
	int scale = Device.dwHeight;

	switch (r__taa_jitter_mode)
	{
	case taa_1x: return calc_jitter(sequence_1x, 2, scale); break;
	case taa_2x: return calc_jitter(sequence_2x, 2, scale); break;
	case taa_4x: return calc_jitter(sequence_4x, 4, scale); break;
	case taa_8x: return calc_jitter(sequence_8x, 8, scale); break;
	case taa_16x: return calc_jitter(sequence_16x, 16, scale); break;
	default: return 0; break;
	}
}

CTAA TAA;