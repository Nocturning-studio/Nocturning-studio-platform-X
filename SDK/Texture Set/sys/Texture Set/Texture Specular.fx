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
      	ADDRESSU=CLAMP;
        ADDRESSV=CLAMP;
      };
   texture SurfaceTexture2 <string Name = " ";>;
   sampler SurfaceSampler2=sampler_state 
      {
	Texture=<SurfaceTexture2>;
	MipFilter=None;
      	ADDRESSU=CLAMP;
        ADDRESSV=CLAMP;
      };
   texture SurfaceTexture3 <string Name = " ";>;
   sampler SurfaceSampler3=sampler_state 
      {
	Texture=<SurfaceTexture3>;
	MipFilter=None;
      	ADDRESSU=CLAMP;
        ADDRESSV=CLAMP;
      };
   texture SurfaceTexture4 <string Name = " ";>;
   sampler SurfaceSampler4=sampler_state 
      {
	Texture=<SurfaceTexture4>;
	MipFilter=None;
      	ADDRESSU=CLAMP;
        ADDRESSV=CLAMP;
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
	OUT.Tex1=OUT.Tex+(ViewVec*2);
	OUT.Tex2=OUT.Tex-(ViewVec*2);
	OUT.Tex3=OUT.Tex+float2(-ViewVec.x*2,ViewVec.y*2);
	OUT.Tex4=OUT.Tex+float2(ViewVec.x*2,-ViewVec.y*2);
	return OUT;
    }

//--------------
// pixel shader
//--------------
   float4 PS_Secondary(OutPut IN) : COLOR
     {
	float4 Sharpen=0;
	float4 Heightmap=tex2D(SurfaceSampler1,IN.Tex);
	Sharpen=(tex2D(SurfaceSampler1,IN.Tex1)+tex2D(SurfaceSampler1,IN.Tex2)+tex2D(SurfaceSampler1,IN.Tex3)+tex2D(SurfaceSampler1,IN.Tex4))*0.25;
 	Heightmap +=clamp((Heightmap-Sharpen)*HeightmapAdjust.z,-HeightmapAdjust.z,HeightmapAdjust.z);
	Heightmap=((Heightmap*HeightmapAdjust.y)+((Heightmap*(Heightmap+0.5))*(1.0-HeightmapAdjust.y)))*HeightmapAdjust.x;
	Heightmap.x=lerp(0.5,dot(Heightmap,HeightmapChannel),HeightmapActive);
	float4 Ambient=dot(tex2D(SurfaceSampler2,IN.Tex),AOChannel);
	Sharpen=(tex2D(SurfaceSampler2,IN.Tex1)+tex2D(SurfaceSampler2,IN.Tex2)+tex2D(SurfaceSampler2,IN.Tex3)+tex2D(SurfaceSampler2,IN.Tex4))*0.25;
 	Ambient +=clamp((Ambient-Sharpen)*AOAdjust.z,-AOAdjust.z,AOAdjust.z);
	Ambient=((Ambient*AOAdjust.y)+((Ambient*(Ambient+0.5))*(1.0-AOAdjust.y)))*AOAdjust.x;
	Ambient.x=lerp(1.0,dot(Ambient,AOChannel),AOActive);
	float4 Emissive=tex2D(SurfaceSampler3,IN.Tex);
	Sharpen=(tex2D(SurfaceSampler3,IN.Tex1)+tex2D(SurfaceSampler3,IN.Tex2)+tex2D(SurfaceSampler3,IN.Tex3)+tex2D(SurfaceSampler3,IN.Tex4))*0.25;
 	Emissive +=clamp((Emissive-Sharpen)*EmissiveAdjust.z,-EmissiveAdjust.z,EmissiveAdjust.z);
	Emissive=((Emissive*EmissiveAdjust.y)+((Emissive*(Emissive+0.5))*(1.0-EmissiveAdjust.y)))*EmissiveAdjust.x;
	Emissive.x=lerp(0.0,dot(Emissive,EmissiveChannel),EmissiveActive);
	float4 Translucency=tex2D(SurfaceSampler4,IN.Tex);
	Sharpen=(tex2D(SurfaceSampler4,IN.Tex1)+tex2D(SurfaceSampler4,IN.Tex2)+tex2D(SurfaceSampler4,IN.Tex3)+tex2D(SurfaceSampler4,IN.Tex4))*0.25;
	Translucency=((Translucency*TranslucencyAdjust.y)+((Translucency*(Translucency+0.5))*(1.0-TranslucencyAdjust.y)))*TranslucencyAdjust.x;
	Translucency.x=lerp(0.0,dot(Translucency,TranslucencyChannel),TranslucencyActive);
	return float4(Emissive.x,Ambient.x,Translucency.x,Heightmap.x);
     }

//--------------
// techniques   
//--------------
    technique Secondary
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_Secondary(); 
	zwriteenable=false;
	zenable=false;
      }
      }
