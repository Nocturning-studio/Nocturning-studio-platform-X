#pragma once



class ENGINE_API CEnvModifier
{
  public:
    Fvector3 position;
    float radius;
    float power;
    float far_plane;
    Fvector3 fog_color;
    float fog_density;
    Fvector3 ambient;
    Fvector3 sky_color;
    Fvector3 hemi_color;
    Flags16 use_flags;

    bool load(IReader* fs);
    float sum(CEnvModifier& _another, Fvector3& view);
};