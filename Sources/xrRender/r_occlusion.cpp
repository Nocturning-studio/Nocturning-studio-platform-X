#include "StdAfx.h"
#include ".\r_occlusion.h"

R_occlusion::R_occlusion(void)
{
	OPTICK_EVENT("R_occlusion::R_occlusion");

	enabled = ps_render_flags.test(RFLAG_EXP_HW_OCC);
}

R_occlusion::~R_occlusion(void)
{
	OPTICK_EVENT("R_occlusion::~R_occlusion");

	occq_destroy();
}

void R_occlusion::occq_create(u32 limit)
{
	OPTICK_EVENT("R_occlusion::occq_create");

	pool.reserve(limit);
	used.reserve(limit);
	fids.reserve(limit);
	for (u32 it = 0; it < limit; it++)
	{
		_Q q;
		q.order = it;
		if (FAILED(HW.pDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q.Q)))
			break;
		pool.push_back(q);
	}
	std::reverse(pool.begin(), pool.end());
}

void R_occlusion::occq_destroy()
{
	OPTICK_EVENT("R_occlusion::occq_destroy");

	while (!used.empty())
	{
		try
		{
			_RELEASE(used.back().Q);
			used.pop_back();
		}
		catch (...)
		{
			Msg("! Failed to release OCCq `used`");
			FlushLog();
		}
	}

	while (!pool.empty())
	{
		try
		{
			_RELEASE(pool.back().Q);
			pool.pop_back();
		}
		catch (...)
		{
			Msg("! Failed to release OCCq `pool`");
			FlushLog();
		}
	}
	used.clear();
	pool.clear();
	fids.clear();
}

u32 R_occlusion::occq_begin(u32& ID)
{
	OPTICK_EVENT("R_occlusion::occq_begin");

	if (!enabled)
		return 0;

	RenderImplementation.stats.o_queries++;
	if (!fids.empty())
	{
		ID = fids.back();
		fids.pop_back();
		VERIFY(pool.size());
		used[ID] = pool.back();
	}
	else
	{
		ID = used.size();
		VERIFY(pool.size());
		used.push_back(pool.back());
	}
	pool.pop_back();
	CHK_DX(used[ID].Q->Issue(D3DISSUE_BEGIN));

	// Msg				("begin: [%2d] - %d", used[ID].order, ID);

	return used[ID].order;
}
void R_occlusion::occq_end(u32& ID)
{
	OPTICK_EVENT("R_occlusion::occq_end");

	if (!enabled)
		return;

	// Msg				("end  : [%2d] - %d", used[ID].order, ID);
	CHK_DX(used[ID].Q->Issue(D3DISSUE_END));
}

u32 R_occlusion::occq_get(u32& ID)
{
	OPTICK_EVENT("R_occlusion::occq_get");

	if (!enabled)
		return 0xffffffff;

	DWORD fragments = 0;
	HRESULT hr;
	// CHK_DX		(used[ID].Q->GetData(&fragments,sizeof(fragments),D3DGETDATA_FLUSH));
	// Msg			("get  : [%2d] - %d => %d", used[ID].order, ID, fragments);
	CTimer T;
	T.Start();
	Device.Statistic->RenderDUMP_Wait.Begin();
	while ((hr = used[ID].Q->GetData(&fragments, sizeof(fragments), D3DGETDATA_FLUSH)) == S_FALSE)
	{
		if (!SwitchToThread())
			Sleep(ps_r_thread_wait_sleep);
		if (T.GetElapsed_ms() > 500)
		{
			fragments = 0xffffffff;
			break;
		}
	}
	Device.Statistic->RenderDUMP_Wait.End();
	if (hr == D3DERR_DEVICELOST)
		fragments = 0xffffffff;

	if (0 == fragments)
		RenderImplementation.stats.o_culled++;

	// insert into pool (sorting in decreasing order)
	_Q& Q = used[ID];
	if (pool.empty())
		pool.push_back(Q);
	else
	{
		int it = int(pool.size()) - 1;
		while ((it >= 0) && (pool[it].order < Q.order))
			it--;
		pool.insert(pool.begin() + it + 1, Q);
	}

	// remove from used and shrink as nescessary
	used[ID].Q = 0;
	fids.push_back(ID);
	ID = 0;
	return fragments;
}
