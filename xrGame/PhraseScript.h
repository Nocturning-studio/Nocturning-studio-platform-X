///////////////////////////////////////////////////////////////
// PhraseScript.h
// ������ ��� ����� �������� �� ���������
///////////////////////////////////////////////////////////////

#include "InfoPortionDefs.h"

#pragma once

class CGameObject;
class CInventoryOwner;
class TiXmlNode;
class CUIXml;

typedef TiXmlNode XML_NODE;

class CPhraseScript
{
  public:
	CPhraseScript();
	virtual ~CPhraseScript();

	// �������� �� XML �����
	virtual void Load(CUIXml* ui_xml, XML_NODE* phrase_node);

	// ����� � ����� ���������� (info_portion)
	virtual bool Precondition(const CGameObject* pSpeaker, LPCSTR dialog_id, LPCSTR phrase_id) const;
	virtual void Action(const CGameObject* pSpeaker, LPCSTR dialog_id, LPCSTR phrase_id) const;
	// ����� � ����� ����������� (dialog, phrase)
	virtual bool Precondition(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2, LPCSTR dialog_id,
							  LPCSTR phrase_id, LPCSTR next_phrase_id) const;
	virtual void Action(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2, LPCSTR dialog_id,
						LPCSTR phrase_id) const;
	// ����� �� ���������� �������
	//	virtual LPCSTR Text			(LPCSTR original_text, const CGameObject* pSpeaker1, const CGameObject* pSpeaker2,
	//LPCSTR dialog_id, int phrase_num) const; 	virtual bool   HasText		() const {return *m_sScriptTextFunc!=NULL;}

	DEFINE_VECTOR(shared_str, PRECONDITION_VECTOR, PRECONDITION_VECTOR_IT);
	virtual const PRECONDITION_VECTOR& Preconditions() const
	{
		return m_Preconditions;
	}

	DEFINE_VECTOR(shared_str, ACTION_NAME_VECTOR, ACTION_NAME_VECTOR_IT);
	virtual const ACTION_NAME_VECTOR& Actions() const
	{
		return m_ScriptActions;
	}

	void AddPrecondition(LPCSTR str);
	void AddAction(LPCSTR str);
	void AddHasInfo(LPCSTR str);
	void AddDontHasInfo(LPCSTR str);
	void AddGiveInfo(LPCSTR str);
	void AddDisableInfo(LPCSTR str);

  protected:
	// �������� ���������� ������������������ ����� � ��������� �����
	template <class T> void LoadSequence(CUIXml* ui_xml, XML_NODE* phrase_node, LPCSTR tag, T& str_vector);

	// ����������� � ����������� �� ����� ������� Precondition � Action
	virtual bool CheckInfo(const CInventoryOwner* pOwner) const;
	virtual void TransferInfo(const CInventoryOwner* pOwner) const;

	// ��� ���������� �������, ������� ���������� �����-�� �����
	//	shared_str m_sScriptTextFunc;

	// ���������� ��������, ������� ������������ ����� ���� ���
	// ��������� �����
	DEFINE_VECTOR(shared_str, ACTION_NAME_VECTOR, ACTION_NAME_VECTOR_IT);
	ACTION_NAME_VECTOR m_ScriptActions;

	DEFINE_VECTOR(shared_str, INFO_VECTOR, INFO_VECTOR_IT);

	INFO_VECTOR m_GiveInfo;
	INFO_VECTOR m_DisableInfo;

	// ������ ���������� ����������, ����������, ������� ����������
	// ��� ���� ���� ����� ����� ���������
	DEFINE_VECTOR(shared_str, PRECONDITION_VECTOR, PRECONDITION_VECTOR_IT);

	PRECONDITION_VECTOR m_Preconditions;
	// �������� �������/���������� ����������
	INFO_VECTOR m_HasInfo;
	INFO_VECTOR m_DontHasInfo;
};
