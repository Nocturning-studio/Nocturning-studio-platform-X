// LightTrack.cpp: implementation of the CROS_impl class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "LightTrack.h"
#include "..\xrEngine\xr_object.h"

#ifdef _EDITOR
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

#include <ppl.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CROS_impl::CROS_impl()
{
	approximate.set(0, 0, 0);
	dwFrame = u32(-1);
	shadow_recv_frame = u32(-1);
	shadow_recv_slot = -1;

	result_count = 0;
	result_iterator = 0;
	result_frame = u32(-1);
	result_sun = 0;
	hemi_value = 0.5f;
	hemi_smooth = 0.5f;
	sun_value = 0.2f;
	sun_smooth = 0.2f;

	MODE = IRender_ObjectSpecific::TRACE_HEMI + IRender_ObjectSpecific::TRACE_SUN;
}

void CROS_impl::add(light* source)
{
	// Search
	for (xr_vector<Item>::iterator I = track.begin(); I != track.end(); I++)
		if (source == I->source)
		{
			I->frame_touched = Device.dwFrame;
			return;
		}

	// Register _new_
	track.push_back(Item());
	Item& L = track.back();
	L.frame_touched = Device.dwFrame;
	L.source = source;
	L.cache.verts[0].set(0, 0, 0);
	L.cache.verts[1].set(0, 0, 0);
	L.cache.verts[2].set(0, 0, 0);
	L.test = 0.f;
	L.energy = 0.f;
}

IC bool pred_energy(const CROS_impl::Light& L1, const CROS_impl::Light& L2)
{
	return L1.energy > L2.energy;
}
//////////////////////////////////////////////////////////////////////////
#pragma warning(push)
#pragma warning(disable : 4305)

// Оптимизированные направления гемисферы из X-Ray 1.6
const float hdir[lt_hemisamples][3] = {
	{-0.26287, 0.52573, 0.80902},  {0.27639, 0.44721, 0.85065},	  {-0.95106, 0.00000, 0.30902},
	{-0.95106, 0.00000, -0.30902}, {0.58779, 0.00000, -0.80902},  {0.58779, 0.00000, 0.80902},

	{-0.00000, 0.00000, 1.00000},  {0.52573, 0.85065, 0.00000},	  {-0.26287, 0.52573, -0.80902},
	{-0.42533, 0.85065, 0.30902},  {0.95106, 0.00000, 0.30902},	  {0.95106, 0.00000, -0.30902},

	{0.00000, 1.00000, 0.00000},   {-0.58779, 0.00000, 0.80902},  {-0.72361, 0.44721, 0.52573},
	{-0.72361, 0.44721, -0.52573}, {-0.58779, 0.00000, -0.80902}, {0.16246, 0.85065, -0.50000},

	{0.89443, 0.44721, 0.00000},   {-0.85065, 0.52573, -0.00000}, {0.16246, 0.85065, 0.50000},
	{0.68819, 0.52573, -0.50000},  {0.27639, 0.44721, -0.85065},  {0.00000, 0.00000, -1.00000},

	{-0.42533, 0.85065, -0.30902}, {0.68819, 0.52573, 0.50000},
};
#pragma warning(pop)

inline void CROS_impl::accum_hemi(float* hemi_cube, Fvector3& dir, float scale)
{
	if (dir.x > 0)
		hemi_cube[CUBE_FACE_POS_X] += dir.x * scale;
	else
		hemi_cube[CUBE_FACE_NEG_X] -= dir.x * scale;

	if (dir.y > 0)
		hemi_cube[CUBE_FACE_POS_Y] += dir.y * scale;
	else
		hemi_cube[CUBE_FACE_NEG_Y] -= dir.y * scale;

	if (dir.z > 0)
		hemi_cube[CUBE_FACE_POS_Z] += dir.z * scale;
	else
		hemi_cube[CUBE_FACE_NEG_Z] -= dir.z * scale;
}

void CROS_impl::smart_update(IRenderable* O)
{
	if (!O)
		return;
	if (0 == O->renderable.visual)
		return;

	--ticks_to_update;

	// Получение текущей позиции
	Fvector position;
	VERIFY(dynamic_cast<CROS_impl*>(O->renderable_ROS()));
	O->renderable.xform.transform_tiny(position, O->renderable.visual->vis.sphere.P);

	if (ticks_to_update <= 0)
	{
		update(O);
		last_position = position;

		if (result_count < lt_hemisamples)
			ticks_to_update = ::Random.randI(1, 2); // s_iUTFirstTimeMin, s_iUTFirstTimeMax
		else if (sky_rays_uptodate < lt_hemisamples)
			ticks_to_update = ::Random.randI(3, 7); // s_iUTPosChangedMin, s_iUTPosChangedMax
		else
			ticks_to_update = ::Random.randI(1000, 2001); // s_iUTIdleMin, s_iUTIdleMax
	}
	else
	{
		if (!last_position.similar(position, 0.15))
		{
			sky_rays_uptodate = 0;
			update(O);
			last_position = position;

			if (result_count < lt_hemisamples)
				ticks_to_update = ::Random.randI(1, 2);
			else
				ticks_to_update = ::Random.randI(3, 7);
		}
	}
}

void CROS_impl::calc_sky_hemi_value(Fvector& position, CObject* _object)
{
	// hemi-tracing
	if (MODE & IRender_ObjectSpecific::TRACE_HEMI)
	{
		sky_rays_uptodate += ps_r_dhemi_count;
		sky_rays_uptodate = _min(sky_rays_uptodate, lt_hemisamples);

		for (u32 it = 0; it < (u32)ps_r_dhemi_count; it++)
		{
			u32 sample = 0;
			if (result_count < lt_hemisamples)
			{
				sample = result_count;
				result_count++;
			}
			else
			{
				sample = (result_iterator % lt_hemisamples);
				result_iterator++;
			}

			// take sample
			Fvector direction;
			direction.set(hdir[sample][0], hdir[sample][1], hdir[sample][2]).normalize();
			result[sample] = !g_pGameLevel->ObjectSpace.RayTest(position, direction, 50.f, collide::rqtStatic,
																&cache[sample], _object);
		}
	}

	// Расчет значения гемисферы и накопление в кубические грани
	int _pass = 0;
	for (int it = 0; it < result_count; it++)
		if (result[it])
			_pass++;

	hemi_value = float(_pass) / float(result_count ? result_count : 1);
	hemi_value *= ps_r_dhemi_sky_scale;

	// Накопление в кубические грани для каждого успешного сэмпла
	for (int it = 0; it < result_count; it++)
	{
		if (result[it])
		{
			accum_hemi(hemi_cube, Fvector3().set(hdir[it][0], hdir[it][1], hdir[it][2]), ps_r_dhemi_sky_scale);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CROS_impl::update(IRenderable* O)
{
	// clip & verify
	if (dwFrame == Device.dwFrame)
		return;
	dwFrame = Device.dwFrame;
	if (0 == O)
		return;
	if (0 == O->renderable.visual)
		return;
	VERIFY(dynamic_cast<CROS_impl*>(O->renderable_ROS()));
	float dt = Device.fTimeDelta;

	CObject* _object = dynamic_cast<CObject*>(O);

	// select sample, randomize position inside object
	Fvector position;
	O->renderable.xform.transform_tiny(position, O->renderable.visual->vis.sphere.P);
	float radius;
	radius = O->renderable.visual->vis.sphere.R;
	position.y += .3f * radius;
	Fvector direction;
	direction.random_dir();

	// ========== НАЧАЛО: ЗАМЕНА HEMI РАСЧЕТА ==========

	// Инициализация кубических граней
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube[i] = 0;
	}

	bool bFirstTime = (0 == result_count);

	// Используем новый расчет HEMI из 1.6
	calc_sky_hemi_value(position, _object);

	// ========== КОНЕЦ: ЗАМЕНА HEMI РАСЧЕТА ==========

	// sun-tracing (БЕЗ ИЗМЕНЕНИЙ)
	light* sun = (light*)RenderImplementation.Lights.sun_adapted._get();

	if (MODE & IRender_ObjectSpecific::TRACE_SUN)
	{
		if (--result_sun < 0)
		{
			result_sun += ::Random.randI(lt_hemisamples / 4, lt_hemisamples / 2);
			Fvector sun_direction;
			sun_direction.set(sun->get_direction()).invert().normalize();
			sun_value = !(g_pGameLevel->ObjectSpace.RayTest(position, sun_direction, 500.f, collide::rqtBoth,
															&cache_sun, _object))
							? 1.f
							: 0.f;
		}
	}

	// Сглаживание HEMI (обновленная версия)
	if (bFirstTime)
	{
		hemi_smooth = hemi_value;
		CopyMemory(hemi_cube_smooth, hemi_cube, NUM_FACES * sizeof(float));
	}
	update_smooth();

	// light-tracing (БЕЗ ИЗМЕНЕНИЙ)
	BOOL bTraceLights = MODE & IRender_ObjectSpecific::TRACE_LIGHTS;
	if ((!O->renderable_ShadowGenerate()) && (!O->renderable_ShadowReceive()))
		bTraceLights = FALSE;
	if (bTraceLights)
	{
		// Select nearest lights
		Fvector bb_size = {radius, radius, radius};
		g_SpatialSpace->q_box(RenderImplementation.lstSpatial, 0, STYPE_LIGHTSOURCE, position, bb_size);
		for (u32 o_it = 0; o_it < RenderImplementation.lstSpatial.size(); o_it++)
		{
			ISpatial* spatial = RenderImplementation.lstSpatial[o_it];
			light* source = (light*)(spatial->dcast_Light());
			VERIFY(source); // sanity check
			float R = radius + source->get_range();
			if (position.distance_to(source->get_position()) < R)
				add(source);
		}

		// Trace visibility
		lights.clear();
		float traceR = radius * .5f;
		for (s32 id = 0; id < s32(track.size()); id++)
		{
			// remove untouched lights
			xr_vector<CROS_impl::Item>::iterator I = track.begin() + id;
			if (I->frame_touched != Device.dwFrame)
			{
				track.erase(I);
				id--;
				continue;
			}

			// Trace visibility
			Fvector P, D;
			float amount = 0;
			light* xrL = I->source;
			Fvector& LP = xrL->get_position();
			P.mad(position, P.random_dir(), traceR); // Random point inside range

			// point/spot
			float f = D.sub(P, LP).magnitude();
			if (g_pGameLevel->ObjectSpace.RayTest(LP, D.div(f), f, collide::rqtStatic, &I->cache, _object))
				amount -= lt_dec;
			else
				amount += lt_inc;
			I->test += amount * dt;
			clamp(I->test, -.5f, 1.f);
			I->energy = .9f * I->energy + .1f * I->test;

			//
			float E = I->energy * xrL->get_color().intensity();
			if (E > EPS)
			{
				// Select light
				lights.push_back(CROS_impl::Light());
				CROS_impl::Light& L = lights.back();
				L.source = xrL;
				L.color.mul_rgb(xrL->get_color(), I->energy / 2);
				L.energy = I->energy / 2;
				if (!xrL->flags.bStatic)
				{
					L.color.mul_rgb(.5f);
					L.energy *= .5f;
				}
			}
		}

		// Sun
		float E = sun_smooth * sun->get_color().intensity();
		if (E > EPS)
		{
			// Select light
			lights.push_back(CROS_impl::Light());
			CROS_impl::Light& L = lights.back();
			L.source = sun;
			L.color.mul_rgb(sun->get_color(), sun_smooth / 2);
			L.energy = sun_smooth;
		}

		// Sort lights by importance - important for R1-shadows
		concurrency::parallel_sort(lights.begin(), lights.end(), pred_energy);
	}

	// Process ambient lighting and approximate average lighting (БЕЗ ИЗМЕНЕНИЙ)
	// Process our lights to find average luminiscense
	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	Fvector accum = {desc->ambient.x, desc->ambient.y, desc->ambient.z};
	Fvector hemi = {desc->hemi_color.x, desc->hemi_color.y, desc->hemi_color.z};
	Fvector sun_ = {desc->sun_color.x, desc->sun_color.y, desc->sun_color.z};
	if (MODE & IRender_ObjectSpecific::TRACE_HEMI)
		hemi.mul(hemi_smooth);
	else
		hemi.mul(.2f);
	accum.add(hemi);
	if (MODE & IRender_ObjectSpecific::TRACE_SUN)
		sun_.mul(sun_smooth);
	else
		sun_.mul(.2f);
	accum.add(sun_);
	if (MODE & IRender_ObjectSpecific::TRACE_LIGHTS)
	{
		Fvector lacc = {0, 0, 0};
		for (u32 lit = 0; lit < lights.size(); lit++)
		{
			float d = lights[lit].source->get_position().distance_to(position);
			float r = lights[lit].source->get_range();
			float a = clampr(1.f - d / (r + EPS), 0.f, 1.f) * (lights[lit].source->flags.bStatic ? 1.f : 2.f);
			lacc.x += lights[lit].color.r * a;
			lacc.y += lights[lit].color.g * a;
			lacc.z += lights[lit].color.b * a;
		}
		accum.add(lacc);
	}
	else
		accum.set(.1f, .1f, .1f);
	approximate = accum;
}

extern float ps_r_lt_smooth;

// hemi & sun: update and smooth
void CROS_impl::update_smooth(IRenderable* O)
{
	if (dwFrameSmooth == Device.dwFrame)
		return;

	dwFrameSmooth = Device.dwFrame;

	// Используем умное обновление для HEMI
	smart_update(O);

	float l_f = Device.fTimeDelta * ps_r_lt_smooth;
	clamp(l_f, 0.f, 1.f);
	float l_i = 1.f - l_f;
	hemi_smooth = hemi_value * l_f + hemi_smooth * l_i;
	sun_smooth = sun_value * l_f + sun_smooth * l_i;

	// Сглаживание кубических граней (новая функциональность)
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube_smooth[i] = hemi_cube[i] * l_f + hemi_cube_smooth[i] * l_i;
	}
}
