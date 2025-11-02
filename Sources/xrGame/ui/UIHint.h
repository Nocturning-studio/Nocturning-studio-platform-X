#pragma once
#ifndef UIHINT_H
#define UIHINT_H

#include "UIWindow.h"

class CUIStatic;

class CUIHint : public CUIWindow, public pureRender
{
	friend class CUIXmlInit;

	CUIWindow* m_ownerWnd;
	CUIStatic* m_hint;

  public:
	CUIHint();
	virtual ~CUIHint();

	CUIWindow* Owner()
	{
		return m_ownerWnd;
	}
	CUIStatic* HintStatic()
	{
		return m_hint;
	}

	virtual void OnRender();
};

#endif
