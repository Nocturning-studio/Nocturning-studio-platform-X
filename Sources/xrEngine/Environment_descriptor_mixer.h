#pragma once

class ENGINE_API CEnvDescriptorMixer : public CEnvDescriptor
{
  public:
	STextureList sky_r_textures;
	STextureList sky_r_textures_env;
	STextureList clouds_r_textures;
	STextureList lut_r_textures;
	float weight;

  public:
	CEnvDescriptorMixer(shared_str const& identifier);
	void lerp(CEnvironment* parent, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& M, float m_power);
	void clear();
	void destroy();
};