#pragma once

#include "../xrEngine/feel_touch.h"
#include "../xrEngine/feel_sound.h"
#include "../xrEngine/iinputreceiver.h"
#include "../xrEngine/SkeletonAnimated.h"
#include "actor_flags.h"
#include "actor_defs.h"
#include "entity_alive.h"
#include "PHMovementControl.h"
#include "PhysicsShell.h"
#include "InventoryOwner.h"
#include "../xrEngine/StatGraph.h"
#include "PhraseDialogManager.h"

#include "step_manager.h"

using namespace ACTOR_DEFS;

class CInfoPortion;
struct GAME_NEWS_DATA;
class CActorCondition;
class CCustomOutfit;
class CKnownContactsRegistryWrapper;
class CEncyclopediaRegistryWrapper;
class CGameTaskRegistryWrapper;
class CGameNewsRegistryWrapper;
class CCharacterPhysicsSupport;
class CActorCameraManager;
// refs
class ENGINE_API CCameraBase;
class ENGINE_API CBoneInstance;
class ENGINE_API CBlend;
class CWeaponList;
class CEffectorBobbing;
class CHolderCustom;
class CUsableScriptObject;

struct SShootingEffector;
struct SSleepEffector;
class CSleepEffectorPP;
class CInventoryBox;
// class  CActorEffector;

class CHudItem;
class CArtefact;

struct SActorMotions;
struct SActorVehicleAnims;
class CActorCondition;
class SndShockEffector;
class DeathEffector;
class CActorFollowerMngr;
class CGameTaskManager;

class CCameraShotEffector;
class CActorInputHandler;

class CActorMemory;
class CActorStatisticMgr;

class CLocationManager;

class CActor : public CEntityAlive,
			   public IInputReceiver,
			   public Feel::Touch,
			   public CInventoryOwner,
			   public CPhraseDialogManager,
			   public CStepManager,
			   public Feel::Sound
#ifdef DEBUG
	,
			   public pureRender
#endif
{
	friend class CActorCondition;

  private:
	typedef CEntityAlive inherited;
	//////////////////////////////////////////////////////////////////////////
	// General fucntions
	//////////////////////////////////////////////////////////////////////////
  public:
	CActor();
	virtual ~CActor();

  public:
	virtual BOOL AlwaysTheCrow()
	{
		return TRUE;
	}

	virtual CAttachmentOwner* cast_attachment_owner()
	{
		return this;
	}
	virtual CInventoryOwner* cast_inventory_owner()
	{
		return this;
	}
	virtual CActor* cast_actor()
	{
		return this;
	}
	virtual CGameObject* cast_game_object()
	{
		return this;
	}
	virtual IInputReceiver* cast_input_receiver()
	{
		return this;
	}
	virtual CCharacterPhysicsSupport* character_physics_support()
	{
		return m_pPhysics_support;
	}
	virtual CCharacterPhysicsSupport* character_physics_support() const
	{
		return m_pPhysics_support;
	}
	virtual CPHDestroyable* ph_destroyable();
	CHolderCustom* Holder()
	{
		return m_holder;
	}

  public:
	virtual void Load(LPCSTR section);

	virtual void shedule_Update(u32 T);
	virtual void UpdateCL();

	virtual void OnEvent(NET_Packet& P, u16 type);

	// Render
	virtual void renderable_Render();
	virtual BOOL renderable_ShadowGenerate();
	virtual void feel_sound_new(CObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position,
								float power);
	virtual Feel::Sound* dcast_FeelSound()
	{
		return this;
	}
	float m_snd_noise;
#ifdef DEBUG
	virtual void OnRender();
#endif

	/////////////////////////////////////////////////////////////////
	// Inventory Owner

  public:
	// information receive & dialogs
	virtual bool OnReceiveInfo(shared_str info_id) const;
	virtual void OnDisableInfo(shared_str info_id) const;
	//	virtual void ReceivePdaMessage	(u16 who, EPdaMsg msg, shared_str info_id);

	virtual void NewPdaContact(CInventoryOwner*);
	virtual void LostPdaContact(CInventoryOwner*);

  protected:
	//	virtual void AddMapLocationsFromInfo (const CInfoPortion* info_portion) const;
	virtual void AddEncyclopediaArticle(const CInfoPortion* info_portion) const;
	virtual void AddGameTask(const CInfoPortion* info_portion) const;

  protected:
	struct SDefNewsMsg
	{
		GAME_NEWS_DATA* news_data;
		u32 time;
		bool operator<(const SDefNewsMsg& other) const
		{
			return time > other.time;
		}
	};
	xr_vector<SDefNewsMsg> m_defferedMessages;
	void UpdateDefferedMessages();

  public:
	void AddGameNews_deffered(GAME_NEWS_DATA& news_data, u32 delay);
	virtual void AddGameNews(GAME_NEWS_DATA& news_data);

  protected:
	CGameTaskManager* m_game_task_manager;
	CActorStatisticMgr* m_statistic_manager;

  public:
	virtual void StartTalk(CInventoryOwner* talk_partner);
	virtual void RunTalkDialog(CInventoryOwner* talk_partner);
	CGameTaskManager& GameTaskManager() const
	{
		return *m_game_task_manager;
	}
	CActorStatisticMgr& StatisticMgr()
	{
		return *m_statistic_manager;
	}
	CEncyclopediaRegistryWrapper* encyclopedia_registry;
	CGameNewsRegistryWrapper* game_news_registry;
	CCharacterPhysicsSupport* m_pPhysics_support;

	virtual LPCSTR Name() const
	{
		return CInventoryOwner::Name();
	}

  public:
	// PhraseDialogManager
	virtual void ReceivePhrase(DIALOG_SHARED_PTR& phrase_dialog);
	virtual void UpdateAvailableDialogs(CPhraseDialogManager* partner);
	virtual void TryToTalk();
	bool OnDialogSoundHandlerStart(CInventoryOwner* inv_owner, LPCSTR phrase);
	bool OnDialogSoundHandlerStop(CInventoryOwner* inv_owner);

	virtual void reinit();
	virtual void reload(LPCSTR section);
	virtual bool use_bolts() const;

	virtual void OnItemTake(CInventoryItem* inventory_item);

	virtual void OnItemRuck(CInventoryItem* inventory_item, EItemPlace previous_place);
	virtual void OnItemBelt(CInventoryItem* inventory_item, EItemPlace previous_place);

	virtual void OnItemDrop(CInventoryItem* inventory_item, bool just_before_destroy);
	virtual void OnItemDropUpdate();

	virtual void OnPlayHeadShotParticle(NET_Packet P);

	virtual void Die(CObject* who);
	virtual void Hit(SHit* pHDS);
	virtual void PHHit(float P, Fvector& dir, CObject* who, s16 element, Fvector p_in_object_space, float impulse,
					   ALife::EHitType hit_type /* = ALife::eHitTypeWound */);
	virtual void HitSignal(float P, Fvector& vLocalDir, CObject* who, s16 element);
	void HitSector(CObject* who, CObject* weapon);
	void HitMark(float P, Fvector dir, CObject* who, s16 element, Fvector position_in_bone_space, float impulse,
				 ALife::EHitType hit_type);

	virtual float GetMass();
	virtual float Radius() const;
	virtual void g_PerformDrop();

	virtual bool NeedToDestroyObject() const;
	virtual ALife::_TIME_ID TimePassedAfterDeath() const;

  public:
	// ���
	//			void		UpdateSleep			();

	// �������� ����������
	virtual void UpdateArtefactsOnBelt();
	virtual void MoveArtefactBelt(const CArtefact* artefact, bool on_belt);
	virtual float HitArtefactsOnBelt(float hit_power, ALife::EHitType hit_type);
	const xr_vector<const CArtefact*>& ArtefactsOnBelt()
	{
		return m_ArtefactsOnBelt;
	}

  protected:
	// ���� �������� �������
	ref_sound m_HeavyBreathSnd;
	ref_sound m_BloodSnd;

	xr_vector<const CArtefact*> m_ArtefactsOnBelt;

	bool m_bLastChanceTimeOutTimerStarted;
	bool m_bLastChanceAvailable;
	bool m_bLastChanceActivated;
	float m_fDamagePowerSaved;
	float m_fTimeFactorSaved;

  protected:
	// Sleep params
	// ����� ����� ������ ���� ���������
	ALife::_TIME_ID m_dwWakeUpTime;
	float m_fOldTimeFactor;
	float m_fOldOnlineRadius;
	float m_fSleepTimeFactor;

	/////////////////////////////////////////////////////////////////
	// misc properties
  protected:
	// Death
	float hit_slowmo;
	float hit_probability;

	// media
	SndShockEffector* m_sndShockEffector;
	DeathEffector* m_DeathEffector;
	xr_vector<ref_sound> sndHit[ALife::eHitTypeMax];
	ref_sound sndDie[SND_DIE_COUNT];

	float m_fLandingTime;
	float m_fJumpTime;
	float m_fFallTime;
	float m_fCamHeightFactor;

	// Dropping
	BOOL b_DropActivated;
	float f_DropPower;

	// random seed ��� Zoom mode
	s32 m_ZoomRndSeed;
	// random seed ��� Weapon Effector Shot
	s32 m_ShotRndSeed;

	bool m_bOutBorder;
	// ��������� ������� �������� � feel_touch, ��� ������� ���������� ��������� ������ �������� � �������
	u32 m_feel_touch_characters;
	// ���������� �� �������� ����� ������
	// ����� ���� ��� �������������� ��� ����� ������������� ������.
	// ��������������� � game
  private:
	void SwitchOutBorder(bool new_border_state);

  public:
	bool m_bAllowDeathRemove;
	//	u32						m_u32RespawnTime;

	////////////////////////////////////////////////////////
	void SetZoomRndSeed(s32 Seed = 0);
	s32 GetZoomRndSeed()
	{
		return m_ZoomRndSeed;
	};
	////////////////////////////////////////////////////////
	void SetShotRndSeed(s32 Seed = 0);
	s32 GetShotRndSeed()
	{
		return m_ShotRndSeed;
	};

  public:
	void detach_Vehicle();
	void steer_Vehicle(float angle);
	void attach_Vehicle(CHolderCustom* vehicle);

	virtual bool can_attach(const CInventoryItem* inventory_item) const;

  protected:
	CHolderCustom* m_holder;
	u16 m_holderID;
	bool use_Holder(CHolderCustom* holder);

	bool use_Vehicle(CHolderCustom* object);
	bool use_MountedWeapon(CHolderCustom* object);
	void ActorUse();

	/////////////////////////////////////////////////////////
	// actor model & animations
	/////////////////////////////////////////////////////////
  protected:
	BOOL m_bAnimTorsoPlayed;
	static void AnimTorsoPlayCallBack(CBlend* B);

	// Rotation
	SRotation r_torso;
	float r_torso_tgt_roll;
	// ��������� ����� ��� ����������� ������� ������ ������
	SRotation unaffected_r_torso;

	// ���������� ������
	float r_model_yaw_dest;
	float r_model_yaw;		 // orientation of model
	float r_model_yaw_delta; // effect on multiple "strafe"+"something"

  public:
	SActorMotions* m_anims;
	SActorVehicleAnims* m_vehicle_anims;

	CBlend* m_current_legs_blend;
	CBlend* m_current_torso_blend;
	CBlend* m_current_jump_blend;
	MotionID m_current_legs;
	MotionID m_current_torso;
	MotionID m_current_head;

	// callback �� �������� ������ ������
	void SetCallbacks();
	void ResetCallbacks();
	static void Spin0Callback(CBoneInstance*);
	static void Spin1Callback(CBoneInstance*);
	static void ShoulderCallback(CBoneInstance*);
	static void HeadCallback(CBoneInstance*);
	static void VehicleHeadCallback(CBoneInstance*);

	virtual const SRotation Orientation() const
	{
		return r_torso;
	};
	SRotation& Orientation()
	{
		return r_torso;
	};

	void g_SetAnimation(u32 mstate_rl);
	void g_SetSprintAnimation(u32 mstate_rl, MotionID& head, MotionID& torso, MotionID& legs);
	//////////////////////////////////////////////////////////////////////////
	// HUD
	//////////////////////////////////////////////////////////////////////////
  public:
	virtual void OnHUDDraw(CCustomHUD* hud);
	BOOL HUDview() const;

	// visiblity
	virtual float ffGetFov() const
	{
		return 90.f;
	}
	virtual float ffGetRange() const
	{
		return 500.f;
	}

	//////////////////////////////////////////////////////////////////////////
	// Cameras and effectors
	//////////////////////////////////////////////////////////////////////////
  public:
	CActorCameraManager& Cameras()
	{
		VERIFY(m_pActorEffector);
		return *m_pActorEffector;
	}
	IC CCameraBase* cam_Active()
	{
		return cameras[cam_active];
	}
	IC CCameraBase* cam_FirstEye()
	{
		return cameras[eacFirstEye];
	}
	// KD: need to know which cam active outside actor methods
	IC EActorCameras active_cam()
	{
		return cam_active;
	}
	void cam_Set(EActorCameras style);

  protected:
	void cam_Update(float dt, float fFOV);
	void camUpdateLadder(float dt);
	void cam_SetLadder();
	void cam_UnsetLadder();
	float currentFOV();

	// Cameras
	CCameraBase* cameras[eacMaxCam];
	EActorCameras cam_active;
	float fPrevCamPos;
	Fvector vPrevCamDir;
	float fCurAVelocity;
	CEffectorBobbing* pCamBobbing;

	//	void					LoadShootingEffector	(LPCSTR section);
	//	SShootingEffector*		m_pShootingEffector;

	void LoadSleepEffector(LPCSTR section);
	SSleepEffector* m_pSleepEffector;
	CSleepEffectorPP* m_pSleepEffectorPP;

	// �������� ����������, ���� � ������� �������
	CActorCameraManager* m_pActorEffector;
	static float f_Ladder_cam_limit;
	////////////////////////////////////////////
	// ��� �������������� � ������� �����������
	// ��� ����������
	///////////////////////////////////////////
  public:
	virtual void feel_touch_new(CObject* O);
	virtual void feel_touch_delete(CObject* O);
	virtual BOOL feel_touch_contact(CObject* O);
	virtual BOOL feel_touch_on_contact(CObject* O);

	CGameObject* ObjectWeLookingAt()
	{
		return m_pObjectWeLookingAt;
	}
	CInventoryOwner* PersonWeLookingAt()
	{
		return m_pPersonWeLookingAt;
	}
	LPCSTR GetDefaultActionForObject()
	{
		return *m_sDefaultObjAction;
	}
	//.	void					AddFollower					(u16 id);
	//.	void					RemoveFollower				(u16 id);
	//.	void					SendCmdToFollowers			(int cmd);
  protected:
	//.	void					DestroyFollowerInternal();//hack
	//.	CActorFollowerMngr&		Followers	();
	//.	CActorFollowerMngr*		m_followers;
	CUsableScriptObject* m_pUsableObject;
	// Person we're looking at
	CInventoryOwner* m_pPersonWeLookingAt;
	CHolderCustom* m_pVehicleWeLookingAt;
	CGameObject* m_pObjectWeLookingAt;
	CInventoryBox* m_pInvBoxWeLookingAt;

	// Tip for action for object we're looking at
	shared_str m_sDefaultObjAction;
	shared_str m_sCharacterUseAction;
	shared_str m_sDeadCharacterUseAction;
	shared_str m_sDeadCharacterUseOrDragAction;
	shared_str m_sCarCharacterUseAction;
	shared_str m_sInventoryItemUseAction;
	shared_str m_sInventoryBoxUseAction;

	// ����� ���������� ���������
	bool m_bPickupMode;
	// ���������� ��������� ���������
	float m_fPickupInfoRadius;

	void PickupModeUpdate();
	void PickupInfoDraw(CObject* object);
	void PickupModeUpdate_COD();

  public:
	void PickupModeOn();
	void PickupModeOff();

	//////////////////////////////////////////////////////////////////////////
	// Motions (������������ �������)
	//////////////////////////////////////////////////////////////////////////
  public:
	void g_cl_CheckControls(u32 mstate_wf, Fvector& vControlAccel, float& Jump, float dt);
	void g_cl_ValidateMState(float dt, u32 mstate_wf);
	void g_cl_Orientate(u32 mstate_rl, float dt);
	void g_sv_Orientate(u32 mstate_rl, float dt);
	void g_Orientate(u32 mstate_rl, float dt);
	bool g_LadderOrient();
	void UpdateMotionIcon(u32 mstate_rl);

	bool CanAccelerate();
	bool CanJump();
	bool CanMove();
	float CameraHeight();
	bool CanSprint();
	bool CanRun();
	void StopAnyMove();

	bool AnyAction()
	{
		return (mstate_real & mcAnyAction) != 0;
	};

	bool is_jump();

  protected:
	u32 mstate_wishful;
	u32 mstate_old;
	u32 mstate_real;

	BOOL m_bJumpKeyPressed;

	float m_fWalkAccel;
	float m_fJumpSpeed;
	float m_fRunFactor;
	float m_fRunBackFactor;
	float m_fWalkBackFactor;
	float m_fCrouchFactor;
	float m_fClimbFactor;
	float m_fSprintFactor;

	float m_fWalk_StrafeFactor;
	float m_fRun_StrafeFactor;
	//////////////////////////////////////////////////////////////////////////
	// User input/output
	//////////////////////////////////////////////////////////////////////////
  public:
	virtual void IR_OnMouseMove(int x, int y);
	virtual void IR_OnKeyboardPress(int dik);
	virtual void IR_OnKeyboardRelease(int dik);
	virtual void IR_OnKeyboardHold(int dik);
	virtual void IR_OnMouseWheel(int direction);
	virtual float GetLookFactor();

	//////////////////////////////////////////////////////////////////////////
	// Weapon fire control (������ �������)
	//////////////////////////////////////////////////////////////////////////
  public:
	virtual void g_WeaponBones(int& L, int& R1, int& R2);
	virtual void g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D);
	virtual BOOL g_State(SEntityState& state) const;
	virtual float GetWeaponAccuracy() const;
	bool IsZoomAimingMode() const
	{
		return m_bZoomAimingMode;
	}

  protected:
	// ���� ����� ������� � ������
	bool m_bZoomAimingMode;

	// ��������� ������������ ��������
	// ������� ��������� (����� ����� ����� �� �����)
	float m_fDispBase;
	float m_fDispAim;
	// ������������ �� ������� ��������� ���������� ������� ���������
	// ��������� �������� ������
	float m_fDispVelFactor;
	// ���� ����� �����
	float m_fDispAccelFactor;
	// ���� ����� �����
	float m_fDispCrouchFactor;
	// crouch+no acceleration
	float m_fDispCrouchNoAccelFactor;
	// �������� firepoint ������������ default firepoint ��� �������� ������ � ������
	Fvector m_vMissileOffset;

  public:
	// ���������, � ������ �������� ��� ������
	Fvector GetMissileOffset() const;
	void SetMissileOffset(const Fvector& vNewOffset);

  protected:
	// �������� ������������ ��� ��������
	int m_r_hand;
	int m_l_finger1;
	int m_r_finger2;
	int m_head;

	int m_l_clavicle;
	int m_r_clavicle;
	int m_spine2;
	int m_spine1;
	int m_spine;
	int m_neck;

	//////////////////////////////////////////////////////////////////////////
	// Network
	//////////////////////////////////////////////////////////////////////////
	void ConvState(u32 mstate_rl, string128* buf);

  public:
	virtual BOOL net_Spawn(CSE_Abstract* DC);
	virtual void net_Export(NET_Packet& P); // export to server
	virtual void net_Import(NET_Packet& P); // import from server
	virtual void net_Destroy();
	virtual BOOL net_Relevant();		  //	{ return getSVU() | getLocal(); };		// relevant for export to server
	virtual void net_Relcase(CObject* O); //
	virtual void xr_stdcall on_requested_spawn(CObject* object);
	// object serialization
	virtual void save(NET_Packet& output_packet);
	virtual void load(IReader& input_packet);
	virtual void net_Save(NET_Packet& P);
	virtual BOOL net_SaveRelevant();

  protected:
	xr_deque<net_update> NET;
	Fvector NET_SavedAccel;
	net_update NET_Last;
	BOOL NET_WasInterpolating; // previous update was by interpolation or by extrapolation
	u32 NET_Time;			   // server time of last update

	//---------------------------------------------
	void net_Import_Base(NET_Packet& P);
	void net_Import_Physic(NET_Packet& P);
	void net_Import_Base_proceed();
	void net_Import_Physic_proceed();
	//---------------------------------------------

	////////////////////////////////////////////////////////////////////////////
	virtual bool can_validate_position_on_spawn()
	{
		return false;
	}
	///////////////////////////////////////////////////////
	// ������ � ������� ������
	xr_deque<net_update_A> NET_A;

	//---------------------------------------------
	//	bool					m_bHasUpdate;
	/// spline coeff /////////////////////
	float SCoeff[3][4];			 // ������������ ��� ������� �����
	float HCoeff[3][4];			 // ������������ ��� ������� ������
	Fvector IPosS, IPosH, IPosL; // ��������� ������ ����� ������������ �����, ������, ��������

#ifdef DEBUG
	DEF_DEQUE(VIS_POSITION, Fvector);

	VIS_POSITION LastPosS;
	VIS_POSITION LastPosH;
	VIS_POSITION LastPosL;
#endif

	SPHNetState LastState;
	SPHNetState RecalculatedState;
	SPHNetState PredictedState;

	InterpData IStart;
	InterpData IRec;
	InterpData IEnd;

	bool m_bInInterpolation;
	bool m_bInterpolate;
	u32 m_dwIStartTime;
	u32 m_dwIEndTime;
	u32 m_dwILastUpdateTime;

	//---------------------------------------------
	DEF_DEQUE(PH_STATES, SPHNetState);
	PH_STATES m_States;
	u16 m_u16NumBones;
	void net_ExportDeadBody(NET_Packet& P);
	//---------------------------------------------
	void CalculateInterpolationParams();
	//---------------------------------------------
	virtual void make_Interpolation();
#ifdef DEBUG
	//---------------------------------------------
	virtual void OnRender_Network();
	//---------------------------------------------
#endif

	ref_geom hFriendlyIndicator;
	//////////////////////////////////////////////////////////////////////////
	// Actor physics
	//////////////////////////////////////////////////////////////////////////
  public:
	void g_Physics(Fvector& accel, float jump, float dt);
	virtual void ForceTransform(const Fmatrix& m);
	void SetPhPosition(const Fmatrix& pos);
	virtual void PH_B_CrPr(); // actions & operations before physic correction-prediction steps
	virtual void PH_I_CrPr(); // actions & operations after correction before prediction steps
	virtual void PH_A_CrPr(); // actions & operations after phisic correction-prediction steps
							  //	virtual void			UpdatePosStack	( u32 Time0, u32 Time1 );
	virtual void MoveActor(Fvector NewPos, Fvector NewDir);

	virtual void SpawnAmmoForWeapon(CInventoryItem* pIItem);
	virtual void RemoveAmmoForWeapon(CInventoryItem* pIItem);
	virtual void spawn_supplies();
	virtual bool human_being() const
	{
		return (true);
	}

	virtual shared_str GetDefaultVisualOutfit() const
	{
		return m_DefaultVisualOutfit;
	};
	virtual void SetDefaultVisualOutfit(shared_str DefaultOutfit)
	{
		m_DefaultVisualOutfit = DefaultOutfit;
	};
	virtual void UpdateAnimation()
	{
		g_SetAnimation(mstate_real);
	};

	virtual void ChangeVisual(shared_str NewVisual);
	virtual void OnChangeVisual();

	virtual void RenderIndicator(Fvector dpos, float r1, float r2, ref_shader IndShader);
	virtual void RenderText(LPCSTR Text, Fvector dpos, float* pdup, u32 color);

	//////////////////////////////////////////////////////////////////////////
	// Controlled Routines
	//////////////////////////////////////////////////////////////////////////

	void set_input_external_handler(CActorInputHandler* handler);
	bool input_external_handler_installed() const
	{
		return (m_input_external_handler != 0);
	}

	IC void lock_accel_for(u32 time)
	{
		m_time_lock_accel = Device.dwTimeGlobal + time;
	}

  private:
	CActorInputHandler* m_input_external_handler;
	u32 m_time_lock_accel;

	/////////////////////////////////////////
	// DEBUG INFO
  protected:
	CStatGraph* pStatGraph;

	shared_str m_DefaultVisualOutfit;

	LPCSTR invincibility_fire_shield_3rd;
	LPCSTR invincibility_fire_shield_1st;
	shared_str m_sHeadShotParticle;
	u32 last_hit_frame;
#ifdef DEBUG
	friend class CLevelGraph;
#endif
	Fvector m_AutoPickUp_AABB;
	Fvector m_AutoPickUp_AABB_Offset;

	void Check_for_AutoPickUp();
	void SelectBestWeapon(CObject* O);

  public:
	void SetWeaponHideState(u32 State, bool bSet);
	virtual CCustomOutfit* GetOutfit() const;

  private:
	CActorCondition* m_entity_condition;

  protected:
	virtual CEntityConditionSimple* create_entity_condition(CEntityConditionSimple* ec);

  public:
	IC CActorCondition& conditions() const;
	virtual DLL_Pure* _construct();
	virtual bool natural_weapon() const
	{
		return false;
	}
	virtual bool natural_detector() const
	{
		return false;
	}
	virtual bool use_center_to_aim() const;

  protected:
	u16 m_iLastHitterID;
	u16 m_iLastHittingWeaponID;
	s16 m_s16LastHittedElement;
	Fvector m_vLastHitDir;
	Fvector m_vLastHitPos;
	float m_fLastHealth;
	bool m_bWasHitted;
	bool m_bWasBackStabbed;

	virtual bool Check_for_BackStab_Bone(u16 element);

  public:
	virtual void SetHitInfo(CObject* who, CObject* weapon, s16 element, Fvector Pos, Fvector Dir);

	virtual void OnHitHealthLoss(float NewHealth);
	virtual void OnCriticalHitHealthLoss();
	virtual void OnCriticalWoundHealthLoss();
	virtual void OnCriticalRadiationHealthLoss();

	virtual bool InventoryAllowSprint();
	virtual void OnNextWeaponSlot();
	virtual void OnPrevWeaponSlot();

  public:
	virtual void on_weapon_shot_start(CWeapon* weapon);
	virtual void on_weapon_shot_stop(CWeapon* weapon);
	virtual void on_weapon_hide(CWeapon* weapon);
	Fvector weapon_recoil_delta_angle();
	Fvector weapon_recoil_last_delta();

  protected:
	virtual void update_camera(CCameraShotEffector* effector);
	// step manager
	virtual bool is_on_ground();

  private:
	CActorMemory* m_memory;

  public:
	void SetActorVisibility(u16 who, float value);
	IC CActorMemory& memory() const
	{
		VERIFY(m_memory);
		return (*m_memory);
	};

	void OnDifficultyChanged();

	IC float HitProbability()
	{
		return hit_probability;
	}
	virtual CVisualMemoryManager* visual_memory() const;

	virtual BOOL BonePassBullet(int boneID);
	virtual void On_B_NotCurrentEntity();

  private:
	collide::rq_results RQR;
	BOOL CanPickItem(const CFrustum& frustum, const Fvector& from, CObject* item);
	xr_vector<ISpatial*> ISpatialResult;

  private:
	CLocationManager* m_location_manager;

  public:
	IC const CLocationManager& locations() const
	{
		VERIFY(m_location_manager);
		return (*m_location_manager);
	}

  private:
	ALife::_OBJECT_ID m_holder_id;

  public:
	virtual bool register_schedule() const
	{
		return false;
	}
};

extern bool isActorAccelerated(u32 mstate, bool ZoomMode);

IC CActorCondition& CActor::conditions() const
{
	VERIFY(m_entity_condition);
	return (*m_entity_condition);
}

extern CActor* g_actor;
CActor* Actor();
extern const float s_fFallTime;
