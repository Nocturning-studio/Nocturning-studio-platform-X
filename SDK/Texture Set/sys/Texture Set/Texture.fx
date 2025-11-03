// By EVOLVED
// www.evolved-software.com

//--------------
// tweaks
//--------------
   float2 ViewVec;
   float2 ViewVec2;
   float AspectX;
   float AspectY;
   float Dxt5N;
   float4 MaskColor;
   float4 BackColor;
   float BlurOffset;
   float GaussianBlurWeight[16]=
    {
     0.07981,
     0.078235,
     0.073694,
     0.066703,
     0.058016,
     0.048488,
     0.038941,
     0.030051,
     0.022285,
     0.01588,
     0.010873,
     0.007154,
     0.004523,
     0.002748,
     0.001604,
     0.0009
    };

//--------------
// Textures
//--------------
   texture SurfaceTexture1 <string Name = " ";>;
   sampler SurfaceSampler1=sampler_state 
      {
	Texture=<SurfaceTexture1>;
  	MagFilter=None;
	MinFilter=None;
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
	AddressU=Border;
	AddressV=Border;
	AddressW=Border;
	BorderColor=float4(5,5,5,5);
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
     };

//--------------
// vertex shader
//--------------
   OutPut VS(InPut IN) 
     {
 	OutPut OUT;
	OUT.Pos=IN.Pos;
  	OUT.Tex=((float2(IN.Pos.x,-IN.Pos.y)+1.0)*0.5)+ViewVec;
	return OUT;
    }

//--------------
// pixel shader
//--------------
   float4 PS(OutPut IN) : COLOR
     {
	float4 Base=tex2D(SurfaceSampler4,IN.Tex*float2(AspectX,AspectY));
	Base.xyz=lerp(Base.xyz,lerp(BackColor.xyz,Base.xyz,Base.w),BackColor.w);
	Base.xyz=lerp(Base.xyz,Base.xxx,MaskColor.x);
	Base.xyz=lerp(Base.xyz,Base.yyy,MaskColor.y);
	Base.xyz=lerp(Base.xyz,Base.zzz,MaskColor.z);
	Base.xyz=lerp(Base.xyz,Base.www,MaskColor.w);
	Base.xyz=lerp(Base.xyz,float3(tex2D(SurfaceSampler4,IN.Tex*float2(AspectX,AspectY)).yw,0.75),Dxt5N);
	return float4(Base.xyz,1);
     }
   float4 PS_DownFilter(OutPut IN) : COLOR
     {
	float4 Surface=tex2D(SurfaceSampler1,IN.Tex);
	Surface +=tex2D(SurfaceSampler1,IN.Tex+float2(0,ViewVec2.y*0.5));
	Surface +=tex2D(SurfaceSampler1,IN.Tex+float2(ViewVec2.x*0.5,0));
	Surface +=tex2D(SurfaceSampler1,IN.Tex+float2(ViewVec2.x*0.5,ViewVec2.y*0.5));
	return pow(Surface*0.25,2.2);
     }
   float4 PS_BlurH(OutPut IN) : COLOR
     {
	float4 BlurH=tex2D(SurfaceSampler2,IN.Tex)*GaussianBlurWeight[0];
	for (int i=1; i < 15; i++) {
	   BlurH +=tex2D(SurfaceSampler2,IN.Tex+float2(ViewVec.x*i*BlurOffset*2,0))*GaussianBlurWeight[i];
	   BlurH +=tex2D(SurfaceSampler2,IN.Tex-float2(ViewVec.x*i*BlurOffset*2,0))*GaussianBlurWeight[i];
	  }
	return BlurH;
     }
   float4 PS_BlurV(OutPut IN) : COLOR
     {
	float4 BlurV=tex2D(SurfaceSampler3,IN.Tex)*GaussianBlurWeight[0];
	for (int i=1; i < 15; i++) {
	   BlurV +=tex2D(SurfaceSampler3,IN.Tex+float2(0,ViewVec.y*i*BlurOffset*2))*GaussianBlurWeight[i];
	   BlurV +=tex2D(SurfaceSampler3,IN.Tex-float2(0,ViewVec.y*i*BlurOffset*2))*GaussianBlurWeight[i];
	  }
	return pow(BlurV,1.0/2.2);
     }

//--------------
// techniques   
//--------------
    technique Resize
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique DownFilter
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_DownFilter(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique BlurH
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_BlurH(); 
	zwriteenable=false;
	zenable=false;
      }
      }
    technique BlurV
      {
 	pass p1
      {	
 	VertexShader = compile vs_3_0 VS();
 	PixelShader  = compile ps_3_0 PS_BlurV(); 
	zwriteenable=false;
	zenable=false;
      }
      }
