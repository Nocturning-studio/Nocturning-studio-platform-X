///////////////////////////////////////////////////////////////////////////////////
// Created: 05.01.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
class ENGINE_API COptickCapture
{
  private:
	int m_frames_to_capture;
	int m_start_capture_frame;
	int m_end_capture_frame;

	bool m_need_capture;
	bool m_switched_to_capturing;

  public:
	void Initialize();
	void Destroy();

	void OnFrame();
	void StartCapturing();
	void StartCapturing(int frames_to_capture);
	void StopCapturing();
	void SaveCapturedFrames();
	void TryToSaveCapture(str_c save_path);
	void SaveCapture(str_c save_path);

	void SwitchProfiler();
	void SwitchToCapturing();
	void SwitchToSaving();
};
///////////////////////////////////////////////////////////////////////////////////
extern ENGINE_API COptickCapture OptickCapture;
///////////////////////////////////////////////////////////////////////////////////