// By EVOLVED
// www.evolved-software.com

//--------------
// un-tweaks
//--------------
   float4x4 WorldVP:WorldViewProjection; 
   float4x4 World:World;   
   float4x4 ViewInv:ViewInverse; 
   float4x4 ViewProj:ViewProjection; 

//--------------
// tweaks
//--------------
   float3 AmbientColor={0.025,0.025,0.025};
   float3 LightDirection={0.5,0.5,0.5};
   float3 LightDirectionColor={2.0,2.0,2.0};
   float Heightvec;
 
//--------------
// Textures
//--------------
   texture BaseTexture <string Name = "";>;	
   sampler BaseSampler=sampler_state 
      {
 	Texture=<BaseTexture>;
  	MagFilter=anisotropic;
	MinFilter=anisotropic;
	MipFilter=anisotropic;
	MaxAnisotropy=8;
      };
   texture NormalMapTexture <string Name = "";>;	
   sampler NormalMapSampler=sampler_state 
      {
 	Texture=<NormalMapTexture>;
  	MagFilter=anisotropic;
	MinFilter=anisotropic;
	MipFilter=anisotropic;
	MaxAnisotropy=4;
      };
   texture SecondaryTexture <string Name = "";>;	
   sampler SecondarySampler=sampler_state 
      {
 	Texture=<SecondaryTexture>;
      };

//--------------
// structs 
//--------------
   struct InPut
     {
 	float4 Pos:POSITION;
    	float2 Tex0:TEXCOORD;
 	float3 Normal:NORMAL;
 	float3 Tangent:TANGENT;
     };
   struct OutPut
     {
	float4 Pos:POSITION;
 	float2 Tex:TEXCOORD0;
  	float3 TBNRow1:TEXCOORD1;
  	float3 TBNRow2:TEXCOORD2;
  	float3 TBNRow3:TEXCOORD3;
  	float3 WorldPos:TEXCOORD4;
     };

//--------------
// vertex shader
//--------------
   OutPut VS(InPut IN) 
     { 
 	OutPut OUT;
  	OUT.Pos=mul(IN.Pos,WorldVP);
	OUT.Tex=IN.Tex0;
	float3 Normals=normalize(mul(IN.Normal,World)); 
	float3 Tangent=normalize(mul(IN.Tangent,World));
	OUT.TBNRow1=Tangent;
	OUT.TBNRow2=cross(Normals,Tangent);
	OUT.TBNRow3=Normals;
	float3 WorldPos=mul(IN.Pos,World);
	OUT.WorldPos=WorldPos;
	return OUT;
     }

//--------------
// pixel shader
//--------------
   float4 PS(OutPut IN) : COLOR
     {
	return float4(tex2D(BaseSampler,IN.Tex).xyz,1.0);
     }
   float4 PS_Lighting(OutPut IN) : COLOR
     {
	float3 ViewVec=normalize(ViewInv[3].xyz-IN.WorldPos);
	float3x3 WorldTBN=float3x3(IN.TBNRow1,IN.TBNRow2,IN.TBNRow3);
	float3 ViewNor=normalize(mul(ViewVec,transpose(WorldTBN)));
	IN.Tex +=(Heightvec*(saturate(tex2D(SecondarySampler,IN.Tex).w)-0.5))*ViewNor;
	IN.Tex +=(Heightvec*(saturate(tex2D(SecondarySampler,IN.Tex).w)-0.5))*ViewNor;
	IN.Tex +=(Heightvec*(saturate(tex2D(SecondarySampler,IN.Tex).w)-0.5))*ViewNor;
	float4 Diffuse=pow(tex2D(BaseSampler,IN.Tex),float4(2.2,2.2,2.2,1.0));
	float4 NormalMap=tex2D(NormalMapSampler,IN.Tex);
	float4 Secondarys=saturate(tex2D(SecondarySampler,IN.Tex.xy));
	float3 Normals=mul(normalize(float3(NormalMap.yw*2.0-1.0,0.75)),WorldTBN);
	float ViewNormal=max(dot(ViewVec,Normals),0.0);
	float3 Specular=lerp(0.04,Diffuse,NormalMap.x);
	Specular +=(1.0-Specular)*pow(1.0-NormalMap.z,2.2)*pow(1-ViewNormal,5.0);
	float Distribution=pow(NormalMap.z*NormalMap.z+0.004,2.0);
	float Denominator=Distribution.x-1.0;
	float3 Lighting=LightDirectionColor*max(dot(LightDirection,Normals),0.0);
        float Translucency=max((0.5+dot(-LightDirection,IN.TBNRow3)*0.5)*dot(normalize(-LightDirection+ViewVec),ViewVec),0.0)*0.5;
	float HalfVec=max(dot(normalize(LightDirection+ViewVec),Normals),0.0)*Denominator+1.0001;
	float3 LightSpecular=(Distribution/(3.141592*HalfVec*HalfVec))*Lighting;
	LightSpecular +=lerp(float3(1.0,0.85,0.6)*0.8,float3(0.8,0.85,1.0)*0.8,0.5+Normals.y*0.5);
	Lighting +=AmbientColor;
	Lighting *=(1.0-Specular)*(1.0-NormalMap.x)*Diffuse.xyz;
	Lighting +=(LightSpecular*Specular)+(Diffuse.xyz*Secondarys.x*25);
        Lighting +=pow(Diffuse.xyz,0.4545)*Translucency*Secondarys.z*LightDirectionColor;
	return float4(pow(Lighting*Secondarys.y,0.4545),Diffuse.w);
     }
   float4 PS_LightingFoliage(OutPut IN) : COLOR
     {
	float3 ViewVec=normalize(ViewInv[3].xyz-IN.WorldPos);
	float3x3 WorldTBN=float3x3(IN.TBNRow1,IN.TBNRow2,IN.TBNRow3);
	float3 ViewNor=normalize(mul(ViewVec,transpose(WorldTBN)));
	float4 Diffuse=pow(tex2D(BaseSampler,IN.Tex),float4(2.2,2.2,2.2,1.0));
	float4 NormalMap=tex2D(NormalMapSampler,IN.Tex);
	float3 Normals=mul(normalize(float3(NormalMap.yw*2.0-1.0,0.75)),WorldTBN);
	float ViewNormal=max(dot(ViewVec,Normals),0.0);
	float3 Specular=0.04+0.96*pow(1.0-NormalMap.z,2.2)*pow(1-ViewNormal,5.0);
	float Distribution=pow(NormalMap.z*NormalMap.z+0.004,2.0);
	float Denominator=Distribution.x-1.0;
	float3 Lighting=LightDirectionColor*max(dot(LightDirection,Normals),0.0);
        float Translucency=max((0.5+dot(-LightDirection,IN.TBNRow3)*0.5)*dot(normalize(-LightDirection+ViewVec),ViewVec),0.0)*0.5;
	float HalfVec=max(dot(normalize(LightDirection+ViewVec),Normals),0.0)*Denominator+1.0001;
	float3 LightSpecular=(Distribution/(3.141592*HalfVec*HalfVec))*Lighting;
	LightSpecular +=lerp(float3(1.0,0.85,0.6)*0.8,float3(0.8,0.85,1.0)*0.8,0.5+Normals.y*0.5);;
	Lighting +=AmbientColor;
	Lighting *=(1.0-Specular)*Diffuse.xyz;
	Lighting +=LightSpecular*Specular;
        Lighting +=pow(Diffuse.xyz,0.4545)*Translucency*NormalMap.x*LightDirectionColor;
	return float4(pow(Lighting,0.4545),Diffuse.w);
     }
   float4 PS_LightingTerrain(OutPut IN) : COLOR
     {
	float3 ViewVec=normalize(ViewInv[3].xyz-IN.WorldPos);
	float3x3 WorldTBN=float3x3(IN.TBNRow1,IN.TBNRow2,IN.TBNRow3);
	float3 ViewNor=normalize(mul(ViewVec,transpose(WorldTBN)));
	IN.Tex +=(Heightvec*(saturate(tex2D(NormalMapSampler,IN.Tex).x)-0.5))*ViewNor;
	IN.Tex +=(Heightvec*(saturate(tex2D(NormalMapSampler,IN.Tex).x)-0.5))*ViewNor;
	IN.Tex +=(Heightvec*(saturate(tex2D(NormalMapSampler,IN.Tex).x)-0.5))*ViewNor;
	float4 Diffuse=pow(tex2D(BaseSampler,IN.Tex),float4(2.2,2.2,2.2,1.0));
	float4 NormalMap=tex2D(NormalMapSampler,IN.Tex);
	float3 Normals=mul(normalize(float3(NormalMap.yw*2.0-1.0,0.75)),WorldTBN);
	float ViewNormal=max(dot(ViewVec,Normals),0.0);
	float3 Specular=0.04+0.96*pow(1.0-Diffuse.w,2.2)*pow(1-ViewNormal,5.0);
	float Distribution=pow(Diffuse.w*Diffuse.w+0.004,2.0);
	float Denominator=Distribution.x-1;
	float3 Lighting=LightDirectionColor*max(dot(LightDirection,Normals),0.0);
	float HalfVec=max(dot(normalize(LightDirection+ViewVec),Normals),0.0)*Denominator+1.0001;
	float3 LightSpecular=(Distribution/(3.141592*HalfVec*HalfVec))*Lighting;
	LightSpecular +=lerp(float3(1.0,0.85,0.6)*0.8,float3(0.8,0.85,1.0)*0.8,0.5+Normals.y*0.5);
	Lighting +=AmbientColor; 
	Lighting *=(1.0-Specular)*(1.0-0.0)*Diffuse.xyz;
	Lighting +=LightSpecular*Specular;
	return float4(pow(Lighting*NormalMap.z,0.4545),1.0);
     }

//--------------
// techniques   
//--------------
   technique Texture
      {
 	pass p1
      {		
 	vertexShader = compile vs_3_0 VS();
 	pixelShader  = compile ps_3_0 PS();
      }
      }
   technique Lighting
      {
 	pass p1
      {		
 	vertexShader = compile vs_3_0 VS();
 	pixelShader  = compile ps_3_0 PS_Lighting();
  	AlphaRef=32;
      }
      }
   technique LightingFoliage
      {
 	pass p1
      {		
 	vertexShader = compile vs_3_0 VS();
 	pixelShader  = compile ps_3_0 PS_LightingFoliage();
  	AlphaRef=32;
      }
      }
   technique LightingTerrain
      {
 	pass p1
      {		
 	vertexShader = compile vs_3_0 VS();
 	pixelShader  = compile ps_3_0 PS_LightingTerrain();
  	AlphaRef=32;
      }
      }
