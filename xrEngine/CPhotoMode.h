//////////////////////////////////////////////////////////////////////
// CPhotoMode.h: interface for the CPhotoMode class
//////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////
#include "iinputreceiver.h"
#include "effector.h"
//////////////////////////////////////////////////////////////////////
class ENGINE_API CPhotoMode : public CEffectorCam, public IInputReceiver
{
  protected:
	CObject* Actor;

  private:
	static struct force_position
	{
		bool set_position;
		Fvector p;
	} g_position;

	static Fvector cmNorm[6];
	static Fvector cmDir[6];

	static Flags32 s_hud_flag;
	static Flags32 s_dev_flags;

	Fvector m_HPB;
	Fvector m_Position;
	Fmatrix m_Camera;
	u32 m_Stage;

	Fvector m_ActorPosition;

	Fvector m_vT;
	Fvector m_vR;
	Fvector m_vVelocity;
	Fvector m_vAngularVelocity;
	float m_fFov;
	Fvector3 m_fDOF;
	Fvector m_vGlobalDepthOfFieldParameters;
	float m_fGlobalTimeFactor;

	bool m_bAutofocusEnabled;
	bool m_bGridEnabled;
	bool m_bBordersEnabled;
	bool m_bWatermarkEnabled;
	bool m_bShowInputInfo;
	bool m_bGlobalHudDraw;
	bool m_bGlobalCrosshairDraw;

	bool m_bActorShowState;

	BOOL m_bMakeCubeMap;
	BOOL m_bMakeScreenshot;
	int m_iLMScreenshotFragment;

	float m_fSpeed0;
	float m_fSpeed1;
	float m_fSpeed2;
	float m_fSpeed3;
	float m_fAngSpeed0;
	float m_fAngSpeed1;
	float m_fAngSpeed2;
	float m_fAngSpeed3;

	void MakeCubeMapFace(Fvector& D, Fvector& N);
	void MakeScreenshotFace();
	void MakeCubemap();
	void MakeScreenshot();
	void ShowInputInfo();

	ref_sound music;

  public:
	void update_whith_timescale(Fvector& v, const Fvector& v_delta);
	CPhotoMode(float life_time = 60 * 60 * 1000);
	virtual ~CPhotoMode();

	void ChangeDepthOfField(int direction);
	void ChangeFieldOfView(int direction);
	void SwitchAutofocusState();
	void SwitchGridState();
	void SwitchCinemaBordersState();
	void SwitchWatermarkVisibility();
	void SwitchActorVisibility();
	void SwitchShowInputInfo();

	virtual void IR_OnKeyboardPress(int dik);
	virtual void IR_OnKeyboardHold(int dik);
	virtual void IR_OnMouseMove(int dx, int dy);
	virtual void IR_OnMouseHold(int btn);
	virtual void IR_OnMouseWheel(int direction);

	virtual BOOL ProcessCam(SCamEffectorInfo& info);

	static void SetGlobalPosition(const Fvector& p)
	{
		g_position.p.set(p), g_position.set_position = true;
	}

	static void GetGlobalPosition(Fvector& p)
	{
		p.set(g_position.p);
	}
};
//////////////////////////////////////////////////////////////////////
