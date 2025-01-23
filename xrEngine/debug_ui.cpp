///////////////////////////////////////////////////////////////
// Created: 18.01.2025
// Author: NS_Deathman
// User interface class
///////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "debug_ui.h"
///////////////////////////////////////////////////////////////
#include "imgui_api.h"
///////////////////////////////////////////////////////////////
CImguiAPI* ImguiAPI = nullptr;
///////////////////////////////////////////////////////////////
void CDebugUI::Initialize()
{
	ImguiAPI = new CImguiAPI();
	ImguiAPI->Initialize();

	m_bNeedDraw = true;
}

void CDebugUI::Destroy()
{
	ImguiAPI->Destroy();
	delete ImguiAPI;
}

void CDebugUI::OnFrameBegin()
{
	ImguiAPI->OnFrameBegin();
}

void CDebugUI::OnFrame()
{

}

void CDebugUI::OnFrameEnd()
{
	ImguiAPI->RenderFrame();
}

void CDebugUI::Render()
{
	if (m_bNeedDraw)
	{
		ImGui::PushFont(ImguiAPI->font_letterica_big);
		ImGui::Begin("ImGui Window");
		ImGui::PopFont();

		ImGui::PushFont(ImguiAPI->font_letterica_medium);

		ImGui::Button("asdlfn");

		//if (ImGui::Button("Test btn"))
		//	Sleep(0);

		ImGui::PopFont();

		ImGui::End();
	}
}

void CDebugUI::OnResetBegin()
{
	ImguiAPI->OnResetBegin();
}

void CDebugUI::OnResetEnd()
{
	ImguiAPI->OnResetEnd();
}
///////////////////////////////////////////////////////////////
