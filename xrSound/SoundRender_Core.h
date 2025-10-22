#ifndef SoundRender_CoreH
#define SoundRender_CoreH
#pragma once

#include "SoundRender.h"
#include "SoundRender_Cache.h"
#include "../xrEngine/Sound_environment_common.h"

class CSoundRender_Core : public CSound_manager_interface
{
	volatile BOOL bLocked;

  protected:
	virtual void _create_data(ref_sound_data& S, LPCSTR fName, esound_type sound_type, int game_type);
	virtual void _destroy_data(ref_sound_data& S);

  protected:
	BOOL bListenerMoved;

  public:
	typedef std::pair<ref_sound_data_ptr, float> event;
	xr_vector<event> s_events;

  public:
	BOOL bPresent;
	BOOL bUserEnvironment;
	BOOL bEAX; // Boolean variable to indicate presence of EAX Extension
	BOOL bDeferredEAX;
	BOOL bReady;
	BOOL bDevicePaused;

	CTimer Timer;
	float fTimer_Value;
	float fTimer_Delta;
	float fTime_Factor;
	sound_event* Handler;

	bool bNeedToUpdateEnvironment;
	float fEnvironmentRadius;
	float fFogDensity;

  protected:
	// Collider

	CDB::COLLIDER geom_DB;

	CDB::MODEL* geom_SOM;
	CDB::MODEL* geom_MODEL;

	// Containers
	xr_vector<CSoundRender_Source*> s_sources;
	xr_vector<CSoundRender_Emitter*> s_emitters;
	u32 s_emitters_u; // emitter update marker
	xr_vector<CSoundRender_Target*> s_targets;
	xr_vector<CSoundRender_Target*> s_targets_defer;
	u32 s_targets_pu; // parameters update

	int m_iPauseCounter;

  public:
	// Cache
	CSoundRender_Cache cache;
	u32 cache_bytes_per_line;

  protected:
	virtual void i_eax_set(const GUID* guid, u32 prop, void* val, u32 sz) = 0;
	virtual void i_eax_get(const GUID* guid, u32 prop, void* val, u32 sz) = 0;

  public:
	CSoundRender_Core();
	virtual ~CSoundRender_Core();

	// General
	virtual void _initialize(u64 window) = 0;
	virtual void _clear() = 0;
	virtual void _restart();

	// Sound interface
	virtual void create(ref_sound& S, LPCSTR fName, esound_type sound_type, int game_type);
	virtual void attach_tail(ref_sound& S, LPCSTR fName);

	virtual void clone(ref_sound& S, const ref_sound& from, esound_type sound_type, int game_type);
	virtual void destroy(ref_sound& S);
	virtual void stop_emitters();
	virtual int pause_emitters(bool val);

	virtual void play(ref_sound& S, CObject* O, u32 flags = 0, float delay = 0.f);
	virtual void play_at_pos(ref_sound& S, CObject* O, const Fvector& pos, u32 flags = 0, float delay = 0.f);
	virtual void play_no_feedback(ref_sound& S, CObject* O, u32 flags = 0, float delay = 0.f, Fvector* pos = 0, float* vol = 0, float* freq = 0, Fvector2* range = 0);
	virtual void set_master_volume(float f) = 0;
	virtual void set_geometry_som(IReader* I);
	virtual void set_geometry_occ(CDB::MODEL* M);
	virtual void set_handler(sound_event* E);

	virtual void update(const Fvector& P, const Fvector& D, const Fvector& N);
	virtual void update_events();
	virtual void statistic(CSound_stats* dest, CSound_stats_ext* ext);

	void refresh_sources();

	virtual void update_listener(const Fvector& P, const Fvector& D, const Fvector& N, float dt) = 0;

	void InitializeEAX();
	void UpdateEAX();
	void commit_eax(SEAXEnvironmentData* EAXEnvData);

  public:
	CSoundRender_Source* i_create_source(LPCSTR name);
	void i_destroy_source(CSoundRender_Source* S);
	CSoundRender_Emitter* i_play(ref_sound* S, BOOL _loop, float delay);
	void i_start(CSoundRender_Emitter* E);
	void i_stop(CSoundRender_Emitter* E);
	void i_rewind(CSoundRender_Emitter* E);
	BOOL i_allow_play(CSoundRender_Emitter* E);
	virtual BOOL i_locked()
	{
		return bLocked;
	}

	virtual void object_relcase(CObject* obj);

	virtual float get_occlusion_to(const Fvector& hear_pt, const Fvector& snd_pt, float dispersion = 0.2f);
	float get_occlusion(Fvector& P, float R, Fvector* occ);

	SEAXEnvironmentData m_EAXEnvData;
	bool b_EAXUpdated;

	void set_device_pause_state(bool paused)
	{
		bDevicePaused = paused;
	};
};
extern CSoundRender_Core* SoundRender;
#endif
