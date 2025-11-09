#ifndef	dx10SamplerStateCache_included
#define	dx10SamplerStateCache_included
#pragma once


class ENGINE_API dx10SamplerStateCache
{
public:
	enum
	{
		hInvalidHandle = 0xFFFFFFFF
	};

	//	State handle
	typedef	u32	SHandle;
	typedef	xr_vector<SHandle>	HArray;
public:
	dx10SamplerStateCache();
	~dx10SamplerStateCache();

	void	ClearStateArray();

	SHandle	GetState( D3D11_SAMPLER_DESC& desc );

	void	VSApplySamplers(HArray &samplers);
	void	PSApplySamplers(HArray &samplers);
	void	GSApplySamplers(HArray &samplers);
	void	HSApplySamplers(HArray &samplers);
	void	DSApplySamplers(HArray &samplers);
	void	CSApplySamplers(HArray &samplers);

	void SetMaxAnisotropy(UINT uiMaxAniso);
	void SetMipBias(float uiMipBias);

	//	Marks all device sample as unused
	void	ResetDeviceState();

	//	Private declarations
private:
	typedef	ID3D11SamplerState	IDeviceState;
	typedef	D3D11_SAMPLER_DESC	StateDecs;

	struct StateRecord 
	{
		u32					m_crc;
		IDeviceState*		m_pState;
	};

private:
	void	CreateState( StateDecs desc, IDeviceState** ppIState );
	SHandle	FindState( const StateDecs& desc, u32 StateCRC );

	void	PrepareSamplerStates(
		HArray &samplers, 
		ID3D11SamplerState	*pSS[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT],
		SHandle pCurrentState[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT],
		u32	&uiMin,
		u32	&uiMax
	) const;

	//	Private data
private:
	//	This must be cleared on device destroy
	xr_vector<StateRecord>	m_StateArray;

	SHandle					m_aPSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle					m_aVSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle					m_aGSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle					m_aHSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle					m_aDSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle					m_aCSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

	u32						m_uiMaxAnisotropy;
	float					m_uiMipBias;
};

extern ENGINE_API	dx10SamplerStateCache	SSManager;

#endif	//	dx10SamplerStateCache_included