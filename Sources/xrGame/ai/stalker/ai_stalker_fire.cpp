////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_fire.cpp
//	Created 	: 25.02.2003
//  Modified 	: 25.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Fire and enemy parameters for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "ai_stalker_impl.h"
#include "../../script_entity_action.h"
#include "../../inventory.h"
#include "../../ef_storage.h"
#include "../../stalker_decision_space.h"
#include "../../script_game_object.h"
#include "../../customzone.h"
#include "../../../xrEngine/skeletonanimated.h"
#include "../../agent_manager.h"
#include "../../stalker_animation_manager.h"
#include "../../stalker_planner.h"
#include "../../ef_pattern.h"
#include "../../memory_manager.h"
#include "../../hit_memory_manager.h"
#include "../../enemy_manager.h"
#include "../../item_manager.h"
#include "../../stalker_movement_manager.h"
#include "../../entitycondition.h"
#include "../../sound_player.h"
#include "../../cover_point.h"
#include "../../agent_member_manager.h"
#include "../../agent_location_manager.h"
#include "../../danger_cover_location.h"
#include "../../object_handler_planner.h"
#include "../../object_handler_space.h"
#include "../../visual_memory_manager.h"
#include "../../weapon.h"
#include "ai_stalker_space.h"
#include "../../effectorshot.h"
#include "../../BoneProtections.h"
#include "../../RadioactiveZone.h"
#include "../../restricted_object.h"
#include "../../ai_object_location.h"
#include "../../missile.h"
#include "../../phworld.h"
#include "../../stalker_animation_names.h"
#include "../../agent_corpse_manager.h"
#include "../../CharacterPhysicsSupport.h"
#include "../../stalker_planner.h"
#include "../../stalker_decision_space.h"
#include "../../script_game_object.h"
#include "../../inventory.h"

using namespace StalkerSpace;

const float DANGER_DISTANCE = 3.f;
const u32 DANGER_INTERVAL = 120000;

const float PRECISE_DISTANCE = 2.5f;
const float FLOOR_DISTANCE = 2.f;
const float NEAR_DISTANCE = 2.5f;
const u32 FIRE_MAKE_SENSE_INTERVAL = 10000;

#define HIT_FRONT 0
#define HIT_BACK 1
#define HIT_LEFT 2
#define HIT_RIGHT 3
#define HIT_UNKNOWN 4

// Вспомогательная функция для определения направления попадания
int GetHitDirection(const Fvector& hit_dir_local)
{
	if (_abs(hit_dir_local.z) > _abs(hit_dir_local.x))
	{
		return (hit_dir_local.z > 0) ? HIT_FRONT : HIT_BACK;
	}
	else
	{
		return (hit_dir_local.x > 0) ? HIT_RIGHT : HIT_LEFT;
	}
}

float CAI_Stalker::GetWeaponAccuracy() const
{
	float base = PI / 180.f;

	// влияние ранга на меткость (базовое значение движка)
	base *= m_fRankDisperison;

	// [IMPROVEMENT] 1. Panic Spray: Если паника, стреляем "в молоко"
	if (movement().mental_state() == eMentalStatePanic)
		base *= 3.0f;

	// [IMPROVEMENT] 2. Flinch: Штраф за недавнее ранение (действует 1 сек)
	if (Device.dwTimeGlobal - m_dwLastHitTime < 1000)
		base *= 5.0f;

	// [IMPROVEMENT] 3. Health Impact: Чем меньше здоровья, тем хуже точность
	float health = conditions().health();
	if (health < 0.9f)
		base *= (1.0f + (1.0f - health) * 2.0f);

	// [IMPROVEMENT] 4. Turn Penalty: Штраф за резкий разворот корпуса
	float yaw_diff = _abs(angle_difference(movement().m_body.current.yaw, m_previous_yaw));
	if (yaw_diff > PI_DIV_6)
		base *= 2.5f;

	// [IMPROVEMENT] 5. Advanced Zeroing In (Пристрелка + Ранг + Дистанция + Вероятность)
	const CEntityAlive* enemy = memory().enemy().selected();
	if (enemy)
	{
		// FIX CRASH: Используем memory().memory() вместо visual().visible_object()
		// memory() возвращает информацию о враге независимо от того, видим мы его или слышим.
		const MemorySpace::CMemoryInfo* mem_info = &memory().memory(enemy);

		if (mem_info)
		{
			// --- ДАННЫЕ ---
			// m_level_time - это время последнего обновления информации (звук, вид или хит)
			u32 time_since_update = Device.dwTimeGlobal - mem_info->m_level_time;

			// Позиция берется из общих параметров памяти
			float dist = Position().distance_to(mem_info->m_object_params.m_position);

			// Ранг сталкера нормализованный (0.0 - Новичок ... 1.0 - Мастер/Легенда)
			float rank_val = float(Rank()) / 100.f;
			clamp(rank_val, 0.f, 1.f);

			// --- 1. СКОРОСТЬ ПРИСТРЕЛКИ ---
			// Используем время с последнего обновления как фактор "свежести" цели
			// Если мы видим/слышим врага ПРЯМО СЕЙЧАС, time_since_update ~ 0.

			// Логика "первого выстрела":
			// Если мы только что заметили врага (или постоянно видим), у нас есть базовый штраф за реакцию.
			// Сделаем простую зависимость от ранга и дистанции.

			// --- 2. ВЕРОЯТНОСТЬ ПОПАСТЬ С ПЕРВОГО РАЗА (Стартовый разброс) ---
			// Коэффициент дистанции: на 5м штрафа нет, на 50м штраф большой
			float dist_penalty = dist / 10.0f;

			// Коэффициент скилла: Мастер (1.0) гасит 80% штрафа дистанции
			float skill_mitigation = 1.0f - (rank_val * 0.8f);

			// Итоговый множитель.
			float accuracy_mult = 1.0f + (dist_penalty * skill_mitigation);

			// Дополнительный бонус, если мы "потеряли" врага из виду пару секунд назад, но продолжаем целиться
			// (префаер)
			if (time_since_update > 1000 && time_since_update < 4000)
			{
				accuracy_mult *= 0.8f; // Бонус за "сведение" в точку, где был враг
			}

			base *= accuracy_mult;
		}
		else
		{
			// Враг выбран, но памяти о нем нет (странная ситуация, но на всякий случай)
			base *= 2.0f;
		}
	}

	if (!movement().path_completed())
	{
		if (movement().movement_type() == eMovementTypeWalk)
			if (movement().body_state() == eBodyStateStand)
				return (base * m_disp_walk_stand);
			else
				return (base * m_disp_walk_crouch);
		else if (movement().movement_type() == eMovementTypeRun)
			if (movement().body_state() == eBodyStateStand)
				return (base * m_disp_run_stand);
			else
				return (base * m_disp_run_crouch);
	}

	if (movement().body_state() == eBodyStateStand)
		if (zoom_state())
			return (base * m_disp_stand_stand);
		else
			return (base * m_disp_stand_stand_zoom);
	else if (zoom_state())
		return (base * m_disp_stand_crouch);
	else
		return (base * m_disp_stand_crouch_zoom);
}

void CAI_Stalker::g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D)
{
	//.	VERIFY				(inventory().ActiveItem());
	if (!inventory().ActiveItem())
	{
#ifdef DEBUG
		Msg("! CAI_Stalker::g_fireParams() : VERIFY(inventory().ActiveItem())");
#endif // DEBUG
		P = Position();
		D = Fvector().set(0.f, 0.f, 1.f);
		return;
	}

	CWeapon* weapon = smart_cast<CWeapon*>(inventory().ActiveItem());
	if (!weapon)
	{
		CMissile* missile = smart_cast<CMissile*>(inventory().ActiveItem());
		if (missile)
		{
			update_throw_params();
			P = m_throw_position;
			D = m_throw_direction;
			VERIFY(!fis_zero(D.square_magnitude()));
			return;
		}
		P = eye_matrix.c;
		D = eye_matrix.k;
		if (weapon_shot_effector().IsActive())
			D = weapon_shot_effector_direction(D);
		VERIFY(!fis_zero(D.square_magnitude()));
		return;
	}

	if (!g_Alive() || !animation().script_animations().empty())
	{
		P = weapon->get_LastFP();
		D = weapon->get_LastFD();
		VERIFY(!fis_zero(D.square_magnitude()));
		return;
	}

	// [IMPROVEMENT] Suppression Fire: Исправленная версия
	const CEntityAlive* enemy = memory().enemy().selected();
	bool suppress_fire = false;
	Fvector enemy_last_pos = {0, 0, 0};

	// Если враг выбран, но мы его не видим прямо сейчас
	if (enemy && !memory().visual().visible_right_now(enemy))
	{
		// Получаем информацию о враге из "памяти" сталкера
		// Метод memory(object) возвращает структуру со всей инфой (последняя позиция, время и т.д.)
		const MemorySpace::CMemoryInfo* mem_info = &memory().memory(enemy);

		if (mem_info)
		{
			suppress_fire = true;
			// Берем позицию, где мы в последний раз засекли врага
			enemy_last_pos = mem_info->m_object_params.m_position;

			// Небольшой рандом для огня на подавление, чтобы создать разброс по площади
			enemy_last_pos.x += ::Random.randF(-0.5f, 0.5f);
			enemy_last_pos.y += ::Random.randF(0.0f, 1.0f); // Чуть выше, на уровень груди/головы
		}
	}

	switch (movement().body_state())
	{
	case eBodyStateStand: {
		if (movement().movement_type() == eMovementTypeStand)
		{
			P = eye_matrix.c;
			D = eye_matrix.k;

			// Переопределяем вектор, если подавляем
			if (suppress_fire)
			{
				// Стреляем от глаз (P) в сторону последней позиции врага
				D.sub(enemy_last_pos, P).normalize();
			}

			if (weapon_shot_effector().IsActive())
				D = weapon_shot_effector_direction(D);
			VERIFY(!fis_zero(D.square_magnitude()));
		}
		else
		{
			D.setHP(-movement().m_head.current.yaw, -movement().m_head.current.pitch);

			if (suppress_fire)
			{
				// Коррекция для движения + подавления
				Fvector fire_dir;
				fire_dir.sub(enemy_last_pos, Position()).normalize();
				D = fire_dir;
			}

			if (weapon_shot_effector().IsActive())
				D = weapon_shot_effector_direction(D);
			Center(P);
			P.mad(D, .5f);
			P.y += .50f;
			//				P		= weapon->get_LastFP();
			//				D		= weapon->get_LastFD();
			VERIFY(!fis_zero(D.square_magnitude()));
		}
		return;
	}
	case eBodyStateCrouch: {
		P = eye_matrix.c;
		D = eye_matrix.k;

		if (suppress_fire)
		{
			D.sub(enemy_last_pos, P).normalize();
		}

		if (weapon_shot_effector().IsActive())
			D = weapon_shot_effector_direction(D);
		VERIFY(!fis_zero(D.square_magnitude()));
		return;
	}
	default:
		NODEFAULT;
	}

#ifdef DEBUG
	P = weapon->get_LastFP();
	D = weapon->get_LastFD();
	VERIFY(!fis_zero(D.square_magnitude()));
#endif
}

void CAI_Stalker::g_WeaponBones(int& L, int& R1, int& R2)
{
	int r_hand, r_finger2, l_finger1;
	CObjectHandler::weapon_bones(r_hand, r_finger2, l_finger1);
	R1 = r_hand;
	R2 = r_finger2;
	if (!animation().script_animations().empty() && animation().script_animations().front().hand_usage())
		L = R2;
	else
		L = l_finger1;
}

void CAI_Stalker::Hit(SHit* pHDS)
{
	if (invulnerable())
		return;

	// [IMPROVEMENT] Flinch: Запоминаем время попадания
	m_dwLastHitTime = Device.dwTimeGlobal;

	// хит может меняться в зависимости от ранга (новички получают больше хита, чем ветераны)
	SHit HDS = *pHDS;
	HDS.power *= m_fRankImmunity;
	if (m_boneHitProtection && HDS.hit_type == ALife::eHitTypeFireWound)
	{
		float BoneArmour = m_boneHitProtection->getBoneArmour(HDS.bone());
		float NewHitPower = HDS.damage() - BoneArmour;
		if (NewHitPower < HDS.power * m_boneHitProtection->m_fHitFrac)
			HDS.power = HDS.power * m_boneHitProtection->m_fHitFrac;
		else
			HDS.power = NewHitPower;

		if (wounded())
			HDS.power = 1000.f;
	}

	if (g_Alive())
	{
		bool already_critically_wounded = critically_wounded();

		if (!already_critically_wounded)
		{
			const CCoverPoint* cover = agent_manager().member().member(this).cover();
			if (cover && pHDS->initiator() && (pHDS->initiator()->ID() != ID()) && !fis_zero(pHDS->damage()) &&
				brain().affect_cover())
				agent_manager().location().add(
					xr_new<CDangerCoverLocation>(cover, Device.dwTimeGlobal, DANGER_INTERVAL, DANGER_DISTANCE));
		}

		const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(pHDS->initiator());
		if (entity_alive && !wounded())
		{
			if (is_relation_enemy(entity_alive))
				sound().play(eStalkerSoundInjuring);
		}

		int weapon_type = -1;
		if (best_weapon())
			weapon_type = best_weapon()->object().ef_weapon_type();

		if (!wounded() && !already_critically_wounded)
		{
			bool became_critically_wounded = update_critical_wounded(HDS.boneID, HDS.power);
			if (!became_critically_wounded && animation().script_animations().empty() && (pHDS->bone() != BI_NONE))
			{
				// Проверяем, что визуал существует и является анимированным
				if (!Visual())
				{
					inherited::Hit(&HDS);
					return;
				}

				CKinematicsAnimated* tpKinematics = smart_cast<CKinematicsAnimated*>(Visual());
				if (!tpKinematics)
				{
					inherited::Hit(&HDS);
					return;
				}

				// ПРОВЕРКА БЕЗОПАСНОСТИ: убедимся, что кость существует
				if (pHDS->bone() >= tpKinematics->LL_BoneCount())
				{
#ifdef DEBUG
					Msg("! WARNING: tpKinematics has no bone_id %d", pHDS->bone());
					pHDS->_dump();
#endif
					inherited::Hit(&HDS);
					return;
				}

				// Преобразуем направление попадания в локальные координаты модели
				Fvector local_hit_dir;
				XFORM().transform_dir(local_hit_dir, pHDS->direction());
				local_hit_dir.normalize();

				// Определяем направление попадания
				int hit_dir = GetHitDirection(local_hit_dir);

				float power_factor = m_power_fx_factor * pHDS->damage() / 100.f;
				clamp(power_factor, 0.f, 1.f);

				// БЕЗОПАСНОЕ ПОЛУЧЕНИЕ БАЗОВОГО ИНДЕКСА
				float base_param = tpKinematics->LL_GetBoneInstance(pHDS->bone()).get_param(1);
				int base_fx_index = iFloor(base_param);

				// ПРОВЕРКА: если базовый индекс невалиден, не воспроизводим анимацию
				if (base_fx_index == -1)
				{
					inherited::Hit(&HDS);
					return;
				}

				// Используем правильное определение направления для выбора анимации
				int direction_offset = 0;
				if (hit_dir == HIT_FRONT)
					direction_offset = 0;
				else if (hit_dir == HIT_BACK)
					direction_offset = 1;
				else if (hit_dir == HIT_LEFT)
					direction_offset = 2;
				else if (hit_dir == HIT_RIGHT)
					direction_offset = 3;
				else
					direction_offset = 0;

				int fx_index = base_fx_index + direction_offset;

				animation().play_fx(power_factor, fx_index);
#ifdef DEBUG
				else
				{
					Msg("! WARNING: Invalid fx_index %d (max: %d) for bone %d, body_state %d", fx_index, max_fx_index,
						pHDS->bone(), body_state);
				}
#endif
			}
			else
			{
				if (!already_critically_wounded && became_critically_wounded)
				{
					if (HDS.who)
					{
						CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(HDS.who);
						if (stalker)
							stalker->on_critical_wound_initiator(this);
					}
				}
			}
		}
	}

	inherited::Hit(&HDS);
}

void CAI_Stalker::HitSignal(float amount, Fvector& vLocalDir, CObject* who, s16 element)
{
	if (getDestroy())
		return;

	if (g_Alive())
		memory().hit().add(amount, vLocalDir, who, element);
}

void CAI_Stalker::OnItemTake(CInventoryItem* inventory_item)
{
	CObjectHandler::OnItemTake(inventory_item);
	m_item_actuality = false;
	m_sell_info_actuality = false;
}

void CAI_Stalker::OnItemDrop(CInventoryItem* inventory_item, bool just_before_destroy)
{
	CObjectHandler::OnItemDrop(inventory_item, just_before_destroy);

	m_item_actuality = false;
	m_sell_info_actuality = false;

	if (!g_Alive())
		return;

	if (!critically_wounded())
		return;

	//	VERIFY						(inventory().ActiveItem());

	if (inventory().ActiveItem() && (inventory().ActiveItem() != inventory_item))
		return;

	brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, false);
}

void CAI_Stalker::update_best_item_info()
{
	ai().ef_storage().alife_evaluation(false);

	// [IMPROVEMENT] Smart Weapon Choice: Получаем дистанцию до врага
	float dist_to_enemy = 1000.f; // По умолчанию далеко
	if (memory().enemy().selected())
	{
		dist_to_enemy = Position().distance_to(memory().enemy().selected()->Position());
	}

	if (m_item_actuality && m_best_item_to_kill && m_best_item_to_kill->can_kill())
	{

		if (!memory().enemy().selected())
			return;

		ai().ef_storage().non_alife().member() = this;
		ai().ef_storage().non_alife().enemy() = memory().enemy().selected() ? memory().enemy().selected() : this;
		ai().ef_storage().non_alife().member_item() = &m_best_item_to_kill->object();
		float value;
		value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();

		// Коррекция ценности текущего оружия по дистанции
		int weapon_type = m_best_item_to_kill->object().ef_weapon_type();
		if (dist_to_enemy < 15.f && weapon_type >= 7)
			value *= 2.0f; // Дробовики/Пистолеты (обычно типы 7, 8) в упор
		if (dist_to_enemy > 50.f && weapon_type >= 7)
			value *= 0.1f; // Дробовики далеко - мусор
		if (dist_to_enemy < 10.f && weapon_type == 5)
			value *= 0.5f; // Снайперки (тип 5) в упор - плохо

		if (fsimilar(value, m_best_item_value))
			return;
	}

	// initialize parameters
	m_item_actuality = true;
	ai().ef_storage().non_alife().member() = this;
	ai().ef_storage().non_alife().enemy() = memory().enemy().selected() ? memory().enemy().selected() : this;
	m_best_item_to_kill = 0;
	m_best_ammo = 0;
	m_best_found_item_to_kill = 0;
	m_best_found_ammo = 0;
	m_best_item_value = 0.f;

	// try to find the best item which can kill
	{
		TIItemContainer::iterator I = inventory().m_all.begin();
		TIItemContainer::iterator E = inventory().m_all.end();
		for (; I != E; ++I)
		{
			if ((*I)->can_kill())
			{
				ai().ef_storage().non_alife().member_item() = &(*I)->object();
				float value;
				if (memory().enemy().selected())
					value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
				else
					value = (float)(*I)->object().ef_weapon_type();

				// [IMPROVEMENT] Smart Weapon Choice: Применяем ту же логику коррекции
				int w_type = (*I)->object().ef_weapon_type();
				if (dist_to_enemy < 15.f && w_type >= 7)
					value *= 2.0f;
				if (dist_to_enemy > 50.f && w_type >= 7)
					value *= 0.1f;
				if (dist_to_enemy < 10.f && w_type == 5)
					value *= 0.5f;

				if (!fsimilar(value, m_best_item_value) && (value < m_best_item_value))
					continue;

				if (!fsimilar(value, m_best_item_value) && (value > m_best_item_value))
				{
					m_best_item_value = value;
					m_best_item_to_kill = *I;
					continue;
				}

				VERIFY(fsimilar(value, m_best_item_value));
				if (m_best_item_to_kill &&
					((*I)->object().ef_weapon_type() <= m_best_item_to_kill->object().ef_weapon_type()))
					continue;

				m_best_item_value = value;
				m_best_item_to_kill = *I;
			}
		}
	}

	// check if we found
	if (m_best_item_to_kill)
	{
		m_best_ammo = m_best_item_to_kill;
		return;
	}

	// so we do not have such an item
	// check if we remember we saw item which can kill
	// or items which can make my item killing
	{
		xr_vector<const CGameObject*>::const_iterator I = memory().item().objects().begin();
		xr_vector<const CGameObject*>::const_iterator E = memory().item().objects().end();
		for (; I != E; ++I)
		{
			const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
			if (!inventory_item || !memory().item().useful(&inventory_item->object()))
				continue;
			CInventoryItem* item = inventory_item->can_kill(&inventory());
			if (item)
			{
				ai().ef_storage().non_alife().member_item() = &inventory_item->object();
				float value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
				if (value > m_best_item_value)
				{
					m_best_item_value = value;
					m_best_found_item_to_kill = inventory_item;
					m_best_found_ammo = 0;
					m_best_ammo = item;
				}
			}
			else
			{
				item = inventory_item->can_make_killing(&inventory());
				if (!item)
					continue;

				ai().ef_storage().non_alife().member_item() = &item->object();
				float value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
				if (value > m_best_item_value)
				{
					m_best_item_value = value;
					m_best_item_to_kill = item;
					m_best_found_item_to_kill = 0;
					m_best_found_ammo = inventory_item;
				}
			}
		}
	}

	// check if we found such an item
	if (m_best_found_item_to_kill || m_best_found_ammo)
		return;

	// check if we remember we saw item to kill
	// and item which can make this item killing
	xr_vector<const CGameObject*>::const_iterator I = memory().item().objects().begin();
	xr_vector<const CGameObject*>::const_iterator E = memory().item().objects().end();
	for (; I != E; ++I)
	{
		const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
		if (!inventory_item || !memory().item().useful(&inventory_item->object()))
			continue;
		const CInventoryItem* item = inventory_item->can_kill(memory().item().objects());
		if (item)
		{
			ai().ef_storage().non_alife().member_item() = &inventory_item->object();
			float value = ai().ef_storage().m_pfWeaponEffectiveness->ffGetValue();
			if (value > m_best_item_value)
			{
				m_best_item_value = value;
				m_best_found_item_to_kill = inventory_item;
				m_best_found_ammo = item;
			}
		}
	}
}

bool CAI_Stalker::item_to_kill()
{
	update_best_item_info();
	return (!!m_best_item_to_kill);
}

bool CAI_Stalker::item_can_kill()
{
	update_best_item_info();
	return (!!m_best_ammo);
}

bool CAI_Stalker::remember_item_to_kill()
{
	update_best_item_info();
	return (!!m_best_found_item_to_kill);
}

bool CAI_Stalker::remember_ammo()
{
	update_best_item_info();
	return (!!m_best_found_ammo);
}

bool CAI_Stalker::ready_to_kill()
{
	return (m_best_item_to_kill && inventory().ActiveItem() &&
			(inventory().ActiveItem()->object().ID() == m_best_item_to_kill->object().ID()) &&
			m_best_item_to_kill->ready_to_kill());
}

bool CAI_Stalker::ready_to_detour()
{
	if (!ready_to_kill())
		return (false);

	CWeapon* weapon = smart_cast<CWeapon*>(m_best_item_to_kill);
	if (!weapon)
		return (false);

	return (weapon->GetAmmoElapsed() > weapon->GetAmmoMagSize() / 2);
}

namespace xray
{
class ray_query_param
{
  public:
	CAI_Stalker* m_holder;
	float m_power;
	float m_power_threshold;
	bool m_can_kill_enemy;
	bool m_can_kill_member;
	float m_pick_distance;

  public:
	IC ray_query_param(const CAI_Stalker* holder, float power_threshold, float distance)
	{
		m_holder = const_cast<CAI_Stalker*>(holder);
		m_power_threshold = power_threshold;
		m_power = 1.f;
		m_can_kill_enemy = false;
		m_can_kill_member = false;
		m_pick_distance = distance;
	}
};
} // namespace xray

IC BOOL ray_query_callback(collide::rq_result& result, LPVOID params)
{
	xray::ray_query_param* param = (xray::ray_query_param*)params;
	float power = param->m_holder->feel_vision_mtl_transp(result.O, result.element);
	param->m_power *= power;

	//	if (power >= .05f) {
	//		param->m_pick_distance			= result.range;
	//		return							(true);
	//	}

	if (!result.O)
	{
		if (param->m_power > param->m_power_threshold)
			return (true);

		param->m_pick_distance = result.range;
		return (false);
	}

	CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(result.O);
	if (!entity_alive)
	{
		if (param->m_power > param->m_power_threshold)
			return (true);

		param->m_pick_distance = result.range;
		return (false);
	}

	if (param->m_holder->is_relation_enemy(entity_alive))
		param->m_can_kill_enemy = true;
	else
		param->m_can_kill_member = true;

	param->m_pick_distance = result.range;
	return (false);
}

void CAI_Stalker::can_kill_entity(const Fvector& position, const Fvector& direction, float distance,
								  collide::rq_results& rq_storage)
{
	VERIFY(!fis_zero(direction.square_magnitude()));

	collide::ray_defs ray_defs(position, direction, distance, CDB::OPT_CULL, collide::rqtBoth);
	VERIFY(!fis_zero(ray_defs.dir.square_magnitude()));

	xray::ray_query_param params(this, memory().visual().transparency_threshold(), distance);

	Level().ObjectSpace.RayQuery(rq_storage, ray_defs, ray_query_callback, &params, NULL, this);
	m_can_kill_enemy = m_can_kill_enemy || params.m_can_kill_enemy;
	m_can_kill_member = m_can_kill_member || params.m_can_kill_member;
	m_pick_distance = std::max(m_pick_distance, params.m_pick_distance);
}

void CAI_Stalker::can_kill_entity_from(const Fvector& position, Fvector direction, float distance)
{
	m_pick_distance = 0.f;
	rq_storage.r_clear();
	can_kill_entity(position, direction, distance, rq_storage);
	if (m_can_kill_member && m_can_kill_enemy)
		return;

	float yaw, pitch, safety_fire_angle = 1.f * PI_DIV_8 * .5f;
	direction.getHP(yaw, pitch);

	direction.setHP(yaw - safety_fire_angle, pitch);
	can_kill_entity(position, direction, distance, rq_storage);
	if (m_can_kill_member && m_can_kill_enemy)
		return;

	direction.setHP(yaw + safety_fire_angle, pitch);
	can_kill_entity(position, direction, distance, rq_storage);
	if (m_can_kill_member && m_can_kill_enemy)
		return;

	direction.setHP(yaw, pitch - safety_fire_angle);
	can_kill_entity(position, direction, distance, rq_storage);
	if (m_can_kill_member && m_can_kill_enemy)
		return;

	direction.setHP(yaw, pitch + safety_fire_angle);
	can_kill_entity(position, direction, distance, rq_storage);
}

IC float CAI_Stalker::start_pick_distance() const
{
	float result = 50.f;
	if (!memory().enemy().selected())
		return (result);

	return (_max(result, memory().enemy().selected()->Position().distance_to(Position()) + 1.f));
}

float CAI_Stalker::pick_distance()
{
	if (!inventory().ActiveItem())
		return (start_pick_distance());

	update_can_kill_info();
	return (m_pick_distance);
}

void CAI_Stalker::update_can_kill_info()
{
	if (m_pick_frame_id == Device.dwFrame)
		return;

	m_pick_frame_id = Device.dwFrame;
	m_can_kill_member = false;
	m_can_kill_enemy = false;

	Fvector position, direction;
	VERIFY(inventory().ActiveItem());
	g_fireParams(0, position, direction);
	can_kill_entity_from(position, direction, start_pick_distance());

	// [IMPROVEMENT] Anti-Bodyblock: Если мы блокируем стрельбу долгое время
	if (m_can_kill_member && !m_can_kill_enemy)
	{
		m_body_block_time += Device.dwTimeDelta;
	}
	else
	{
		m_body_block_time = 0;
	}
}

bool CAI_Stalker::undetected_anomaly()
{
	return (inside_anomaly() ||
			brain().CStalkerPlanner::m_storage.property(StalkerDecisionSpace::eWorldPropertyAnomaly));
}

bool CAI_Stalker::inside_anomaly()
{
	xr_vector<CObject*>::const_iterator I = feel_touch.begin();
	xr_vector<CObject*>::const_iterator E = feel_touch.end();
	for (; I != E; ++I)
	{
		CCustomZone* zone = smart_cast<CCustomZone*>(*I);
		if (zone)
		{
			if (smart_cast<CRadioactiveZone*>(zone))
				continue;

			return (true);
		}
	}
	return (false);
}

bool CAI_Stalker::zoom_state() const
{
	if (!inventory().ActiveItem())
		return (false);

	if ((movement().movement_type() != eMovementTypeStand) && (movement().body_state() != eBodyStateCrouch) &&
		!movement().path_completed())
		return (false);

	// [IMPROVEMENT] No Scope in CQC: Если враг в упор (ближе 3м), не используем оптику/зум
	if (memory().enemy().selected())
	{
		if (Position().distance_to(memory().enemy().selected()->Position()) < 3.0f)
			return (false);
	}

	switch (CObjectHandler::planner().current_action_state_id())
	{
	case ObjectHandlerSpace::eWorldOperatorAim1:
	case ObjectHandlerSpace::eWorldOperatorAim2:
	case ObjectHandlerSpace::eWorldOperatorAimingReady1:
	case ObjectHandlerSpace::eWorldOperatorAimingReady2:
	case ObjectHandlerSpace::eWorldOperatorQueueWait1:
	case ObjectHandlerSpace::eWorldOperatorQueueWait2:
	case ObjectHandlerSpace::eWorldOperatorFire1:
	// we need this 2 operators to prevent fov/range switching
	case ObjectHandlerSpace::eWorldOperatorReload1:
	case ObjectHandlerSpace::eWorldOperatorReload2:
	case ObjectHandlerSpace::eWorldOperatorForceReload1:
	case ObjectHandlerSpace::eWorldOperatorForceReload2:
		return (true);
	}

	return (false);
}

void CAI_Stalker::update_range_fov(float& new_range, float& new_fov, float start_range, float start_fov)
{
	float range = start_range, fov = start_fov;

	if (zoom_state())
		inventory().ActiveItem()->modify_holder_params(range, fov);

	return (inherited::update_range_fov(new_range, new_fov, range, fov));
}

bool CAI_Stalker::fire_make_sense()
{
	// if we do not have an enemy
	const CEntityAlive* enemy = memory().enemy().selected();
	if (!enemy)
		return (false);

	// [IMPROVEMENT] Panic Spray: Если паника, разрешаем стрелять почти всегда, создавая хаос
	if (movement().mental_state() == eMentalStatePanic)
		return (true);

	// [IMPROVEMENT] Rage Mode: Если в ярости, стреляем агрессивнее
	if (Device.dwTimeGlobal < m_rage_end_time)
		return (true);

	if ((pick_distance() + PRECISE_DISTANCE) < Position().distance_to(enemy->Position()))
		return (false);

	if (_abs(Position().y - enemy->Position().y) > FLOOR_DISTANCE)
		return (false);

	if (pick_distance() < NEAR_DISTANCE)
		return (false);

	if (memory().visual().visible_right_now(enemy))
		return (true);

	u32 last_time_seen = memory().visual().visible_object_time_last_seen(enemy);
	if (last_time_seen == u32(-1))
		return (false);

	// [IMPROVEMENT] Suppression Fire: Увеличиваем время стрельбы по невидимой цели до 5 секунд (было 10, но проверка
	// была жестче) Теперь мы явно разрешаем стрельбу "вслепую" в течение 3 сек после потери контакта
	if (Device.dwTimeGlobal - last_time_seen < 3000)
		return (true);

	if (Device.dwTimeGlobal > last_time_seen + FIRE_MAKE_SENSE_INTERVAL)
		return (false);

	// if we do not have a weapon
	if (!best_weapon())
		return (false);

	// if we do not have automatic weapon
	if (best_weapon()->object().ef_weapon_type() != 6)
		return (false);

	return (true);
}

// shot effector stuff
void CAI_Stalker::on_weapon_shot_start(CWeapon* weapon)
{
	weapon_shot_effector().SetRndSeed(m_weapon_shot_random_seed);
	weapon_shot_effector().Shot(weapon->camDispersion + weapon->camDispersionInc * float(weapon->ShotsFired()));
}

void CAI_Stalker::on_weapon_shot_stop(CWeapon* weapon)
{
}

void CAI_Stalker::on_weapon_hide(CWeapon* weapon)
{
}

void CAI_Stalker::notify_on_wounded_or_killed(CObject* object)
{
	CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(object);
	if (!stalker)
		return;

	stalker->on_enemy_wounded_or_killed(this);

	typedef CAgentCorpseManager::MEMBER_CORPSES MEMBER_CORPSES;

	const MEMBER_CORPSES& corpses = agent_manager().corpse().corpses();
	if (std::find(corpses.begin(), corpses.end(), this) != corpses.end())
		return;

	agent_manager().corpse().register_corpse(this);
}

void CAI_Stalker::notify_on_wounded_or_killed()
{
	ALife::_OBJECT_ID last_hit_object_id = memory().hit().last_hit_object_id();
	if (last_hit_object_id == ALife::_OBJECT_ID(-1))
		return;

	CObject* object = Level().Objects.net_Find(last_hit_object_id);
	if (!object)
		return;

	notify_on_wounded_or_killed(object);
}

void CAI_Stalker::wounded(bool value)
{
	if (m_wounded == value)
		return;

	if (value)
		notify_on_wounded_or_killed();

	m_wounded = value;

	if (!m_wounded && g_Alive())
		character_physics_support()->CreateCharacter();

	if (!m_wounded)
		return;

	character_physics_support()->movement()->DestroyCharacter();

	if (!agent_manager().member().registered_in_combat(this))
		return;

	agent_manager().member().unregister_in_combat(this);
}

bool CAI_Stalker::wounded(const CRestrictedObject* object) const
{
	if (!wounded())
		return (false);

	VERIFY(object);
	if (!object->accessible(Position()))
		return (false);

	if (!object->accessible(ai_location().level_vertex_id()))
		return (false);

	return (true);
}

bool CAI_Stalker::use_default_throw_force()
{
	return (false);
}

bool CAI_Stalker::use_throw_randomness()
{
	return (false);
}

float CAI_Stalker::missile_throw_force()
{
	update_throw_params();
	VERIFY(_valid(m_throw_force));
	return (m_throw_force);
}

void CAI_Stalker::throw_target(const Fvector& position)
{
	float distance_to_sqr = position.distance_to_sqr(m_throw_target);
	m_throw_actual = m_throw_actual && (distance_to_sqr < _sqr(.1f));
	m_throw_target = position;
}

void CAI_Stalker::update_throw_params()
{
	if (m_throw_actual)
	{
		if (m_computed_object_position.similar(Position()))
		{
			if (m_computed_object_direction.similar(Direction()))
			{
				VERIFY(_valid(m_throw_force));
				return;
			}
		}
	}

	m_throw_actual = true;
	m_computed_object_position = Position();
	m_computed_object_direction = Direction();

	m_throw_position = eye_matrix.c;

	// computing velocity with minimum magnitude
	Fvector velocity;
	velocity.sub(m_throw_target, m_throw_position);
	float time = ThrowMinVelTime(velocity, ph_world->Gravity());
	TransferenceToThrowVel(velocity, time, ph_world->Gravity());
	m_throw_force = velocity.magnitude();
	m_throw_direction = velocity.normalize();
	VERIFY(_valid(m_throw_force));
}

bool CAI_Stalker::critically_wounded()
{
	if (critical_wound_type() == critical_wound_type_dummy)
		return (false);

	if (!brain().CStalkerPlanner::m_storage.property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded))
	{
		critical_wounded_state_stop();
		return (false);
	}

	return (true);
}

bool CAI_Stalker::critical_wound_external_conditions_suitable()
{
	if (movement().body_state() != eBodyStateStand)
		return (false);

	if (animation().non_script_need_update())
		return (false);

	CWeapon* active_weapon = smart_cast<CWeapon*>(inventory().ActiveItem());
	if (!active_weapon)
		return (false);

	switch (active_weapon->animation_slot())
	{
	case 1: // pistols
	case 2: // automatic weapon
	case 3: // rifles
		break;
	default:
		return (false);
	}

	if (!agent_manager().member().registered_in_combat(this))
		return (false);

	//	Msg								("%6d executing critical hit",Device.dwTimeGlobal);
	animation().global().make_inactual();
	return (true);
}

void CAI_Stalker::remove_critical_hit()
{
	brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, false);

	animation().global().remove_callback(CStalkerAnimationPair::CALLBACK_ID(this, &CAI_Stalker::remove_critical_hit));
}

void CAI_Stalker::critical_wounded_state_start()
{
	brain().CStalkerPlanner::m_storage.set_property(StalkerDecisionSpace::eWorldPropertyCriticallyWounded, true);

	animation().global().add_callback(CStalkerAnimationPair::CALLBACK_ID(this, &CAI_Stalker::remove_critical_hit));
}

bool CAI_Stalker::can_cry_enemy_is_wounded() const
{
	if (!brain().initialized())
		return (false);

	if (brain().current_action_id() != StalkerDecisionSpace::eWorldOperatorCombatPlanner)
		return (false);

	typedef CActionPlannerActionScript<CAI_Stalker> planner_type;
	planner_type* planner = smart_cast<planner_type*>(&brain().current_action());
	VERIFY(planner);

	switch (planner->current_action_id())
	{
	case StalkerDecisionSpace::eWorldOperatorGetReadyToKill:
	case StalkerDecisionSpace::eWorldOperatorGetReadyToDetour:
	case StalkerDecisionSpace::eWorldOperatorKillEnemy:
	case StalkerDecisionSpace::eWorldOperatorTakeCover:
	case StalkerDecisionSpace::eWorldOperatorLookOut:
	case StalkerDecisionSpace::eWorldOperatorHoldPosition:
	case StalkerDecisionSpace::eWorldOperatorDetourEnemy:
	case StalkerDecisionSpace::eWorldOperatorSearchEnemy:
	case StalkerDecisionSpace::eWorldOperatorKillEnemyIfNotVisible:
	case StalkerDecisionSpace::eWorldOperatorKillEnemyIfCriticallyWounded:
		return (true);
	case StalkerDecisionSpace::eWorldOperatorGetItemToKill:
	case StalkerDecisionSpace::eWorldOperatorRetreatFromEnemy:
	case StalkerDecisionSpace::eWorldOperatorPostCombatWait:
	case StalkerDecisionSpace::eWorldOperatorHideFromGrenade:
	case StalkerDecisionSpace::eWorldOperatorSuddenAttack:
	case StalkerDecisionSpace::eWorldOperatorKillEnemyIfPlayerOnThePath:
	case StalkerDecisionSpace::eWorldOperatorKillWoundedEnemy:
	case StalkerDecisionSpace::eWorldOperatorCriticallyWounded:
		return (false);
	default:
		NODEFAULT;
	}
#ifdef DEBUG
	return (false);
#endif // DEBUG
}

void CAI_Stalker::on_critical_wound_initiator(const CAI_Stalker* critically_wounded)
{
	if (!can_cry_enemy_is_wounded())
		return;

	sound().play(eStalkerSoundEnemyCriticallyWounded);
}

void CAI_Stalker::on_enemy_wounded_or_killed(const CAI_Stalker* wounded_or_killed)
{
	// [IMPROVEMENT] Rage Mode: Если убили или ранили союзника/врага, включаем режим ярости на 5 секунд
	// В этом режиме (fire_make_sense) мы стреляем агрессивнее.
	m_rage_end_time = Device.dwTimeGlobal + 5000;

	if (!can_cry_enemy_is_wounded())
		return;

	sound().play(eStalkerSoundEnemyKilledOrWounded);
}

bool CAI_Stalker::can_kill_member()
{
	if (!animation().script_animations().empty())
		return (false);

	update_can_kill_info();
	return (m_can_kill_member);
}

bool CAI_Stalker::can_kill_enemy()
{
	VERIFY(inventory().ActiveItem());
	update_can_kill_info();
	return (m_can_kill_enemy);
}
