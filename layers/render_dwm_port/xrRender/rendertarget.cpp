#include "stdafx.h"
#include "resourcemanager.h"
#include "dxRenderDeviceRender.h"

#include <D3DX11Core.h>

#include "3DFluidBlenders.h"

#include "Blender_Editor_Selection.h"
#include "Blender_Editor_Wire.h"
#include "Blender_Detail_still.h"
#include "blender_Lm(EbB).h"
#include "blender_Model_EbB.h"
#include "Blender_Screen_SET.h"
#include "Blender_tree.h"
#include "Blender_BmmD.h"
#include "Blender_Particle.h"
#include "blender_deffer_aref.h"
#include "blender_deffer_flat.h"
#include "blender_deffer_model.h"
#include "blender_light_direct.h"
#include "blender_light_mask.h"
#include "blender_light_occq.h"
#include "blender_light_point.h"
#include "blender_light_reflected.h"
#include "blender_light_spot.h"
#include "blender_sky.h"
#include "blender_light_occq.h"
#include "blender_light_mask.h"
#include "blender_light_direct.h"
#include "blender_light_point.h"
#include "blender_light_spot.h"
#include "blender_light_reflected.h"
#include "blender_combine.h"
#include "blender_luminance.h"
#include "blender_bloom_build.h"
#include "blender_sky.h"
#include "blender_add_effects.h"
#include "blender_minmax.h"
#include "blender_rain.h"
#include "blender_msaa.h"

#include "uber_deffer.h"

#pragma warning(disable:4505 4995)

#include "stdintport\stdintport.h"

float AExp2F1(float a)
{
	return std::pow(2.0f, a);
}

#define A_CPU 1
#include "FidelityFX/ffx_a.h"
#include "FidelityFX/ffx_cas.h"
#include "FidelityFX/ffx_fsr1.h"

bool CRenderTarget::need_to_render_sunshafts(r__sunshafts_mode_values type)
{
	if (!(RImplementation.o.sunshafts && r__sunshafts_mode == (u32)type))
		return false;

	{
		CEnvDescriptor& env = *g_pGamePersistent->Environment().CurrentEnv;
		float current = env.m_fSunShaftsIntensity * env.sun_color.x;
		float min = 0.003; // ~ 1.0 / 255.0
		if (current < min) return false;
	}

	return true;
}

bool CRenderTarget::need_to_render_sun_il()
{
	if (!RImplementation.o.sun_il)
		return false;

	{
		CEnvDescriptor& env = *g_pGamePersistent->Environment().CurrentEnv;
		float current = env.sun_color.x;
		float min = 0.003; // ~ 1.0 / 255.0
		if (current < min) return false;
	}

	return true;
}

bool CRenderTarget::need_to_render_spot_il(Fcolor color)
{
	if (!RImplementation.o.spot_il)
		return false;

	{
		float current = color.r + color.g + color.b;
		float min = 0.003; // ~ 1.0 / 255.0
		if (current < min) return false;
	}

	return true;
}

bool CRenderTarget::use_minmax_sm_this_frame()
{
	switch (RImplementation.o.sm_minmax)
	{
	case CRender::MMSM_AUTO:
		return need_to_render_sunshafts(SUNSHAFTS_VOLUME);
	default:
		return false;
	}
}

void CRenderTarget::u_setzb(ID3DDepthStencilView* zb)
{
	RCache.set_ZB(zb);

	if (RCache.get_RT() == NULL)
	{
		ID3DResource* pRes;
		zb->GetResource(&pRes);
		ID3DTexture2D* pTex = (ID3DTexture2D*)pRes;
		D3D_TEXTURE2D_DESC TexDesc;
		pTex->GetDesc(&TexDesc);
		dwWidth = TexDesc.Width;
		dwHeight = TexDesc.Height;
		_RELEASE(pRes);
	}
}

void CRenderTarget::u_setzb(const ref_rt& _1)
{
	if (_1)
	{
		RCache.set_ZB(_1->pZRT);
		dwWidth = _1->dwWidth;
		dwHeight = _1->dwHeight;
	}
	else
	{
		RCache.set_ZB(NULL);
	}
}

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	if (_1)
	{
		dwWidth = _1->dwWidth;
		dwHeight = _1->dwHeight;
	}

	if (_1) RCache.set_RT(_1->pRT, 0); else RCache.set_RT(NULL, 0);
	if (_2) RCache.set_RT(_2->pRT, 1); else RCache.set_RT(NULL, 1);
	if (_3)	RCache.set_RT(_3->pRT, 2); else RCache.set_RT(NULL, 2);
	if (_4)	RCache.set_RT(_4->pRT, 3); else RCache.set_RT(NULL, 3);
}

void CRenderTarget::u_setrt(u32 W, u32 H, ID3DRenderTargetView* _1, ID3DRenderTargetView* _2, ID3DRenderTargetView* _3, ID3DRenderTargetView* _4)
{
	dwWidth = W;
	dwHeight = H;

	RCache.set_RT(_1, 0);
	RCache.set_RT(_2, 1);
	RCache.set_RT(_3, 2);
	RCache.set_RT(_4, 3);
}

void	CRenderTarget::u_stencil_optimize	(eStencilOptimizeMode eSOM)
{
	PIX_EVENT(stencil_optimize);
	//	TODO: DX10: remove half pixel offset?
	VERIFY	(RImplementation.o.nvstencil);
	//RCache.set_ColorWriteEnable	(FALSE);
	u32		Offset;
	float	_w					= RImplementation.fWidth;
	float	_h					= RImplementation.fHeight;
	u32		C					= color_rgba	(255,255,255,255);
	float	eps					= 0;
	float	_dw					= 0.5f;
	float	_dh					= 0.5f;
	FVF::TL* pv					= (FVF::TL*) RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
	pv->set						(-_dw,		_h-_dh,		eps,	1.f, C, 0, 0);	pv++;
	pv->set						(-_dw,		-_dh,		eps,	1.f, C, 0, 0);	pv++;
	pv->set						(_w-_dw,	_h-_dh,		eps,	1.f, C, 0, 0);	pv++;
	pv->set						(_w-_dw,	-_dh,		eps,	1.f, C, 0, 0);	pv++;
	RCache.Vertex.Unlock		(4,g_combine->vb_stride);
	RCache.set_Element			(s_occq->E[1]	);

	switch(eSOM)
	{
	case SO_Light:
		StateManager.SetStencilRef(dwLightMarkerID);
		break;
	case SO_Combine:
		StateManager.SetStencilRef(0x01);
		break;
	default:
		VERIFY(!"CRenderTarget::u_stencil_optimize. switch no default!");
	}	

	RCache.set_Geometry			(g_combine		);
	RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	
}

// 2D texgen (texture adjustment matrix)
void	CRenderTarget::u_compute_texgen_screen	(Fmatrix& m_Texgen)
{
	Fmatrix			m_TexelAdjust		= 
	{
		0.5f,				0.0f,				0.0f,			0.0f,
		0.0f,				-0.5f,				0.0f,			0.0f,
		0.0f,				0.0f,				1.0f,			0.0f,
		0.5f,				0.5f ,				0.0f,			1.0f
	};
	m_Texgen.mul	(m_TexelAdjust,RCache.get_xform_wvp());
}

// 2D texgen for jitter (texture adjustment matrix)
void	CRenderTarget::u_compute_texgen_jitter	(Fmatrix&		m_Texgen_J)
{
	// place into	0..1 space
	Fmatrix			m_TexelAdjust		= 
	{
		0.5f,				0.0f,				0.0f,			0.0f,
		0.0f,				-0.5f,				0.0f,			0.0f,
		0.0f,				0.0f,				1.0f,			0.0f,
		0.5f,				0.5f,				0.0f,			1.0f
	};
	m_Texgen_J.mul	(m_TexelAdjust,RCache.get_xform_wvp());

	// rescale - tile it
	float	scale_X			= RImplementation.fWidth	/ float(TEX_jitter);
	float	scale_Y			= RImplementation.fHeight/ float(TEX_jitter);
	//float	offset			= (.5f / float(TEX_jitter));
	m_TexelAdjust.scale			(scale_X,	scale_Y,1.f	);
	//m_TexelAdjust.translate_over(offset,	offset,	0	);
	m_Texgen_J.mulA_44			(m_TexelAdjust);
}


u8		fpack			(float v)				{
	s32	_v	= iFloor	(((v+1)*.5f)*255.f + .5f);
	clamp	(_v,0,255);
	return	u8(_v);
}
u8		fpackZ			(float v)				{
	s32	_v	= iFloor	(_abs(v)*255.f + .5f);
	clamp	(_v,0,255);
	return	u8(_v);
}
Fvector	vunpack			(s32 x, s32 y, s32 z)	{
	Fvector	pck;
	pck.x	= (float(x)/255.f - .5f)*2.f;
	pck.y	= (float(y)/255.f - .5f)*2.f;
	pck.z	= -float(z)/255.f;
	return	pck;
}
Fvector	vunpack			(Ivector src)			{
	return	vunpack	(src.x,src.y,src.z);
}
Ivector	vpack			(Fvector src)
{
	Fvector			_v;
	int	bx			= fpack	(src.x);
	int by			= fpack	(src.y);
	int bz			= fpackZ(src.z);
	// dumb test
	float	e_best	= flt_max;
	int		r=bx,g=by,b=bz;
#ifdef DEBUG
	int		d=0;
#else
	int		d=3;
#endif
	for (int x=_max(bx-d,0); x<=_min(bx+d,255); x++)
	for (int y=_max(by-d,0); y<=_min(by+d,255); y++)
	for (int z=_max(bz-d,0); z<=_min(bz+d,255); z++)
	{
		_v				= vunpack(x,y,z);
		float	m		= _v.magnitude();
		float	me		= _abs(m-1.f);
		if	(me>0.03f)	continue;
		_v.div	(m);
		float	e		= _abs(src.dotproduct(_v)-1.f);
		if (e<e_best)	{
			e_best		= e;
			r=x,g=y,b=z;
		}
	}
	Ivector		ipck;
	ipck.set	(r,g,b);
	return		ipck;
}

void	generate_jitter	(DWORD*	dest, u32 elem_count)
{
	const	int		cmax		= 8;
	svector<Ivector2,cmax>		samples;
	while (samples.size()<elem_count*2)
	{
		Ivector2	test;
		test.set	(::Random.randI(0,256),::Random.randI(0,256));
		BOOL		valid = TRUE;
		for (u32 t=0; t<samples.size(); t++)
		{
			int		dist	= _abs(test.x-samples[t].x)+_abs(test.y-samples[t].y);
			if (dist<32)	{
				valid		= FALSE;
				break;
			}
		}
		if (valid)	samples.push_back	(test);
	}
	for	(u32 it=0; it<elem_count; it++, dest++)
		*dest	= color_rgba(samples[2*it].x,samples[2*it].y,samples[2*it+1].y,samples[2*it+1].x);
}

CRenderTarget::CRenderTarget()
{
	dwWidth = Device.dwWidth;
	dwHeight = Device.dwHeight;

	param_blur = 0.f;
	param_gray = 0.f;
	param_noise = 0.f;
	param_duality_h = 0.f;
	param_duality_v = 0.f;
	param_noise_fps = 25.f;
	param_noise_scale = 1.f;

	im_noise_time = 1 / 100;
	im_noise_shift_w = 0;
	im_noise_shift_h = 0;

	param_color_base = color_rgba(127, 127, 127, 0);
	param_color_gray = color_rgba(85, 85, 85, 0);
	param_color_add.set(0.0f, 0.0f, 0.0f);

	CRenderTargetDefferedCreate();
}

CRenderTarget::~CRenderTarget()
{
	CRenderTargetDefferedDelete();
}

void CRenderTarget::build_textures()
{

	// Testure for async sreenshots
	{
		D3D_TEXTURE2D_DESC	desc;
		desc.Width = Device.dwWidth;
		desc.Height = Device.dwHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
		desc.Usage = D3D_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D_CPU_ACCESS_READ;
		desc.MiscFlags = 0;

		R_CHK(HW.pDevice->CreateTexture2D(&desc, 0, &t_ss_async));
	}

	// Build material(s)
	{

		//	Create immutable texture. 
		u16	tempData[TEX_material_LdotN * TEX_material_LdotH * TEX_material_Count];

		D3D_TEXTURE3D_DESC	desc;
		desc.Width = TEX_material_LdotN;
		desc.Height = TEX_material_LdotH;
		desc.Depth = TEX_material_Count;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8_UNORM;
		desc.Usage = D3D_USAGE_IMMUTABLE;
		desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D_SUBRESOURCE_DATA	subData;

		subData.pSysMem = tempData;
		subData.SysMemPitch = desc.Width * 2;
		subData.SysMemSlicePitch = desc.Height * subData.SysMemPitch;


		for (u32 slice = 0; slice < TEX_material_Count; slice++)
		{
			for (u32 y = 0; y < TEX_material_LdotH; y++)
			{
				for (u32 x = 0; x < TEX_material_LdotN; x++)
				{

					u16* p = (u16*)(LPBYTE(subData.pSysMem)	+ slice * subData.SysMemSlicePitch + y * subData.SysMemPitch + x * 2);

					float	ld = float(x) / float(TEX_material_LdotN - 1);
					float	ls = float(y) / float(TEX_material_LdotH - 1) + EPS_S;
					ls *= powf(ld, 1 / 32.f);
					float	fd, fs;

					switch (slice)
					{
					case 0: { // looks like OrenNayar
						fd = powf(ld, 0.75f);		// 0.75
						fs = powf(ls, 16.f) * .5f;
					}	break;
					case 1: {// looks like Blinn
						fd = powf(ld, 0.90f);		// 0.90
						fs = powf(ls, 24.f);
					}	break;
					case 2: { // looks like Phong
						fd = ld;					// 1.0
						fs = powf(ls * 1.01f, 128.f);
					}	break;
					case 3: { // looks like Metal
						float	s0 = _abs(1 - _abs(0.05f * _sin(33.f * ld) + ld - ls));
						float	s1 = _abs(1 - _abs(0.05f * _cos(33.f * ld * ls) + ld - ls));
						float	s2 = _abs(1 - _abs(ld - ls));
						fd = ld;				// 1.0
						fs = powf(_max(_max(s0, s1), s2), 24.f);
						fs *= powf(ld, 1 / 7.f);
					}	break;
					default:
						fd = fs = 0;
					}
					s32		_d = clampr(iFloor(fd * 255.5f), 0, 255);
					s32		_s = clampr(iFloor(fs * 255.5f), 0, 255);
					if ((y == (TEX_material_LdotH - 1)) && (x == (TEX_material_LdotN - 1))) { _d = 255; _s = 255; }
					*p = u16(_s * 256 + _d);
				}
			}
		}

		R_CHK(HW.pDevice->CreateTexture3D(&desc, &subData, &t_material_surf));
		t_material = dxRenderDeviceRender::Instance().Resources->_CreateTexture(tex_t_material);
		t_material->surface_set(t_material_surf);

	}

	// Build noise table
	{

		static const int sampleSize = 4;
		u32	tempData[TEX_jitter_count][TEX_jitter * TEX_jitter];

		D3D_TEXTURE2D_DESC	desc;
		desc.Width = TEX_jitter;
		desc.Height = TEX_jitter;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
		//desc.Usage = D3D_USAGE_IMMUTABLE;
		desc.Usage = D3D_USAGE_DEFAULT;
		desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D_SUBRESOURCE_DATA	subData[TEX_jitter_count];

		for (int it = 0; it < TEX_jitter_count - 1; it++)
		{
			subData[it].pSysMem = tempData[it];
			subData[it].SysMemPitch = desc.Width * sampleSize;
		}

		// Fill it,
		for (u32 y = 0; y < TEX_jitter; y++)
		{
			for (u32 x = 0; x < TEX_jitter; x++)
			{
				DWORD	data[TEX_jitter_count - 1];
				generate_jitter(data, TEX_jitter_count - 1);
				for (u32 it = 0; it < TEX_jitter_count - 1; it++)
				{
					u32* p = (u32*)
						(LPBYTE(subData[it].pSysMem)
							+ y * subData[it].SysMemPitch
							+ x * 4);

					*p = data[it];
				}
			}
		}

		for (int it = 0; it < TEX_jitter_count - 1; it++)
		{
			string_path					name;
			xr_sprintf(name, "%s%d", tex_t_noise_, it);
			R_CHK(HW.pDevice->CreateTexture2D(&desc, &subData[it], &t_noise_surf[it]));
			t_noise[it] = dxRenderDeviceRender::Instance().Resources->_CreateTexture(name);
			t_noise[it]->surface_set(t_noise_surf[it]);
		}

		float tempDataHBAO[TEX_jitter * TEX_jitter * 4];

		// generate HBAO jitter texture (last)
		D3D_TEXTURE2D_DESC	descHBAO;
		descHBAO.Width = TEX_jitter;
		descHBAO.Height = TEX_jitter;
		descHBAO.MipLevels = 1;
		descHBAO.ArraySize = 1;
		descHBAO.SampleDesc.Count = 1;
		descHBAO.SampleDesc.Quality = 0;
		descHBAO.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		//desc.Usage = D3D_USAGE_IMMUTABLE;
		descHBAO.Usage = D3D_USAGE_DEFAULT;
		descHBAO.BindFlags = D3D_BIND_SHADER_RESOURCE;
		descHBAO.CPUAccessFlags = 0;
		descHBAO.MiscFlags = 0;

		it = TEX_jitter_count - 1;
		subData[it].pSysMem = tempDataHBAO;
		subData[it].SysMemPitch = descHBAO.Width * sampleSize * sizeof(float);

		// Fill it,
		for (u32 y = 0; y < TEX_jitter; y++)
		{
			for (u32 x = 0; x < TEX_jitter; x++)
			{
				float numDir = 8.0f;
				float angle = 2 * PI * ::Random.randF(0.0f, 1.0f) / numDir;
				float dist = ::Random.randF(0.0f, 1.0f);

				float* p = (float*)
					(LPBYTE(subData[it].pSysMem)
						+ y * subData[it].SysMemPitch
						+ x * 4 * sizeof(float));
				*p = (float)(_cos(angle));
				*(p + 1) = (float)(_sin(angle));
				*(p + 2) = (float)(dist);
				*(p + 3) = 0;
			}
		}

		string_path					name;
		xr_sprintf(name, "%s%d", tex_t_noise_, it);
		R_CHK(HW.pDevice->CreateTexture2D(&descHBAO, &subData[it], &t_noise_surf[it]));
		t_noise[it] = dxRenderDeviceRender::Instance().Resources->_CreateTexture(name);
		t_noise[it]->surface_set(t_noise_surf[it]);


		//	Create noise mipped
		{
			//	Autogen mipmaps
			desc.MipLevels = 0;
			R_CHK(HW.pDevice->CreateTexture2D(&desc, 0, &t_noise_surf_mipped));
			t_noise_mipped = dxRenderDeviceRender::Instance().Resources->_CreateTexture(tex_t_noise_mipped_);
			t_noise_mipped->surface_set(t_noise_surf_mipped);

			//	Update texture. Generate mips.

			HW.pContext->CopySubresourceRegion(t_noise_surf_mipped, 0, 0, 0, 0, t_noise_surf[0], 0, 0);

			D3DX11FilterTexture(HW.pContext, t_noise_surf_mipped, 0, D3DX11_FILTER_POINT);
		}

	}
}

void CRenderTarget::destroy_textures()
{

	_RELEASE(t_ss_async);


	// Textures
	t_material->surface_set(NULL);

#ifdef DEBUG
	_SHOW_REF("t_material_surf", t_material_surf);
#endif // DEBUG

	_RELEASE(t_material_surf);
}

//----------------------
// Deffered mode

void CRenderTarget::CRenderTargetDefferedCreate()
{
	dwAccumulatorClearMark			= 0;
	dxRenderDeviceRender::Instance().Resources->Evict			();

	u32 s = RImplementation.o.msaa_samples;

	u32 w0 = Device.dwWidth;
	u32 h0 = Device.dwHeight;
	
	// scaled screen size
	u32 w = w0;
	u32 h = h0;
	
	// Blenders
	b_occq				= xr_new<CBlender_light_occq>();
	b_accum_mask		= xr_new<CBlender_accum_direct_mask>();
	b_accum_direct		= xr_new<CBlender_accum_direct>();
	b_accum_point		= xr_new<CBlender_accum_point>();
	b_accum_spot		= xr_new<CBlender_accum_spot>();
	b_accum_reflected	= xr_new<CBlender_accum_reflected>();
	b_combine			= xr_new<CBlender_combine>();
	b_luminance			= xr_new<CBlender_luminance>();
	b_bloom				= xr_new<CBlender_bloom_build>();
	b_ssao				= xr_new<CBlender_ssao>();
	b_sunshafts			= xr_new<CBlender_sunshafts>();
	b_antialiasing		= xr_new<CBlender_AA>();
	b_taa				= xr_new<CBlender_TAA>();

	// Depth buffer
	{
		t_depth.create(tex_t_depth);
		t_depth->surface_set(HW.pBaseDepthSurface);
	}

	SSAA_create();
	w = SSAA.w;
	h = SSAA.h;

	if(RImplementation.o.txaa || RImplementation.o.aa_mode == AA_TAA)
		TXAA_rt_create(s, w, h);

	if (RImplementation.o.aa_mode == AA_TAA_V2)
	{
		for (u32 i = 0; i < TAA_FEEDBACK_SIZE; i++)
		{
			rt_TAA[i].create(rt_TAA_params[i].texture_name, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);
			m_TAA[i] = Fidentity;
		}
	}

	if (RImplementation.o.aa_mode == AA_MSAA)
	{
		b_msaa_mark_edges = xr_new<CBlender_msaa_mark_edges>();
		s_msaa_mark_edges.create(b_msaa_mark_edges);

		rt_Generic_1_ms	.create(tex_rt_Generic_1_ms,	w, h, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV, s);
		rt_Generic_0_ms	.create(tex_rt_Generic_0_ms,	w, h, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV, s);
		rt_Color_ms		.create(tex_rt_Color_ms,		w, h, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV, s);
		rt_MSAA_depth	.create(tex_t_msaa_depth,		w, h, DXGI_FORMAT_D24_UNORM_S8_UINT,	SRV_DSV, s);
	}

	//	NORMAL
	{
		rt_Position		.create(tex_rt_Position,	w,	h,	DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV, s);
		rt_Color		.create(tex_rt_Color,		w,	h,	DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV); // full resolution

		// 64bit + 32bit
		if (RImplementation.o.cspecular && RImplementation.o.wet_surfaces)
		{
			rt_Accumulator	.create(tex_rt_Accumulator,		w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV, s);
			rt_Accumulator_1.create(tex_rt_Accumulator_1,	w, h, DXGI_FORMAT_R11G11B10_FLOAT, SRV_RTV, s);
		}
		// 32bit + 32bit
		else if(RImplementation.o.cspecular)
		{
			rt_Accumulator	.create(tex_rt_Accumulator,		w, h, DXGI_FORMAT_R11G11B10_FLOAT, SRV_RTV, s);
			rt_Accumulator_1.create(tex_rt_Accumulator_1,	w, h, DXGI_FORMAT_R11G11B10_FLOAT, SRV_RTV, s);
		}
		// 64 bit
		else
		{
			rt_Accumulator.create(tex_rt_Accumulator, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV, s);
		}

		rt_Generic_0.create(tex_rt_Generic_0, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV); // generic(LDR) RTs
		rt_Generic_1.create(tex_rt_Generic_1, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV); // generic(LDR) RTs

		if (RImplementation.o.gbd_save)
			rt_Position_prev.create(tex_rt_Position_prev, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV, s);
	}

	// Planar true reflections
	if (RImplementation.o.planar)
	{
		u32 s1 = RImplementation.o.msaa_samples_reflections;

		u32 w1 = w;
		u32 h1 = h;

		// MSAA full enabled
		if (s1 > 1 && s1 == s)
		{
			rt_Planar_color_ms	.create(tex_rt_Planar_color_ms,	w1, h1, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV, s1	); // rendered reflections
			rt_Planar_depth		.create(tex_rt_Planar_depth,	w1, h1, DXGI_FORMAT_D24_UNORM_S8_UINT,	SRV_DSV, s1	);
		}
		// MSAA enabled
		else if (s1 > 1)
		{
			rt_Planar_color		.create(tex_rt_Planar_color,	w1, h1, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV		); // resolved MSAA reflections
			rt_Planar_color_ms	.create(tex_rt_Planar_color_ms,	w1, h1, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV, s1	); // rendered reflections
			rt_Planar_depth		.create(tex_rt_Planar_depth,	w1, h1, DXGI_FORMAT_D24_UNORM_S8_UINT,	SRV_DSV, s1	);
		}
		// MSAA disabled
		else
		{
			rt_Planar_color.create(tex_rt_Planar_color, w1, h1, DXGI_FORMAT_R8G8B8A8_UNORM,		SRV_RTV); // rendered reflections
			rt_Planar_depth.create(tex_rt_Planar_depth, w1, h1, DXGI_FORMAT_D24_UNORM_S8_UINT,	SRV_DSV);
		}

		if(HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1)
			rt_Planar_shadow.create(tex_rt_Planar_shadow, RImplementation.o.smapsize, RImplementation.o.smapsize, (DXGI_FORMAT)RImplementation.o.smap_format, SRV_DSV);
	}

	if (RImplementation.o.advanced_mode)
	{
		rt_Generic.create(tex_rt_Generic, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV); // (temp)
		rt_PPTemp.create(tex_rt_PPTemp, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV); // (temp)

		//	Igor: for volumetric lights
		if (RImplementation.o.volumetriclight || RImplementation.o.sunshafts)
			rt_Generic_2.create(tex_rt_Generic_2, w, h, DXGI_FORMAT_R11G11B10_FLOAT, SRV_RTV, s);

		if (RImplementation.o.ssao == SSAO_PATH_TRACING)
		{
			rt_SSAO.create(tex_rt_SSAO, w, h, DXGI_FORMAT_R16_FLOAT, SRV_RTV);
			rt_SSAO_temp.create(tex_rt_SSAO_temp, w, h, DXGI_FORMAT_R16_FLOAT, SRV_RTV);
			rt_SSAO_prev.create(tex_rt_SSAO_prev, w, h, DXGI_FORMAT_R16_FLOAT, SRV_RTV);

			if (RImplementation.o.pt_downsample)
				rt_SSAO_small.create(tex_rt_SSAO_small, w / 2, h / 2, DXGI_FORMAT_R16_FLOAT, SRV_RTV);
		}
		else if (RImplementation.o.ssao == SSAO_HBAO_PLUS)
		{
			rt_HBAO_plus_normal.create(tex_rt_HBAO_plus_normal, w, h, DXGI_FORMAT_R8G8B8A8_SNORM, SRV_RTV, s);
		}

		if (RImplementation.o.ssr_replace)
		{
			b_sspr = xr_new<CBlender_SSPR>();
			s_sspr.create(b_sspr);
			rt_SSPR.create(tex_rt_SSPR, w, h, DXGI_FORMAT_R32_UINT, SRV_RTV_UAV);
			rt_Depth_1.create(tex_rt_Depth_1, w, h, DXGI_FORMAT_R16_FLOAT, SRV_RTV, s);
		}

		// Create AA blender
		if (RImplementation.o.aa_mode == AA_MLAA)
		{
			rt_MLAA_0.create(tex_rt_MLAA_0, w, h, DXGI_FORMAT_R8_UINT, SRV_RTV);
			rt_MLAA_1.create(tex_rt_MLAA_1, w, h, DXGI_FORMAT_R8G8_UINT, SRV_RTV);
		}
	}

	s_antialiasing.create(b_antialiasing);
	s_taa.create(b_taa);
	s_ssao.create(b_ssao);
	s_sunshafts.create(b_sunshafts);

	if (RImplementation.o.sm_minmax)
	{
		rt_smap_depth_minmax.create(tex_rt_smap_depth_minmax, RImplementation.o.smapsize / 4, RImplementation.o.smapsize / 4, DXGI_FORMAT_D32_FLOAT, SRV_DSV);
		CBlender_createminmax TempBlender;
		s_create_minmax_sm.create(&TempBlender);
	}

	if (RImplementation.o.sun_il || RImplementation.o.spot_il)
	{
		b_rsm = xr_new<CBlender_RSM>();
		s_rsm.create(b_rsm);

		// light RTs
		rt_Normal_IL.create(tex_rt_Normal_IL, RImplementation.o.smapsize, RImplementation.o.smapsize, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		rt_Position_IL.create(tex_rt_Position_IL, RImplementation.o.smapsize, RImplementation.o.smapsize, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		rt_Color_IL.create(tex_rt_Color_IL, RImplementation.o.smapsize, RImplementation.o.smapsize, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);

		// screen RTs
		rt_RSM.create(tex_rt_RSM, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		rt_RSM_copy.create(tex_rt_RSM_copy, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		rt_RSM_prev.create(tex_rt_RSM_prev, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		rt_RSM_depth.create(tex_rt_RSM_depth, w, h, DXGI_FORMAT_R16_FLOAT, SRV_RTV);
		//rt_RSM_depth_prev.create(tex_rt_RSM_depth_prev, w, h, DXGI_FORMAT_R16_FLOAT);
	}

	//	RAIN
	//	TODO: DX10: Create resources only when DX10 rain is enabled.
	//	Or make DX10 rain switch dynamic?
	{
		CBlender_rain TempBlender;
		s_rain.create(&TempBlender, "null");
	}

	// BLOOM
	{
		u32 fvf_build = D3DFVF_XYZRHW | D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE2(1) | D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE2(3);
		u32 fvf_filter = (u32)D3DFVF_XYZRHW | D3DFVF_TEX8 | D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEXCOORDSIZE4(1) | D3DFVF_TEXCOORDSIZE4(2) | D3DFVF_TEXCOORDSIZE4(3) | D3DFVF_TEXCOORDSIZE4(4) | D3DFVF_TEXCOORDSIZE4(5) | D3DFVF_TEXCOORDSIZE4(6) | D3DFVF_TEXCOORDSIZE4(7);
		
		rt_Bloom_1.create(tex_rt_Bloom_1, BLOOM_size_X, BLOOM_size_Y, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);
		rt_Bloom_2.create(tex_rt_Bloom_2, BLOOM_size_X, BLOOM_size_Y, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);
		
		g_bloom_build.create(fvf_build, RCache.Vertex.Buffer(), RCache.QuadIB);
		g_bloom_filter.create(fvf_filter, RCache.Vertex.Buffer(), RCache.QuadIB);

		s_bloom.create(b_bloom);

		f_bloom_factor = 0.5f;
	}

	// TONEMAP
	{
		rt_LUM_64.create(tex_rt_LUM_64, 64, 64, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		rt_LUM_8.create(tex_rt_LUM_8, 8, 8, DXGI_FORMAT_R16G16B16A16_FLOAT, SRV_RTV);
		s_luminance.create(b_luminance);

		f_luminance_adapt = 0.5f;

		t_LUM_src.create(tex_t_LUM_src);
		t_LUM_dest.create(tex_t_LUM_dest);

		// create pool
		for (u32 it = 0; it < HW.Caps.iGPUNum * 2; it++)
		{
			string256 name;
			xr_sprintf(name, "%s_%d", tex_rt_LUM_pool, it);
			rt_LUM_pool[it].create(name, 1, 1, DXGI_FORMAT_R32_FLOAT, SRV_RTV);
			FLOAT rgba[4] = { 127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f, 127.0f / 255.0f };
			RCache.clear_RenderTargetView(rt_LUM_pool[it]->pRT, rgba);
		}
	}

	// OCCLUSION
	s_occq.create(b_occq);

	// Create simple screen quad geom for postprocess shaders
	g_simple_quad.create(FVF::F_V, RCache.Vertex.Buffer(), RCache.QuadIB);

	if (RImplementation.o.vsm)
	{
		rt_vsm_depth.create(tex_rt_vsm_depth, RImplementation.o.smapsize, RImplementation.o.smapsize, DXGI_FORMAT_R32G32_FLOAT, SRV_RTV);
		//rt_vsm_depthms.create(tex_rt_vsm_depth, RImplementation.o.smapsize, RImplementation.o.smapsize, DXGI_FORMAT_R32G32_FLOAT, SRV_RTV, 8);
	}

	// DIRECT (spot)
	{
		rt_smap_depth.create(tex_rt_smap_depth, RImplementation.o.smapsize, RImplementation.o.smapsize, (DXGI_FORMAT)RImplementation.o.smap_format, SRV_DSV);

		s_accum_mask.create(b_accum_mask);
		s_accum_direct.create(b_accum_direct);
	}

	// POINT
	{
		s_accum_point.create		(b_accum_point,				"r2\\accum_point_s");
		accum_point_geom_create		();
		g_accum_point.create		(D3DFVF_XYZ,				g_accum_point_vb, g_accum_point_ib);
		accum_omnip_geom_create		();
		g_accum_omnipart.create		(D3DFVF_XYZ,				g_accum_omnip_vb, g_accum_omnip_ib);
	}

	// SPOT
	{
		s_accum_spot.create			(b_accum_spot,				"r2\\accum_spot_s",	"lights\\lights_spot01");
		accum_spot_geom_create		();
		g_accum_spot.create			(D3DFVF_XYZ,				g_accum_spot_vb, g_accum_spot_ib);
	}

	{
		s_accum_volume.create("accum_volumetric", "lights\\lights_spot01");
		accum_volumetric_geom_create();
		g_accum_volumetric.create( D3DFVF_XYZ, g_accum_volumetric_vb, g_accum_volumetric_ib);
	}

	// REFLECTED
	{
		s_accum_reflected.create	(b_accum_reflected,			"r2\\accum_refl");
	}

	// COMBINE
	{
		static D3DVERTEXELEMENT9 dwDecl[] =
		{
			{ 0, 0,  D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT, 	D3DDECLUSAGE_POSITION,	0 },	// pos+uv
			D3DDECL_END()
		};
		s_combine.create					(b_combine,					"r2\\combine");
		//s_combine_volumetric.create			("combine_volumetric");
		s_combine_dbg_1.create				("effects\\screen_set", tex_rt_LUM_8);
		s_combine_dbg_Accumulator.create	("effects\\screen_set", tex_rt_Accumulator);
		g_combine_VP.create					(dwDecl,		RCache.Vertex.Buffer(), RCache.QuadIB);
		g_combine.create					(FVF::F_TL,		RCache.Vertex.Buffer(), RCache.QuadIB);
		g_combine_2UV.create				(FVF::F_TL2uv,	RCache.Vertex.Buffer(), RCache.QuadIB);
		g_combine_cuboid.create				(dwDecl,	RCache.Vertex.Buffer(), RCache.Index.Buffer());

		u32 fvf_aa_blur				= D3DFVF_XYZRHW|D3DFVF_TEX4|D3DFVF_TEXCOORDSIZE2(0)|D3DFVF_TEXCOORDSIZE2(1)|D3DFVF_TEXCOORDSIZE2(2)|D3DFVF_TEXCOORDSIZE2(3);
		g_aa_blur.create			(fvf_aa_blur,	RCache.Vertex.Buffer(), RCache.QuadIB);

		u32 fvf_aa_AA				= D3DFVF_XYZRHW|D3DFVF_TEX7|D3DFVF_TEXCOORDSIZE2(0)|D3DFVF_TEXCOORDSIZE2(1)|D3DFVF_TEXCOORDSIZE2(2)|D3DFVF_TEXCOORDSIZE2(3)|D3DFVF_TEXCOORDSIZE2(4)|D3DFVF_TEXCOORDSIZE4(5)|D3DFVF_TEXCOORDSIZE4(6);
		g_aa_AA.create				(fvf_aa_AA,		RCache.Vertex.Buffer(), RCache.QuadIB);

		t_envmap_0.create			(tex_t_envmap_0);
		t_envmap_1.create			(tex_t_envmap_1);
	}

	// Build textures
	{
		build_textures();
	}

	// PP
	s_postprocess.create				("postprocess");
	g_postprocess.create				(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX3,RCache.Vertex.Buffer(),RCache.QuadIB);

	// Menu
	s_menu.create						("distort");
	g_menu.create						(FVF::F_TL,RCache.Vertex.Buffer(),RCache.QuadIB);
}

void CRenderTarget::CRenderTargetDefferedDelete()
{
	destroy_textures();

#ifdef DEBUG
	ID3DBaseTexture* pSurf = 0;

	pSurf = t_envmap_0->surface_get();
	if (pSurf) pSurf->Release();
	_SHOW_REF("t_envmap_0 - #small", pSurf);

	pSurf = t_envmap_1->surface_get();
	if (pSurf) pSurf->Release();
	_SHOW_REF("t_envmap_1 - #small", pSurf);
#endif // DEBUG

	t_envmap_0->surface_set(NULL);
	t_envmap_1->surface_set(NULL);
	t_envmap_0.destroy();
	t_envmap_1.destroy();

	// Jitter
	for (int it = 0; it < TEX_jitter_count; it++) {
		t_noise[it]->surface_set(NULL);
#ifdef DEBUG
		_SHOW_REF("t_noise_surf[it]", t_noise_surf[it]);
#endif // DEBUG
		_RELEASE(t_noise_surf[it]);
	}

	t_noise_mipped->surface_set(NULL);
#ifdef DEBUG
	_SHOW_REF("t_noise_surf_mipped", t_noise_surf_mipped);
#endif // DEBUG
	_RELEASE(t_noise_surf_mipped);

	// 
	accum_spot_geom_destroy();
	accum_omnip_geom_destroy();
	accum_point_geom_destroy();
	accum_volumetric_geom_destroy();

	// Blenders
	xr_delete(b_combine);
	xr_delete(b_accum_reflected);
	xr_delete(b_accum_spot);
	xr_delete(b_accum_point);
	xr_delete(b_accum_direct);
	xr_delete(b_accum_mask);
	xr_delete(b_occq);

	t_depth.destroy();

	xr_delete(b_ssao);
	xr_delete(b_sunshafts);
	xr_delete(b_antialiasing);
	xr_delete(b_taa);
	xr_delete(b_luminance);
	xr_delete(b_bloom);

	if (RImplementation.o.ssr_replace)
		xr_delete(b_sspr);

	if (RImplementation.o.aa_mode == AA_MSAA)
		xr_delete(b_msaa_mark_edges);

	if (RImplementation.o.sun_il || RImplementation.o.spot_il)
		xr_delete(b_rsm);
}

void CRenderTarget::TXAA_rt_create(u32 s, u32 w, u32 h)
{
	rt_Motion				.create(tex_rt_Motion,				w, h, DXGI_FORMAT_R16G16_FLOAT,		SRV_RTV);
	rt_Motion_ms			.create(tex_rt_Motion_ms,			w, h, DXGI_FORMAT_R16G16_FLOAT,		SRV_RTV,	s);
	//rt_Generic_0_feedback	.create(tex_rt_Generic_0_feedback,	w, h, DXGI_FORMAT_R8G8B8A8_UNORM,	SRV_RTV);
	rt_Generic_0_feedback.create(tex_rt_Generic_0_feedback, Device.dwWidth, Device.dwHeight, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);
	//rt_Generic_1_feedback	.create(tex_rt_Generic_1_feedback,	w, h, DXGI_FORMAT_R8G8B8A8_UNORM,	SRV_RTV);

	Log("* TXAA RTs created");
}

void CRenderTarget::reset_light_marker( bool bResetStencil)
{
	dwLightMarkerID = 5;
	if (bResetStencil)
	{
		u32		Offset;
		float	_w					= float(Device.dwWidth);
		float	_h					= float(Device.dwHeight);
		u32		C					= color_rgba	(255,255,255,255);
		float	eps					= 0;
		float	_dw					= 0.5f;
		float	_dh					= 0.5f;
		FVF::TL* pv					= (FVF::TL*) RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
		pv->set						(-_dw,		_h-_dh,		eps,	1.f, C, 0, 0);	pv++;
		pv->set						(-_dw,		-_dh,		eps,	1.f, C, 0, 0);	pv++;
		pv->set						(_w-_dw,	_h-_dh,		eps,	1.f, C, 0, 0);	pv++;
		pv->set						(_w-_dw,	-_dh,		eps,	1.f, C, 0, 0);	pv++;
		RCache.Vertex.Unlock		(4,g_combine->vb_stride);
		RCache.set_Element			(s_occq->E[2]	);
		RCache.set_Geometry			(g_combine		);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}
}

void CRenderTarget::increment_light_marker()
{
	dwLightMarkerID += 2;

	const u32 iMaxMarkerValue = (RImplementation.o.aa_mode == AA_MSAA) ? 127 : 255;

	if (dwLightMarkerID > iMaxMarkerValue)
		reset_light_marker(true);
}

void CRenderTarget::prepare_sq_vertex(float w, float h, u32& bias, ref_geom& geom)
{
	FVF::V* pv = (FVF::V*)RCache.Vertex.Lock(4, geom->vb_stride, bias);

	pv->set(0, 0, 0, 0, 0);	pv++;
	pv->set(w, 0, 0, 1, 0);	pv++;
	pv->set(0, h, 0, 0, 1);	pv++;
	pv->set(w, h, 0, 1, 1);	pv++;

	RCache.Vertex.Unlock(4, geom->vb_stride);
}

/*void CRenderTarget::prepare_sq_vertex(u32& bias, ref_geom& geom)
{
	prepare_sq_vertex((float)Device.dwWidth, (float)Device.dwHeight, bias, geom);
}*/

void CRenderTarget::prepare_sq_vertex(const ref_rt& rt, u32& bias, ref_geom& geom)
{
	FVF::V* pv = (FVF::V*)RCache.Vertex.Lock(4, geom->vb_stride, bias);

	float w = (float)rt->dwWidth;
	float h = (float)rt->dwHeight;

	pv->set(0, 0, 0, 0, 0);	pv++;
	pv->set(w, 0, 0, 1, 0);	pv++;
	pv->set(0, h, 0, 0, 1);	pv++;
	pv->set(w, h, 0, 1, 1);	pv++;

	RCache.Vertex.Unlock(4, geom->vb_stride);
}

void CRenderTarget::enable_SSAA()
{
	if (!RImplementation.o.ssaa) return;

	PIX_EVENT(enable_supersampling);

	dwWidth		= SSAA.w;
	dwHeight	= SSAA.h;

	RImplementation.fWidth = (float)SSAA.w;
	RImplementation.fHeight = (float)SSAA.h;
}

void CRenderTarget::disable_SSAA()
{
	if (!RImplementation.o.ssaa) return;

	PIX_EVENT(disable_supersampling);

	dwWidth		= Device.dwWidth;
	dwHeight	= Device.dwHeight;

	RImplementation.fWidth = (float)Device.dwWidth;
	RImplementation.fHeight = (float)Device.dwHeight;
}

void CRenderTarget::SSAA_create()
{
	if (!RImplementation.o.ssaa)
	{
		SSAA.amount = 1.0;
		SSAA.mip = 0;
		SSAA.w = dwWidth;
		SSAA.h = dwHeight;
		return;
	}

	// SSAA
	if (RImplementation.o.ssaa > USE_SSAA && RImplementation.o.ssaa < USE_FSR)
	{
		// scale
		SSAA.amount = _sqrt(RImplementation.o.ssaa * 1.0f);

		// mip bias
		SSAA.mip = 0;

		// resolution
		SSAA.w = dwWidth * SSAA.amount;
		SSAA.h = dwHeight * SSAA.amount;
	}

	// FSR
	if (RImplementation.o.ssaa > USE_FSR)
	{
		// scale
		u32 scale100 = RImplementation.o.ssaa % 1000;
		float area = 0.01f * scale100;
		float amount = 1.0f / _sqrt(area);
		SSAA.amount = amount;

		// mip bias
		u32 mip100 = (RImplementation.o.ssaa % 100000) / 1000;
		float mip = -0.01f * mip100;
		SSAA.mip = mip;

		// resolution
		SSAA.w = dwWidth * SSAA.amount;
		SSAA.h = dwHeight * SSAA.amount;

		// constants
		float cas_sharpening = 0.5;
		float fsr_rcas_sharpening = 0.01f * (RImplementation.o.ssaa / 100000);

		CasSetup(
			SSAA.CAS_const.const0,
			SSAA.CAS_const.const1,
			cas_sharpening,
			(AF1)SSAA.w,
			(AF1)SSAA.h,
			(AF1)dwWidth,
			(AF1)dwHeight
		);

		FsrEasuCon(
			SSAA.FSR_const.EASU_const.const0,
			SSAA.FSR_const.EASU_const.const1,
			SSAA.FSR_const.EASU_const.const2,
			SSAA.FSR_const.EASU_const.const3,
			(AF1)SSAA.w,
			(AF1)SSAA.h,
			(AF1)SSAA.w,
			(AF1)SSAA.h,
			(AF1)dwWidth,
			(AF1)dwHeight
		);

		FsrRcasCon(
			SSAA.FSR_const.RCAS_const.const0,
			fsr_rcas_sharpening
		);
	}

	// disable SSAA
	disable_SSAA();

	// apply mip
	SSManager.SetMipBias(r__tf_mipbias + SSAA.mip);

	// AMD CAS params
	//SSAA.sharpness = 0;
	//SSAA.contrast = 0;

	// depth stencil buffer
	rt_SSAA_depth.create(tex_rt_SSAA_depth, SSAA.w, SSAA.h, DXGI_FORMAT_D24_UNORM_S8_UINT, SRV_DSV);

	// full resolution color & distort RTs
	//if (SSAA.fsr)	rt_SSAA_color.create(tex_rt_SSAA_color, w0, h0, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV_UAV);	// create unordered acces view for AMD CAS or FSR
	//else			rt_SSAA_color.create(tex_rt_SSAA_color, w0, h0, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);

	rt_SSAA_color.create(tex_rt_SSAA_color, dwWidth, dwHeight, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);
	rt_SSAA_distort.create(tex_rt_SSAA_distort, dwWidth, dwHeight, DXGI_FORMAT_R8G8B8A8_UNORM, SRV_RTV);

	// Menu
	s_menu_ssaa.create("distort_ssaa");

	// Post-processing
	s_postprocess_ssaa.create("postprocess_ssaa");

	// Effect
	CBlender_ssaa SSAA_blender;
	s_ssaa.create(&SSAA_blender);

	// Depth buffer
	t_depth->surface_set(rt_SSAA_depth->pSurface);

	Msg("* SSAA area: %f \n* SSAA amount: %f\n* SSAA mip: %f\n* SSAA final resolution: %ux%u",
		1.0f / (SSAA.amount * SSAA.amount), SSAA.amount, SSAA.mip, SSAA.w, SSAA.h);
}