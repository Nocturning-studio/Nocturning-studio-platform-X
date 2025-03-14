#pragma once
#include "../state.h"
#include "state_data.h"

template <typename _Object> class CStateMonsterCustomAction : public CState<_Object>
{
	typedef CState<_Object> inherited;

	SStateDataAction data;

  public:
	CStateMonsterCustomAction(_Object* obj);
	virtual ~CStateMonsterCustomAction();

	virtual void execute();
	virtual bool check_completion();
};

#include "state_custom_action_inline.h"
