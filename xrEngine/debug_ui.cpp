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
#include "xr_input.h"
#include "../xrGame/xr_level_controller.h"
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

void CDebugUI::DrawUI()
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = true;

	//ImGui::PushFont(ImguiAPI->font_letterica_big);
	ImGui::Begin("ImGui Window");
	//ImGui::PopFont();

	//ImGui::PushFont(ImguiAPI->font_letterica_medium);

	ImGui::Text("1");

	//ImGui::Button("asdlfn");

	ImGui::SmallButton("123");

	//if (ImGui::Button("Test btn"))
	//	Sleep(0);

	//ImGui::PopFont();

	ImGui::End();
}

void CDebugUI::OnResetBegin()
{
	ImguiAPI->OnResetBegin();
}

void CDebugUI::OnResetEnd()
{
	ImguiAPI->OnResetEnd();
}

void CDebugUI::IR_OnKeyboardPress(int key)
{
	Msg("Keyboard pressed");
}

void CDebugUI::IR_OnKeyboardHold(int key)
{
}

void CDebugUI::IR_OnMouseMove(int key)
{
}

void CDebugUI::IR_OnMouseWheel(int key)
{
}

void CDebugUI::IR_OnMouseHold(int key)
{
}

void CDebugUI::IR_OnMousePress(int key)
{
	if (key == MOUSE_1)
		Msg("Mouse pressed");
}
///////////////////////////////////////////////////////////////
