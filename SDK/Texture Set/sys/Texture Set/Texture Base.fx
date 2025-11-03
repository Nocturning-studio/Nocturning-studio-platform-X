// By EVOLVED
// www.evolved-software.com

//--------------
// tweaks
//--------------
   float2 ViewVec;
   float4 ColorChannel;
   float4 ColorAdjust;
   float4 AlphaChannel;
   float4 AlphaAdjust;
   float AlphaActive;
   float4 NormalmapChannel;
   float4 NormalmapAdjust;
   float NormalmapActive;
   float4 HeightmapChannel;
   float4 HeightmapAdjust;
   float HeightmapActive;
   float4 MetalnessChannel;
   float4 MetalnessAdjust;
   float MetalnessActive;
   float4 RoughnessChannel;
   float4 RoughnessAdjust;
   float RoughnessActive;
   float4 AOChannel;
   float4 AOAdjust;
   float AOActive;
   float4 EmissiveChannel;
   float4 EmissiveAdjust;
   float EmissiveActive;
   float4 TranslucencyChannel;
   float4 TranslucencyAdjust;
   float TranslucencyActive;
   float Dtx5n;

//--------------
// Textures
//--------------
   texture SurfaceTexture1 <string Name = " ";>;
   sampler SurfaceSampler1=sampler_state 
      {
	Texture=<SurfaceTexture1>;
	MipFilter=None;
      };
   texture SurfaceTexture2 <string Name = " ";>;
   sampler SurfaceSampler2=sampler_state 
      {
	Texture=<SurfaceTexture2>;
	MipFilter=None;
      };
   texture SurfaceTexture3 <string Name = " ";>;
   sampler SurfaceSampler3=sampler_state 
      {
	Texture=<SurfaceTexture3>;
	MipFilter=None;
      };
   texture SurfaceTexture4 <string Name = " ";>;
   sampler SurfaceSampler4=sampler_state 
      {
	Texture=<SurfaceTexture4>;
	MipFilter=None;
      };
   texture SurfaceTexture5 <string Name = " ";>;
   sampler SurfaceSampler5=sampler_state 
      {
	Texture=<SurfaceTexture5>;
	MipFilter=None;
      };
   texture SurfaceTexture6 <string Name = " ";>;
   sampler SurfaceSampler6=sampler_state 
      {
	Texture=<SurfaceTexture6>;
	MipFilter=None;
      };

//--------------
// structs 
//--------------
   struct InPut
     {
 	float4 Pos:POSITION;
     };
   struct OutPut
     {
	float4 Pos:POSITION; 
 	float2 Tex:TEXCOORD0;
 	float2 Tex1:TEXCOORD1;
 	float2 Tex2:TEXCOORD2;
 	float2 Tex3:TEXCOORD3;
 	float2 Tex4:TEXCOORD4;
     };

//--------------
// vertex shader
//--------------
   OutPut VS(InPut IN) 
     {
 	OutPut OUT;
	OUT.Pos=IN.Pos;
  	OUT.Tex=((float2(IN.Pos.x,-IN.Pos.y)+1.0)*0.5)+ViewVec;
	OUT.Tex1=OUT.Tex+(ViewVec*2.0);
	OUT.Tex2=OUT.Tex-(ViewVec*2.0);
	OUT.Tex3=OUT.Tex+float2(-ViewVec.x*2.0,ViewVec.y*2.0);
	OUT.Tex4=OUT.Tex+float2(ViewVec.x*2.0,-ViewVec.y*2.0);
	return OUT;
    }

//--------------
// pixel shader
//--------------
   float4 PS_Base(OutPut IN) : COLOR
     {
	float4 Base=tex2D(SurfaceSampler1,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler1,IN.Tex1)+tex2D(SurfaceSampler1,IN.Tex2)+tex2D(SurfaceSampler1,IN.Tex3)+tex2D(SurfaceSampler1,IN.Tex4))*0.25;
 	Base +=clamp((Base-Sharpen)*ColorAdjust.z,-ColorAdjust.z,ColorAdjust.z);
	Base=((Base*ColorAdjust.y)+((Base*(Base+0.5))*(1-ColorAdjust.y)))*ColorAdjust.x;
	Base.xyz=lerp(Base.xyz,Base.xxx,ColorChannel.x);
	Base.xyz=lerp(Base.xyz,Base.yyy,ColorChannel.y);
	Base.xyz=lerp(Base.xyz,Base.zzz,ColorChannel.z);
	Base.xyz=lerp(Base.xyz,Base.www,ColorChannel.w);
	float4 Alpha=tex2D(SurfaceSampler2,IN.Tex);
	Sharpen=(tex2D(SurfaceSampler2,IN.Tex1)+tex2D(SurfaceSampler2,IN.Tex2)+tex2D(SurfaceSampler2,IN.Tex3)+tex2D(SurfaceSampler2,IN.Tex4))*0.25;
 	Alpha +=clamp((Alpha-Sharpen)*AlphaAdjust.z,-AlphaAdjust.z,AlphaAdjust.z);
	Alpha=((Alpha*AlphaAdjust.y)+((Alpha*(Alpha+0.5))*(1-AlphaAdjust.y)))*AlphaAdjust.x;
	Alpha.x=dot(Alpha,AlphaChannel);
	Base.w=lerp(1.0,Alpha.x,AlphaActive);
	return Base;
     }
   float4 PS_BaseTerrain(OutPut IN) : COLOR
     {
	float4 Base=tex2D(SurfaceSampler1,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler1,IN.Tex1)+tex2D(SurfaceSampler1,IN.Tex2)+tex2D(SurfaceSampler1,IN.Tex3)+tex2D(SurfaceSampler1,IN.Tex4))*0.25;
 	Base +=clamp((Base-Sharpen)*ColorAdjust.z,-ColorAdjust.z,ColorAdjust.z);
	Base=((Base*ColorAdjust.y)+((Base*(Base+0.5))*(1.0-ColorAdjust.y)))*ColorAdjust.x;
	Base.xyz=lerp(Base.xyz,Base.xxx,ColorChannel.x);
	Base.xyz=lerp(Base.xyz,Base.yyy,ColorChannel.y);
	Base.xyz=lerp(Base.xyz,Base.zzz,ColorChannel.z);
	Base.xyz=lerp(Base.xyz,Base.www,ColorChannel.w);
	float4 Roughness=tex2D(SurfaceSampler6,IN.Tex);
	Sharpen=(tex2D(SurfaceSampler6,IN.Tex1)+tex2D(SurfaceSampler6,IN.Tex2)+tex2D(SurfaceSampler6,IN.Tex3)+tex2D(SurfaceSampler6,IN.Tex4))*0.25;
 	Roughness +=clamp((Roughness-Sharpen)*RoughnessAdjust.z,-RoughnessAdjust.z,RoughnessAdjust.z);
	Roughness=((Roughness*RoughnessAdjust.y)+((Roughness*(Roughness+0.5))*(1-RoughnessAdjust.y)))*RoughnessAdjust.x;
	Base.w=lerp(0.5,dot(Roughness,RoughnessChannel),RoughnessActive);
	return Base;
     }
   float4 PS_Color(OutPut IN) : COLOR
     {
	float4 Base=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
	Base +=clamp((Base-Sharpen)*ColorAdjust.z,-ColorAdjust.z,ColorAdjust.z);
	Base=((Base*ColorAdjust.y)+((Base*(Base+0.5))*(1.0-ColorAdjust.y)))*ColorAdjust.x;
	Base.xyz=lerp(Base.xyz,Base.xxx,ColorChannel.x);
	Base.xyz=lerp(Base.xyz,Base.yyy,ColorChannel.y);
	Base.xyz=lerp(Base.xyz,Base.zzz,ColorChannel.z);
	Base.xyz=lerp(Base.xyz,Base.www,ColorChannel.w);
	Sharpen=(tex2D(SurfaceSampler4,IN.Tex1)+tex2D(SurfaceSampler4,IN.Tex2)+tex2D(SurfaceSampler4,IN.Tex3)+tex2D(SurfaceSampler4,IN.Tex4))*0.25;
	return float4(Base.xyz,1.0);
     }
   float4 PS_Alpha(OutPut IN) : COLOR
     {
	float4 Alpha=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	Alpha +=clamp((Alpha-Sharpen)*AlphaAdjust.z,-AlphaAdjust.z,AlphaAdjust.z);
	Alpha=((Alpha*AlphaAdjust.y)+((Alpha*(Alpha+0.5))*(1.0-AlphaAdjust.y)))*AlphaAdjust.x;
	Alpha.x=dot(Alpha,AlphaChannel);
	return float4(lerp(1.0,Alpha.xxx,AlphaActive),1.0);
     }
   float4 PS_Normalmap(OutPut IN) : COLOR
     {
	float4 Normal=lerp(tex2D(SurfaceSampler5,IN.Tex),tex2D(SurfaceSampler5,IN.Tex).wyzx,Dtx5n);
	float4 Sharpen=(lerp(tex2D(SurfaceSampler5,IN.Tex1),tex2D(SurfaceSampler5,IN.Tex1).wyzx,Dtx5n)+lerp(tex2D(SurfaceSampler5,IN.Tex2),tex2D(SurfaceSampler5,IN.Tex2).wyzx,Dtx5n)
                       +lerp(tex2D(SurfaceSampler5,IN.Tex3),tex2D(SurfaceSampler5,IN.Tex3).wyzx,Dtx5n)+lerp(tex2D(SurfaceSampler5,IN.Tex4),tex2D(SurfaceSampler5,IN.Tex4).wyzx,Dtx5n))*0.25;
 	Normal +=clamp((Normal-Sharpen)*NormalmapAdjust.z,-NormalmapAdjust.z,NormalmapAdjust.z);
	Normal=Normal*2-1;
	Normal.xy=((Normal.xy*NormalmapAdjust.y)+((Normal.xy*(abs(Normal.xy)+0.5))*(1.0-NormalmapAdjust.y)));
	Normal.xy *=NormalmapAdjust.x;
	Normal.z=1-dot(Normal.xy,Normal.xy);
	Normal.xyz=lerp(float3(0.5,0.5,1.0),0.5+normalize(Normal.xyz)*0.5,NormalmapActive);
	return float4(Normal.xyz,1.0);
     }
   float4 PS_Heightmap(OutPut IN) : COLOR
     {
	float4 Heightmap=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	Heightmap +=clamp((Heightmap-Sharpen)*HeightmapAdjust.z,-HeightmapAdjust.z,HeightmapAdjust.z);
	Heightmap=((Heightmap*HeightmapAdjust.y)+((Heightmap*(Heightmap+0.5))*(1.0-HeightmapAdjust.y)))*HeightmapAdjust.x;
	Heightmap.x=dot(Heightmap,HeightmapChannel);
	return float4(lerp(0.52,Heightmap.xxx,HeightmapActive),1.0);
     }
   float4 PS_Metalness(OutPut IN) : COLOR
     {
	float4 Metalness=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	Metalness +=clamp((Metalness-Sharpen)*MetalnessAdjust.z,-MetalnessAdjust.z,MetalnessAdjust.z);
	Metalness=((Metalness*MetalnessAdjust.y)+((Metalness*(Metalness+0.5))*(1.0-MetalnessAdjust.y)))*MetalnessAdjust.x;
	Metalness.x=dot(Metalness,MetalnessChannel);
	return float4(lerp(0.0,Metalness.xxx,MetalnessActive),1.0);
     }
   float4 PS_Roughness(OutPut IN) : COLOR
     {
	float4 Specular=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	Specular +=clamp((Specular-Sharpen)*RoughnessAdjust.z,-RoughnessAdjust.z,RoughnessAdjust.z);
	Specular=((Specular*RoughnessAdjust.y)+((Specular*(Specular+0.5))*(1.0-RoughnessAdjust.y)))*RoughnessAdjust.x;
	Specular.x=dot(Specular,RoughnessChannel);
	return float4(lerp(0.996,0.004+saturate(saturate(Specular.xxx)-0.008),RoughnessActive),1.0);
     }
   float4 PS_AO(OutPut IN) : COLOR
     {
	float4 AO=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	AO +=clamp((AO-Sharpen)*AOAdjust.z,-AOAdjust.z,AOAdjust.z);
	AO=((AO*AOAdjust.y)+((AO*(AO+0.5))*(1.0-AOAdjust.y)))*AOAdjust.x;
	AO.x=dot(AO,AOChannel);
	return float4(lerp(1.0,AO.xxx,AOActive),1.0);
     }
   float4 PS_Emissive(OutPut IN) : COLOR
     {
	float4 Emissive=dot(tex2D(SurfaceSampler5,IN.Tex),EmissiveChannel);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	Emissive +=clamp((Emissive-Sharpen)*EmissiveAdjust.z,-EmissiveAdjust.z,EmissiveAdjust.z);
	Emissive=((Emissive*EmissiveAdjust.y)+((Emissive*(Emissive+0.5))*(1.0-EmissiveAdjust.y)))*EmissiveAdjust.x;
	Emissive.x=dot(Emissive,EmissiveChannel);
	return float4(lerp(0.0,Emissive.xxx,EmissiveActive),1.0);
     }
   float4 PS_Translucency(OutPut IN) : COLOR
     {
	float4 Translucency=tex2D(SurfaceSampler5,IN.Tex);
	float4 Sharpen=(tex2D(SurfaceSampler5,IN.Tex1)+tex2D(SurfaceSampler5,IN.Tex2)+tex2D(SurfaceSampler5,IN.Tex3)+tex2D(SurfaceSampler5,IN.Tex4))*0.25;
 	Translucency +=clamp((Translucency-Sharpen)*TranslucencyAdjust.z,-TranslucencyAdjust.z,TranslucencyAdjust.z);
	Translucency=((Translucency*TranslucencyAdjust.y)+((Translucency*(Translucency+0.5))*(1.0-TranslucencyAdjust.y)))*TranslucencyAdjust.x;
	Translucency.x=dot(Translucency,TranslucencyChannel);
	return float4(lerp(0.0,Translucency.xxx,TranslucencyActive),1.0);
     }

//--------------
// techniques   
//--------------
    technique Base
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Base(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique BaseFoliage
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Base(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique BaseTerrain
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_BaseTerrain(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Color
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Base(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Alpha
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Alpha(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Normalmap
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Normalmap(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Heightmap
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Heightmap(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Metalness
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Metalness(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Roughness
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Roughness(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique AO
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_AO(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Emissive
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Emissive(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique Translucency
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Translucency(); 
	zwriteenable=false;
	zenable=false;
      }
      }