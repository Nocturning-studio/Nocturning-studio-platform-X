#ifndef SoundRender_TargetH
#define SoundRender_TargetH
#pragma once

#include "soundrender.h"

class CSoundRender_Target
{
  protected:
	CSoundRender_Emitter* pEmitter;
	BOOL rendering;

  public:
	float priority;

  protected:
	OggVorbis_File ovf;
	IReader* wave;
	void attach();
	void dettach();

  public:
	OggVorbis_File* get_data()
	{
		if (!wave)
			attach();
		return &ovf;
	}

  public:
	CSoundRender_Target(void);
	virtual ~CSoundRender_Target(void);

	CSoundRender_Emitter* get_emitter()
	{
		return pEmitter;
	}
	BOOL get_Rendering()
	{
		return rendering;
	}

	virtual BOOL _initialize() = 0;
	virtual void _destroy() = 0;

	virtual void start(CSoundRender_Emitter* E) = 0;
	virtual void render() = 0;
	virtual void rewind() = 0;
	virtual void stop() = 0;
	virtual void update() = 0;
	virtual void fill_parameters() = 0;
};
#endif
