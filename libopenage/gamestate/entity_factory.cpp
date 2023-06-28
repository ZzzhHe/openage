// Copyright 2023-2023 the openage authors. See copying.md for legal info.

#include "entity_factory.h"

#include "event/state.h"
#include "gamestate/activity/activity.h"
#include "gamestate/activity/end_node.h"
#include "gamestate/activity/event_node.h"
#include "gamestate/activity/start_node.h"
#include "gamestate/activity/task_system_node.h"
#include "gamestate/activity/xor_node.h"
#include "gamestate/component/api/idle.h"
#include "gamestate/component/api/live.h"
#include "gamestate/component/api/move.h"
#include "gamestate/component/api/turn.h"
#include "gamestate/component/internal/activity.h"
#include "gamestate/component/internal/command_queue.h"
#include "gamestate/component/internal/ownership.h"
#include "gamestate/component/internal/position.h"
#include "gamestate/game_entity.h"
#include "gamestate/game_state.h"
#include "gamestate/manager.h"
#include "renderer/render_factory.h"

namespace openage::gamestate {

/**
 * Create a simple test activity for the game entity.
 *
 * The activity is as follows:
 *                      |------------------------------------------------------|
 *                      |                                                      v
 * Start -> Idle -> Condition -> Wait for command -> Move -> Wait for move -> End
 *            ^                                                      |
 *            |------------------------------------------------------|
 * TODO: Replace with config
 */
std::shared_ptr<activity::Activity> create_test_activity(const std::shared_ptr<openage::event::EventLoop> &loop,
                                                         const std::shared_ptr<GameState> &state) {
	auto start = std::make_shared<activity::StartNode>(0);
	auto idle = std::make_shared<activity::TaskSystemNode>(1);
	auto condition_moveable = std::make_shared<activity::ConditionNode>(2);
	auto wait_for_command = std::make_shared<activity::EventNode>(3);
	auto move = std::make_shared<activity::TaskSystemNode>(4);
	auto wait_for_move = std::make_shared<activity::EventNode>(5);
	auto end = std::make_shared<activity::EndNode>(6);

	start->add_output(idle);

	idle->add_output(condition_moveable);
	idle->set_system_id(system::system_id_t::IDLE);

	condition_moveable->add_output(wait_for_command);
	condition_moveable->add_output(end);
	condition_moveable->set_condition_func([&](const curve::time_t & /* time */,
	                                           const std::shared_ptr<GameEntity> &entity) {
		if (entity->has_component(component::component_t::MOVE)) {
			return wait_for_command->get_id();
		}

		return end->get_id();
	});

	wait_for_command->add_output(move);
	wait_for_command->set_primer_func([&](const curve::time_t &time,
	                                      const std::shared_ptr<GameEntity> &entity) {
		auto ev = loop->create_event("game.process_command", entity->get_manager(), state, time);
		auto entity_queue = std::dynamic_pointer_cast<component::CommandQueue>(
			entity->get_component(component::component_t::COMMANDQUEUE));
		auto &queue = const_cast<curve::Queue<std::shared_ptr<component::command::Command>> &>(entity_queue->get_queue());
		queue.add_dependent(ev);
	});
	wait_for_command->set_next_func([&](const curve::time_t &time,
	                                    const std::shared_ptr<GameEntity> &entity) {
		auto entity_queue = std::dynamic_pointer_cast<component::CommandQueue>(
			entity->get_component(component::component_t::COMMANDQUEUE));
		auto &queue = entity_queue->get_queue();

		if (queue.front(time)->get_type() == component::command::command_t::MOVE) {
			return move->get_id();
		}

		throw Error{ERR << "Unknown command type"};
	});

	move->add_output(wait_for_move);
	move->set_system_id(system::system_id_t::MOVE_COMMAND);

	wait_for_move->add_output(idle);
	wait_for_move->add_output(end);

	return std::make_shared<activity::Activity>(0, "test", start);
}

EntityFactory::EntityFactory() :
	next_id{0},
	render_factory{nullptr} {
}

std::shared_ptr<GameEntity> EntityFactory::add_game_entity(const std::shared_ptr<openage::event::EventLoop> &loop,
                                                           const std::shared_ptr<GameState> &state,
                                                           const nyan::fqon_t &nyan_entity) {
	auto entity = std::make_shared<GameEntity>(this->get_next_id());
	init_components(loop, state, entity, nyan_entity);

	if (this->render_factory) {
		entity->set_render_entity(this->render_factory->add_world_render_entity());
	}

	return entity;
}

void EntityFactory::attach_renderer(const std::shared_ptr<renderer::RenderFactory> &render_factory) {
	std::unique_lock lock{this->mutex};

	this->render_factory = render_factory;
}

void EntityFactory::init_components(const std::shared_ptr<openage::event::EventLoop> &loop,
                                    const std::shared_ptr<GameState> &state,
                                    const std::shared_ptr<GameEntity> &entity,
                                    const nyan::fqon_t &nyan_entity) {
	auto position = std::make_shared<component::Position>(loop);
	entity->add_component(position);

	auto ownership = std::make_shared<component::Ownership>(loop);
	entity->add_component(ownership);

	auto command_queue = std::make_shared<component::CommandQueue>(loop);
	entity->add_component(command_queue);

	auto db_view = state->get_nyan_db();
	auto nyan_obj = db_view->get_object(nyan_entity);
	nyan::set_t abilities = nyan_obj.get_set("GameEntity.abilities");

	for (const auto &ability_val : abilities) {
		auto ability_fqon = std::dynamic_pointer_cast<nyan::ObjectValue>(ability_val.get_ptr())->get_name();
		auto ability_obj = db_view->get_object(ability_fqon);

		auto ability_parent = ability_obj.get_parents()[0];
		if (ability_parent == "engine.ability.type.Move") {
			auto move = std::make_shared<component::Move>(loop, ability_obj);
			entity->add_component(move);
		}
		else if (ability_parent == "engine.ability.type.Turn") {
			auto turn = std::make_shared<component::Turn>(loop, ability_obj);
			entity->add_component(turn);
		}
		else if (ability_parent == "engine.ability.type.Idle") {
			auto idle = std::make_shared<component::Idle>(loop, ability_obj);
			entity->add_component(idle);
		}
		else if (ability_parent == "engine.ability.type.Live") {
			auto live = std::make_shared<component::Live>(loop, ability_obj);
			entity->add_component(live);
		}
	}

	// must be initialized after all other components
	auto activity = std::make_shared<component::Activity>(loop, create_test_activity(loop, state));
	entity->add_component(activity);
}

entity_id_t EntityFactory::get_next_id() {
	auto new_id = this->next_id;
	this->next_id++;

	return new_id;
}

} // namespace openage::gamestate
