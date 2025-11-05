#include "stdafx.h"
#include "render.h"

void CRender::planar_render(ref_texture t_env_0, ref_texture t_env_1, Fvector4 env, Fvector4 amb)
{
	PIX_EVENT(render_reflections);
	//if (!DEVX) return; // debug

	// save params
	Fmatrix m_view_projet_saved;
	m_view_projet_saved.set(Device.mFullTransform);
	
	Fmatrix m_view_saved;
	m_view_saved.set(RCache.get_xform_view());

	Fvector camera_position_saved;
	camera_position_saved.set(Device.vCameraPosition);

	Fvector camera_direction_saved;
	camera_direction_saved.set(Device.vCameraDirection);

	Fvector camera_top_saved;
	camera_top_saved.set(Device.vCameraTop);

	CFrustum frustum_saved = ViewBase;

	phase = PHASE_PLANAR;

	// prepare render space & set render targets
	float h = r__dbg_planar_h;

	Device.vCameraPosition.y = -(Device.vCameraPosition.y - 2 * h);
	//Device.vCameraPosition_saved.y = Device.vCameraPosition.y; // for details
	Device.vCameraDirection.y = -Device.vCameraDirection.y;
	Device.vCameraTop.y = -Device.vCameraTop.y;

	Fmatrix m_view_new;
	m_view_new.build_camera_dir(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
	RCache.set_xform_view(m_view_new);

	HOM.Disable();

	r_pmask(true, false);

	Fmatrix m_view_project_new;
	m_view_project_new.mul(Device.mProject, m_view_new);

	Device.mFullTransform.set(m_view_project_new);
	//Device.mFullTransform_saved.set(Device.mFullTransform);

	ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	u32 s  = o.msaa_samples;
	u32 s1 = o.msaa_samples_reflections;

	// choose MSAA mode
	Target->u_setrt(s1 > 1 ? 
		Target->rt_Planar_color_ms : 
		Target->rt_Planar_color);

	Target->u_setzb(Target->rt_Planar_depth);

	RCache.clear_CurrentRenderTargetView(rgba_black);
	RCache.clear_CurrentDepthStencilView();

	D3D_VIEWPORT VP = { 0, 0, (float)Target->get_width(), (float)Target->get_height(), 0, 1.0f };
	RCache.set_Viewport(&VP);

	RCache.set_Stencil(FALSE);
	RCache.set_CullMode(D3D_CULL_BACK);

	float torch_state = 0;
	if (m_torch_enabled == TRUE) torch_state = 1;

	// calculate shadow map matrice
	Fmatrix xf_project;
	xf_project.set(m_planar_shadow_project);

	Fmatrix xf_invview;
	xf_invview.invert(m_view_new);

	Fmatrix m_shadow;
	m_shadow.mul(xf_project, xf_invview);
	m_shadow.mulB_44(m_planar_shadow_bias_t);

	// setup matrices
	m_planar_shadow.set(m_shadow);
	m_planar_vp_camera.set(m_view_projet_saved);

	// setup constants
	m_planar_env.set(env);
	m_planar_amb.set(amb);
	m_planar_bias.set(r__planar_bias_n, r__planar_bias_d, torch_state, h);

	// render sky
	g_pGamePersistent->Environment().RenderSky();
	g_pGamePersistent->Environment().RenderClouds();

	// render geometry & lighting
	render_main(m_view_project_new, false);

	r_dsgraph_render_graph(0);

	if (Details && opt(R__USE_PLANAR_DETAILS))
	{
		//Details->UpdateVisibleM(RDEVICE.vCameraPosition_saved, RDEVICE.mFullTransform_saved);
		Details->UpdateVisibleM(Device.vCameraPosition, Device.mFullTransform);
		Details->Render();
	}

	// reset render space & render params
	m_planar_env.set(0, 0, 0, 0);
	m_planar_amb.set(0, 0, 0, 0);
	m_planar_bias.set(0, 0, 0, 0);

	HOM.Enable();

	phase = PHASE_NORMAL;

	ViewBase = frustum_saved;

	Device.vCameraPosition.set(camera_position_saved);
	//Device.vCameraPosition_saved.set(Device.vCameraPosition);
	Device.vCameraDirection.set(camera_direction_saved);
	Device.vCameraTop.set(camera_top_saved);

	Device.mFullTransform.set(m_view_projet_saved);
	//Device.mFullTransform_saved.set(Device.mFullTransform);

	RCache.set_xform_view(m_view_saved);

	// Reset viewport from device params
	//VP.Width = (float)HW.m_ChainDesc.BufferDesc.Width;
	//VP.Height = (float)HW.m_ChainDesc.BufferDesc.Height;
	
	// Reset viewport from SSAA params
	VP.Width = (FLOAT)Target->rt_Generic_0->dwWidth;
	VP.Height = (FLOAT)Target->rt_Generic_0->dwHeight;

	RCache.set_Viewport(&VP);

	// MSAA partial
	if (s1 > 1 && s1 != s)
	{
		HW.pContext->ResolveSubresource(Target->rt_Planar_color->pTexture->surface_get(), 0,
			Target->rt_Planar_color_ms->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}

void CRender::planar_save_shadow(Fmatrix xf_project, Fmatrix bias_t)
{
	//if (!DEVX) return; // debug

	if (HW.FeatureLevel <= D3D_FEATURE_LEVEL_10_0) return;

	// copy z buffer
	HW.pContext->CopyResource(Target->rt_Planar_shadow->pTexture->surface_get(), Target->rt_smap_depth->pTexture->surface_get());

	// save matrices
	m_planar_shadow_project.set(xf_project);
	m_planar_shadow_bias_t.set(bias_t);
}