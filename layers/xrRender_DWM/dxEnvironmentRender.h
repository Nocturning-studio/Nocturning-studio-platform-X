#ifndef dxEnvironmentRender_included
#define dxEnvironmentRender_included
#pragma once

#include "..\Include\xrRender\EnvironmentRender.h"

#include "blender.h"
#include "blender_sky.h"

class dxEnvDescriptorRender : public IEnvDescriptorRender
{
	friend class dxEnvDescriptorMixerRender;
public:
	virtual void OnDeviceCreate(CEnvDescriptor &owner);
	virtual void OnDeviceDestroy();

	virtual void Copy(IEnvDescriptorRender &_in);
private:
	ref_texture sky_texture, sky_texture_env, clouds_texture;
};

class dxEnvDescriptorMixerRender : public IEnvDescriptorMixerRender
{
public:
	virtual void Copy(IEnvDescriptorMixerRender &_in);

	virtual void Destroy();
	virtual void Clear();
	virtual void lerp(IEnvDescriptorRender *inA, IEnvDescriptorRender *inB);
public:
	STextureList sky_r_textures, sky_r_textures_env, clouds_r_textures;
};

class dxEnvironmentRender : public IEnvironmentRender
{
public:
	dxEnvironmentRender();
	virtual void Copy(IEnvironmentRender &_in);

	virtual void OnFrame(CEnvironment &env);
	virtual void OnLoad();
	virtual void OnUnload();
	virtual void RenderSky(CEnvironment &env);
	virtual void RenderClouds(CEnvironment &env);
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
	virtual particles_systems::library_interface const& particles_systems_library	();

private:

	CBlender_sky			b_sky;

	ref_shader				s_sky;
	ref_geom				g_skybox, g_clouds;

	ref_texture				tonemap;
	ref_texture				tsky0, tsky1;
};

#endif	//	EnvironmentRender_included