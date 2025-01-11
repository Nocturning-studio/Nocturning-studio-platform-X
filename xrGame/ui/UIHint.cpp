#include "stdafx.h"
#include "UIHint.h"
#include "UIStatic.h"

CUIHint::CUIHint()
{
	Device.seqRender.Add(this, 0);

	m_ownerWnd = nullptr;
	m_hint = nullptr;
}

CUIHint::~CUIHint()
{
	Device.seqRender.Remove(this);
}

void CUIHint::OnRender()
{
	OPTICK_EVENT("CUIHint::OnRender");

	m_hint->Update();
	Draw();
}
