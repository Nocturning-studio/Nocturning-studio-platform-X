#ifndef	dx10ShaderResourceStateCache_included
#define	dx10ShaderResourceStateCache_included
#pragma once
#include "../R_Backend.h"
class ENGINE_API  dx10ShaderResourceStateCache
{
public:
	dx10ShaderResourceStateCache();

	void	ResetDeviceState();

	void	Apply();

	void	SetPSResource( u32 uiSlot, ID3D11ShaderResourceView	*pRes );
	void	SetGSResource( u32 uiSlot, ID3D11ShaderResourceView	*pRes );
	void	SetVSResource( u32 uiSlot, ID3D11ShaderResourceView	*pRes );
	void	SetDSResource( u32 uiSlot, ID3D11ShaderResourceView	*pRes );
	void	SetHSResource( u32 uiSlot, ID3D11ShaderResourceView	*pRes );
	void	SetCSResource( u32 uiSlot, ID3D11ShaderResourceView	*pRes );

private:
	ID3D11ShaderResourceView* m_PSViews[mtMaxPixelShaderTextures];
	ID3D11ShaderResourceView* m_GSViews[mtMaxGeometryShaderTextures];
	ID3D11ShaderResourceView* m_VSViews[mtMaxVertexShaderTextures];
	ID3D11ShaderResourceView* m_HSViews[mtMaxHullShaderTextures];
	ID3D11ShaderResourceView* m_DSViews[mtMaxDomainShaderTextures];
	ID3D11ShaderResourceView* m_CSViews[mtMaxComputeShaderTextures];

	u32		m_uiMinPSView;
	u32		m_uiMaxPSView;

	u32		m_uiMinGSView;
	u32		m_uiMaxGSView;

	u32		m_uiMinVSView;
	u32		m_uiMaxVSView;

	u32		m_uiMinHSView;
	u32		m_uiMaxHSView;

	u32		m_uiMinDSView;
	u32		m_uiMaxDSView;

	u32		m_uiMinCSView;
	u32		m_uiMaxCSView;

	bool	m_bUpdatePSViews;
	bool	m_bUpdateGSViews;
	bool	m_bUpdateVSViews;
	bool	m_bUpdateHSViews;
	bool	m_bUpdateDSViews;
	bool	m_bUpdateCSViews;
};

extern ENGINE_API dx10ShaderResourceStateCache SRVSManager;

#endif	//	dx10ShaderResourceStateCache_included