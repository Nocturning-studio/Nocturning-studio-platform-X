////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_detail_path_manager.cpp
//	Created 	: 01.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster detail path manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_monster_detail_path_manager.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "game_graph.h"
#include "level_graph.h"
#include "game_level_cross_table.h"
#include "alife_smart_terrain_task.h"
#include "alife_graph_registry.h"
#include "graph_engine.h"
#include "alife_monster_brain.h"

CALifeMonsterDetailPathManager::CALifeMonsterDetailPathManager(object_type* object)
{
	VERIFY(object);
	m_object = object;
	m_last_update_time = 0;
	m_destination.m_game_vertex_id = this->object().m_tGraphID;
	m_destination.m_level_vertex_id = this->object().m_tNodeID;
	m_destination.m_position = this->object().o_Position;
	m_walked_distance = 0.f;
}

void CALifeMonsterDetailPathManager::target(const GameGraph::_GRAPH_ID& game_vertex_id, const u32& level_vertex_id,
											const Fvector& position)
{
	VERIFY(ai().game_graph().valid_vertex_id(game_vertex_id));
	VERIFY((ai().game_graph().vertex(game_vertex_id)->level_id() != ai().alife().graph().level().level_id()) ||
		   ai().level_graph().valid_vertex_id(level_vertex_id));
	VERIFY((ai().game_graph().vertex(game_vertex_id)->level_id() != ai().alife().graph().level().level_id()) ||
		   (ai().cross_table().vertex(level_vertex_id).game_vertex_id() == game_vertex_id));
	VERIFY((ai().game_graph().vertex(game_vertex_id)->level_id() != ai().alife().graph().level().level_id()) ||
		   ai().level_graph().inside(level_vertex_id, position));

	m_destination.m_game_vertex_id = game_vertex_id;
	m_destination.m_level_vertex_id = level_vertex_id;
	m_destination.m_position = position;

	//	Msg
	//("[%6d][%s][%f][%f][%f]",Device.dwTimeGlobal,object().name_replace(),VPUSH(m_destination.m_position));
}

void CALifeMonsterDetailPathManager::target(const GameGraph::_GRAPH_ID& game_vertex_id)
{
	VERIFY(ai().game_graph().valid_vertex_id(game_vertex_id));
	target(game_vertex_id, ai().game_graph().vertex(game_vertex_id)->level_vertex_id(),
		   ai().game_graph().vertex(game_vertex_id)->level_point());
}

void CALifeMonsterDetailPathManager::target(const CALifeSmartTerrainTask& task)
{
	target(task.game_vertex_id(), task.level_vertex_id(), task.position());
}

void CALifeMonsterDetailPathManager::target(const CALifeSmartTerrainTask* task)
{
	//	Msg
	//("[%6d][%s][%s]",Device.dwTimeGlobal,object().name_replace(),*task->patrol_path_name());
	target(*task);
}

bool CALifeMonsterDetailPathManager::completed() const
{
	if (m_destination.m_game_vertex_id != object().m_tGraphID)
		return (false);

	if (m_destination.m_level_vertex_id != object().m_tNodeID)
		return (false);

	return (true);
}

bool CALifeMonsterDetailPathManager::actual() const
{
	if (failed())
		return (false);

	if (m_destination.m_game_vertex_id != m_path.front())
		return (false);

	return (true);
}

bool CALifeMonsterDetailPathManager::failed() const
{
	return (m_path.empty());
}

void CALifeMonsterDetailPathManager::update()
{
	ALife::_TIME_ID current_time = ai().alife().time_manager().game_time();
	if (current_time <= m_last_update_time)
		return;

	//	if (ai().game_graph().vertex(object().m_tGraphID)->level_id() == ai().level_graph().level_id())
	//		Msg							("[detail::update][%6d][%s]",Device.dwTimeGlobal,object().name_replace());

	ALife::_TIME_ID time_delta = current_time - m_last_update_time;
	update(time_delta);
	// we advisedly "lost" time we need to process a query to avoid some undesirable effects
	m_last_update_time = ai().alife().time_manager().game_time();
}

void CALifeMonsterDetailPathManager::actualize()
{
	m_path.clear();

	typedef GraphEngineSpace::CGameVertexParams CGameVertexParams;
	CGameVertexParams temp = CGameVertexParams(object().m_tpaTerrain);
	bool failed = !ai().graph_engine().search(ai().game_graph(), object().m_tGraphID, m_destination.m_game_vertex_id,
											  &m_path, temp);

#ifdef DEBUG
	if (failed)
	{
		Msg("! %s couldn't build game path from", object().name_replace());
		{
			const CGameGraph::CVertex* vertex = ai().game_graph().vertex(object().m_tGraphID);
			Msg("! [%d][%s][%f][%f][%f]", object().m_tGraphID,
				*ai().game_graph().header().level(vertex->level_id()).name(), VPUSH(vertex->level_point()));
		}
		{
			const CGameGraph::CVertex* vertex = ai().game_graph().vertex(m_destination.m_game_vertex_id);
			Msg("! [%d][%s][%f][%f][%f]", m_destination.m_game_vertex_id,
				*ai().game_graph().header().level(vertex->level_id()).name(), VPUSH(vertex->level_point()));
		}
	}
#endif
	if (failed)
		return;

	VERIFY(!m_path.empty());

	if (m_path.size() == 1)
	{
		VERIFY(m_path.back() == object().m_tGraphID);
		return;
	}

	m_walked_distance = 0.f;
	std::reverse(m_path.begin(), m_path.end());
	VERIFY(m_path.back() == object().m_tGraphID);
}

void CALifeMonsterDetailPathManager::update(const ALife::_TIME_ID& time_delta)
{
	// first update has enormous time delta, therefore just skip it
	if (!m_last_update_time)
		return;

	if (completed())
		return;

	if (!actual())
	{
		actualize();

		if (failed())
			return;
	}

	follow_path(time_delta);
}

void CALifeMonsterDetailPathManager::setup_current_speed()
{
	if (ai().game_graph().vertex(object().m_tGraphID)->level_id() == ai().level_graph().level_id())
		speed(object().m_fCurrentLevelGoingSpeed);
	else
		speed(object().m_fGoingSpeed);
}

void CALifeMonsterDetailPathManager::follow_path(const ALife::_TIME_ID& time_delta)
{
	VERIFY(!completed());
	VERIFY(!failed());
	VERIFY(actual());
	VERIFY(!m_path.empty());
	VERIFY(m_path.back() == object().m_tGraphID);

	if (m_path.size() == 1)
	{
		VERIFY(object().m_tGraphID == m_destination.m_game_vertex_id);
		m_walked_distance = 0.f;
		object().m_tNodeID = m_destination.m_level_vertex_id;
		object().o_Position = m_destination.m_position;
#ifdef DEBUG
		object().m_fDistanceFromPoint = 0.f;
		object().m_fDistanceToPoint = 0.f;
		object().m_tNextGraphID = object().m_tGraphID;
#endif
		return;
	}

	float last_time_delta = float(time_delta) / 1000.f;
	for (; m_path.size() > 1;)
	{
		setup_current_speed();
		float update_distance = (last_time_delta / ai().alife().time_manager().normal_time_factor()) * speed();

		float distance_between =
			ai().game_graph().distance(object().m_tGraphID, (GameGraph::_GRAPH_ID)m_path[m_path.size() - 2]);
		if (distance_between > (update_distance + m_walked_distance))
		{
			m_walked_distance += update_distance;
#ifdef DEBUG
			object().m_fDistanceFromPoint = m_walked_distance;
			object().m_fDistanceToPoint = distance_between;
			object().m_tNextGraphID = (GameGraph::_GRAPH_ID)m_path[m_path.size() - 2];
#endif
			return;
		}

		update_distance += m_walked_distance;
		update_distance -= distance_between;

		last_time_delta = update_distance * ai().alife().time_manager().normal_time_factor() / speed();

		m_walked_distance = 0.f;
		m_path.pop_back();
		//		Msg									("%6d %s changes graph point from %d to
		//%d",Device.dwTimeGlobal,object().name_replace(),object().m_tGraphID,(GameGraph::_GRAPH_ID)m_path.back());
		object().alife().graph().change(&object(), object().m_tGraphID, (GameGraph::_GRAPH_ID)m_path.back());
		VERIFY(m_path.back() == object().m_tGraphID);
		object().brain().on_location_change();
		VERIFY(m_path.back() == object().m_tGraphID);
	}
}

void CALifeMonsterDetailPathManager::on_switch_online()
{
	m_path.clear();
}

void CALifeMonsterDetailPathManager::on_switch_offline()
{
	m_path.clear();
}

Fvector CALifeMonsterDetailPathManager::draw_level_position() const
{
	if (path().empty())
		return (object().Position());

	u32 path_size = path().size();
	if (path_size == 1)
		return (object().Position());

	VERIFY(m_path.back() == object().m_tGraphID);

	const GameGraph::CVertex* current = ai().game_graph().vertex(object().m_tGraphID);
	const GameGraph::CVertex* next = ai().game_graph().vertex(m_path[path_size - 2]);
	if (current->level_id() != next->level_id())
		return (object().Position());

	Fvector current_vertex = current->level_point();
	Fvector next_vertex = next->level_point();
	Fvector direction = Fvector().sub(next_vertex, current_vertex);
	direction.normalize();
	return (current_vertex.mad(direction, walked_distance()));
}
