/**************************************************************************/
/*  spx_sprite_mgr.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "spx_sprite_mgr.h"

#include "core/templates/rb_map.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/packed_scene.h"
#include "spx_engine.h"
#include "spx_res_mgr.h"
#include "spx_ext_mgr.h"
#include "spx_scene_mgr.h"
#include "spx_physics_mgr.h"
#include "spx_layer_sorter.h"
#include "spx_sprite.h"
#include "spx_object_guard.h"
#include "core/typedefs.h"


#define DEFAULT_COLLISION_ALPHA_THRESHOLD 0.05

StringName SpxSpriteMgr::default_texture_anim;

// Refactored sprite validation using unified SpxObjectGuard (RAII pattern)
// See spx_object_guard.h for details
#define SPX_REQUIRE_SPRITE_VOID() \
	SPX_SPRITE_GUARD_VOID(obj, __func__)

#define SPX_REQUIRE_SPRITE_RETURN(VALUE) \
	SPX_SPRITE_GUARD_RETURN(obj, __func__, VALUE)

#define SPX_REQUIRE_TARGET_SPRITE_VOID(TARGET) \
	SPX_TARGET_SPRITE_GUARD_VOID(TARGET, __func__)

#define SPX_REQUIRE_TARGET_SPRITE_RETURN(TARGET, VALUE) \
	SPX_TARGET_SPRITE_GUARD_RETURN(TARGET, __func__, VALUE)

void SpxSpriteMgr::on_awake() {
	SpxBaseMgr::on_awake();
	default_texture_anim = "default";
	
	// Initialize pixel collision sampling step with default value of 2 (good balance between performance and accuracy)
	pixel_collision_sampling_step = 2;
	
	dont_destroy_root = memnew(Node2D);
	dont_destroy_root->set_name("dont_destroy_root");
	get_spx_root()->add_child(dont_destroy_root);

	sprite_root = memnew(Node2D);
	sprite_root->set_name("sprite_root");
	get_spx_root()->add_child(sprite_root);
}

void SpxSpriteMgr::on_start() {
	SpxBaseMgr::on_start();
	auto nodes = get_root()->find_children("*","SpxSprite",true,false);
	for(int i = 0; i < nodes.size(); i++) {
		auto sprite = Object::cast_to<SpxSprite>(nodes[i]);
		if(sprite != nullptr) {
			sprite->set_gid(get_unique_id());
			sprite->on_start();
			spriteMgr->id_objects[sprite->get_gid()] = sprite;
			auto value = sprite->get_spx_type_name();
			auto data = SpxReturnStr(value);
			SPX_CALLBACK->func_on_scene_sprite_instantiated(sprite->get_gid(), data);
		}
	}
}

void SpxSpriteMgr::on_destroy() {
	SpxBaseMgr::on_destroy();
}

void SpxSpriteMgr::on_update(float delta) {
	SpxBaseMgr::on_update(delta);
	_check_pixel_collision_events();

	Vector<ISortableSprite*> all_sortables;

	for (auto& pair : id_objects) {
		if (pair.value) {
			all_sortables.push_back(pair.value);
		}
	}

	sceneMgr->collect_sortable_sprites(all_sortables);
	SpxLayerSorter::instance().update(all_sortables);
}

void SpxSpriteMgr::on_reset(int reset_code) {
	default_texture_anim = "default";
	dont_destroy_root->queue_free();
	dont_destroy_root = memnew(Node2D);
	dont_destroy_root->set_name("dont_destroy_root");
	get_spx_root()->add_child(dont_destroy_root);
	
	destroy_all_sprites();
}

void SpxSpriteMgr::collect_sortable_sprites(Vector<ISortableSprite*>& out) {
	for (auto& pair : id_objects) {
		if (pair.value) {
			out.push_back(pair.value);
		}
	}
}

SpxSprite *SpxSpriteMgr::get_sprite(GdObj obj) {
	if (id_objects.has(obj)) {
		return id_objects[obj];
	}
	return nullptr;
}

void SpxSpriteMgr::on_sprite_destroy(SpxSprite *sprite) {
	if (id_objects.erase(sprite->get_gid())) {
		SPX_CALLBACK->func_on_sprite_destroyed(sprite->get_gid());
	}
}

void SpxSpriteMgr::set_dont_destroy_on_load(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite.get()->get_parent()->remove_child(sprite.get());
	dont_destroy_root->add_child(sprite.get());
}

void SpxSpriteMgr::set_process(GdObj obj, GdBool is_on) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_process(is_on);
}

void SpxSpriteMgr::set_physic_process(GdObj obj, GdBool is_on) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_physics_process(is_on);
}
void SpxSpriteMgr::set_type_name(GdObj obj, GdString type_name) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_type_name(type_name);
}

void SpxSpriteMgr::set_child_position(GdObj obj, GdString path, GdVec2 pos) {
	SPX_REQUIRE_SPRITE_VOID()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_position(GdVec2{ pos.x, -pos.y });
	}
}

GdVec2 SpxSpriteMgr::get_child_position(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		auto pos = child->get_position();
		return GdVec2{ pos.x, -pos.y };
	}
	return GdVec2();
}

void SpxSpriteMgr::set_child_rotation(GdObj obj, GdString path, GdFloat rot) {
	SPX_REQUIRE_SPRITE_VOID()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_rotation(rot);
	}
}

GdFloat SpxSpriteMgr::get_child_rotation(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		return child->get_rotation();
	}
	return 0;
}

void SpxSpriteMgr::set_child_scale(GdObj obj, GdString path, GdVec2 scale) {
	SPX_REQUIRE_SPRITE_VOID()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_scale(scale);
	}
}

GdVec2 SpxSpriteMgr::get_child_scale(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		return child->get_scale();
	}
	return GdVec2();
}

GdBool SpxSpriteMgr::check_collision(GdObj obj, GdObj target, GdBool is_src_trigger, GdBool is_dst_trigger) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	SPX_REQUIRE_TARGET_SPRITE_RETURN(target,false)
	return sprite->check_collision(sprite_target.get(),is_src_trigger,is_dst_trigger);
}

GdBool SpxSpriteMgr::check_collision_with_point(GdObj obj, GdVec2 point, GdBool is_trigger) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	point.y = - point.y;
	return sprite->check_collision_with_point(point, is_trigger);
}

GdInt SpxSpriteMgr::create_backdrop(GdString path) {
	return _create_sprite(path, GdVec2(), true);
}

GdInt SpxSpriteMgr::create_sprite(GdString path, GdVec2 pos) {
	return _create_sprite(path, pos, false);
}

// sprite
GdInt SpxSpriteMgr::_create_sprite(GdString path, GdVec2 pos, GdBool is_backdrop) {
	const String path_str = SpxStr(path);
	SpxSprite *sprite = nullptr;
	if (path_str == "") {
		sprite = memnew(SpxSprite);
		sprite->set_position(GdVec2(pos.x, -pos.y));
		AnimatedSprite2D *animated_sprite = memnew(AnimatedSprite2D);
		sprite->add_child(animated_sprite);
		Area2D *area = memnew(Area2D);
		sprite->add_child(area);
		CollisionShape2D *area_collision_shape = memnew(CollisionShape2D);
		const Ref<CircleShape2D> area_shape = memnew(CircleShape2D);
		area_shape->set_radius(10.0f);
		area_collision_shape->set_shape(area_shape);
		area->add_child(area_collision_shape);
		CollisionShape2D *body_collision_shape = memnew(CollisionShape2D);
		const Ref<CircleShape2D> body_shape = memnew(CircleShape2D);
		body_shape->set_radius(10.0f);
		body_collision_shape->set_shape(body_shape);
		sprite->add_child(body_collision_shape);
		Node2D *shooting_point = memnew(Node2D);
		shooting_point->set_name("ShootingPoint");
		sprite->add_child(shooting_point);
	} else {
		// load from path
		Ref<PackedScene> scene = ResourceLoader::load(path_str);
		if (scene.is_null()) {
			print_error("Failed to load sprite scene " + path_str);
			return NULL_OBJECT_ID;
		} else {
			sprite = dynamic_cast<SpxSprite *>(scene->instantiate());
			if (sprite == nullptr) {
				print_error("Failed to load sprite scene , type invalid " + path_str);
			}
		}
	}

	sprite->is_backdrop = is_backdrop;
	sprite->set_gid(get_unique_id());
	sprite_root->add_child(sprite);
	sprite->on_start();
	id_objects[sprite->get_gid()] = sprite;
	SPX_CALLBACK->func_on_sprite_ready(sprite->get_gid());
	return sprite->get_gid();
}

void SpxSpriteMgr::destroy_all_sprites() {
	sprite_root->queue_free();
	sprite_root = memnew(Node2D);
	sprite_root->set_name("sprite_root");
	get_spx_root()->add_child(sprite_root);

	id_objects.clear();
	bounding_collision_pairs.clear();
	pixel_collision_pairs.clear();
}

GdInt SpxSpriteMgr::clone_sprite(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(NULL_OBJECT_ID)
	SpxSprite *cloned = dynamic_cast<SpxSprite *>(sprite->duplicate());
	if (unlikely(!cloned)) {
		ERR_PRINT("Failed to clone sprite with GID: " + itos(obj));
		return NULL_OBJECT_ID;
	}
	cloned->set_gid(get_unique_id());
	sprite_root->add_child(cloned);
	id_objects[cloned->get_gid()] = cloned;
	cloned->on_start();
	SPX_CALLBACK->func_on_sprite_ready(cloned->get_gid());
	return cloned->get_gid();
}

GdBool SpxSpriteMgr::destroy_sprite(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	sprite->set_block_signals(true);
	sprite->queue_free();
	return true;
}

GdBool SpxSpriteMgr::is_sprite_alive(GdObj obj) {
	return get_sprite(obj) != nullptr;
}

void SpxSpriteMgr::set_position(GdObj obj, GdVec2 pos) {
	SPX_REQUIRE_SPRITE_VOID()
	// flip y axis
	sprite->set_position(GdVec2(pos.x, -pos.y));
}

void SpxSpriteMgr::set_rotation(GdObj obj, GdFloat rot) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_rotation(rot);
}

void SpxSpriteMgr::set_scale(GdObj obj, GdVec2 scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_scale(scale);
}

GdVec2 SpxSpriteMgr::get_position(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto pos = sprite->get_position();
	// flip y axis
	return GdVec2{ pos.x, -pos.y };
}

GdFloat SpxSpriteMgr::get_rotation(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_rotation();
}

GdVec2 SpxSpriteMgr::get_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_scale();
}

void SpxSpriteMgr::set_render_scale(GdObj obj, GdVec2 scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_render_scale(scale);
}
GdVec2 SpxSpriteMgr::get_render_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_render_scale();
}

void SpxSpriteMgr::set_color(GdObj obj, GdColor color) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_color(color);
}

GdColor SpxSpriteMgr::get_color(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdColor()) return sprite->get_color();
}

void SpxSpriteMgr::set_material_shader(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_shader(path);
}

GdString SpxSpriteMgr::get_material_shader(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_material_shader();
}

void SpxSpriteMgr::set_material_params(GdObj obj, GdString effect, GdFloat amount) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params(effect, amount);
}

GdFloat SpxSpriteMgr::get_material_params(GdObj obj, GdString effect) {
	SPX_REQUIRE_SPRITE_RETURN(GdFloat())
	return sprite->get_material_params(effect);
}

void SpxSpriteMgr::set_material_params_vec4(GdObj obj, GdString effect, GdVec4 vec4) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params_vec4(effect, vec4);
}

void SpxSpriteMgr::set_material_params_vec(GdObj obj, GdString effect,  GdFloat x, GdFloat y, GdFloat z, GdFloat w){
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params_vec4(effect, GdVec4(x,y,z,w));
}


GdVec4 SpxSpriteMgr::get_material_params_vec4(GdObj obj, GdString effect) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec4())
	return sprite->get_material_params_vec4(effect);
}

void SpxSpriteMgr::set_material_params_color(GdObj obj, GdString effect, GdColor color) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params_color(effect, color);
}

GdColor SpxSpriteMgr::get_material_params_color(GdObj obj, GdString effect) {
	SPX_REQUIRE_SPRITE_RETURN(GdColor())
	return sprite->get_material_params_color(effect);
}

void SpxSpriteMgr::set_texture_atlas(GdObj obj, GdString path, GdRect2 rect2) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture_atlas(path, rect2);
}

void SpxSpriteMgr::set_texture(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture(path);
}

void SpxSpriteMgr::set_texture_atlas_direct(GdObj obj, GdString path, GdRect2 rect2) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture_atlas_direct(path, rect2, true);
}

void SpxSpriteMgr::set_texture_direct(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture_direct(path, true);
}

GdString SpxSpriteMgr::get_texture(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_texture();
}

void SpxSpriteMgr::set_visible(GdObj obj, GdBool visible) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_visible(visible);
	sprite->on_set_visible(visible);
}

GdBool SpxSpriteMgr::get_visible(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_visible();
}

GdInt SpxSpriteMgr::get_z_index(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_z_index();
}

void SpxSpriteMgr::set_z_index(GdObj obj, GdInt z) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_z_index(z);
}

void SpxSpriteMgr::play_anim(GdObj obj, GdString p_name, GdFloat p_speed, GdBool isLoop, GdBool p_revert) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->play_anim(p_name, p_speed, isLoop, p_revert);
}

void SpxSpriteMgr::play_backwards_anim(GdObj obj, GdString p_name) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->play_backwards_anim(p_name);
}

void SpxSpriteMgr::pause_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->pause_anim();
}

void SpxSpriteMgr::stop_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->stop_anim();
}

GdBool SpxSpriteMgr::is_playing_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_playing_anim();
}

void SpxSpriteMgr::set_anim(GdObj obj, GdString p_name) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim(p_name);
}

GdString SpxSpriteMgr::get_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_anim();
}

void SpxSpriteMgr::set_anim_frame(GdObj obj, GdInt p_frame) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_frame(p_frame);
}

GdInt SpxSpriteMgr::get_anim_frame(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_anim_frame();
}

void SpxSpriteMgr::set_anim_speed_scale(GdObj obj, GdFloat p_speed_scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_speed_scale(p_speed_scale);
}

GdFloat SpxSpriteMgr::get_anim_speed_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(1.0)
	return sprite->get_anim_speed_scale();
}

GdFloat SpxSpriteMgr::get_anim_playing_speed(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(1.0)
	return sprite->get_anim_playing_speed();
}

void SpxSpriteMgr::set_anim_centered(GdObj obj, GdBool p_center) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_centered(p_center);
}

GdBool SpxSpriteMgr::is_anim_centered(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_anim_centered();
}

void SpxSpriteMgr::set_anim_offset(GdObj obj, GdVec2 p_offset) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_offset(p_offset);
}

GdVec2 SpxSpriteMgr::get_anim_offset(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_anim_offset();
}

void SpxSpriteMgr::set_anim_flip_h(GdObj obj, GdBool p_flip) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_flip_h(p_flip);
}

GdBool SpxSpriteMgr::is_anim_flipped_h(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get property of a null sprite" + itos(obj));
		return false;
	}
	return sprite->is_anim_flipped_h();
}

void SpxSpriteMgr::set_anim_flip_v(GdObj obj, GdBool p_flip) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_flip_v(p_flip);
}

GdBool SpxSpriteMgr::is_anim_flipped_v(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_anim_flipped_v();
}
GdString SpxSpriteMgr::get_current_anim_name(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_current_anim_name();
}

void SpxSpriteMgr::set_velocity(GdObj obj, GdVec2 velocity) {
	SPX_REQUIRE_SPRITE_VOID()
	// flip y axis
	sprite->set_velocity(GdVec2(velocity.x, -velocity.y));
}

GdVec2 SpxSpriteMgr::get_velocity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto val = sprite->get_velocity();
	// flip y axis
	return GdVec2{ val.x, -val.y };
}

GdBool SpxSpriteMgr::is_on_floor(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_floor();
}

GdBool SpxSpriteMgr::is_on_floor_only(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_floor_only();
}

GdBool SpxSpriteMgr::is_on_wall(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_wall();
}

GdBool SpxSpriteMgr::is_on_wall_only(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_wall_only();
}

GdBool SpxSpriteMgr::is_on_ceiling(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_ceiling();
}

GdBool SpxSpriteMgr::is_on_ceiling_only(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_ceiling_only();
}

GdVec2 SpxSpriteMgr::get_last_motion(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_last_motion();
}

GdVec2 SpxSpriteMgr::get_position_delta(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_position_delta();
}

GdVec2 SpxSpriteMgr::get_floor_normal(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_floor_normal();
}

GdVec2 SpxSpriteMgr::get_wall_normal(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_wall_normal();
}

GdVec2 SpxSpriteMgr::get_real_velocity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_real_velocity();
}

void SpxSpriteMgr::move_and_slide(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->move_and_slide();
}

void SpxSpriteMgr::set_gravity(GdObj obj, GdFloat gravity) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_gravity(gravity);
}

GdFloat SpxSpriteMgr::get_gravity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_gravity();
}

void SpxSpriteMgr::set_mass(GdObj obj, GdFloat mass) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_mass(mass);
}

GdFloat SpxSpriteMgr::get_mass(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_mass();
}

void SpxSpriteMgr::add_force(GdObj obj, GdVec2 force) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->add_force(force);
}

void SpxSpriteMgr::add_impulse(GdObj obj, GdVec2 impulse) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->add_impulse(impulse);
}


void SpxSpriteMgr::set_physics_mode(GdObj obj, GdInt mode) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_physics_mode(mode);
}

GdInt SpxSpriteMgr::get_physics_mode(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_physics_mode();
}

void SpxSpriteMgr::set_use_gravity(GdObj obj, GdBool enabled) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_use_gravity(enabled);
}

GdBool SpxSpriteMgr::is_use_gravity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_use_gravity();
}

void SpxSpriteMgr::set_gravity_scale(GdObj obj, GdFloat scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_gravity_scale(scale);
}

GdFloat SpxSpriteMgr::get_gravity_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(1.0f)
	return sprite->get_gravity_scale();
}

void SpxSpriteMgr::set_drag(GdObj obj, GdFloat drag) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_drag(drag);
}

GdFloat SpxSpriteMgr::get_drag(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0.0f)
	return sprite->get_drag();
}

void SpxSpriteMgr::set_friction(GdObj obj, GdFloat friction) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_friction(friction);
}

GdFloat SpxSpriteMgr::get_friction(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0.0f)
	return sprite->get_friction();
}

void SpxSpriteMgr::set_collision_layer(GdObj obj, GdInt layer) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collision_layer((uint32_t)layer);
}

GdInt SpxSpriteMgr::get_collision_layer(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_collision_layer();
}

void SpxSpriteMgr::set_collision_mask(GdObj obj, GdInt mask) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collision_mask((uint32_t)mask);
}

GdInt SpxSpriteMgr::get_collision_mask(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_collision_mask();
}


void SpxSpriteMgr::set_trigger_layer(GdObj obj, GdInt layer) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_layer(layer);
}

GdInt SpxSpriteMgr::get_trigger_layer(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_trigger_layer();
}

void SpxSpriteMgr::set_trigger_mask(GdObj obj, GdInt mask) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_mask(mask);
}

GdInt SpxSpriteMgr::get_trigger_mask(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_trigger_mask();
}

void SpxSpriteMgr::set_collider_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collider_rect(center, size);
}

void SpxSpriteMgr::set_collider_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collider_circle(center, radius);
}

void SpxSpriteMgr::set_collider_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collider_capsule(center, size);
}

void SpxSpriteMgr::set_collider_polygon(GdObj obj, GdVec2 center, GdArray points) {
	SPX_REQUIRE_SPRITE_VOID()
	center.y = -center.y;
	sprite->set_collider_polygon(center, points);
}

void SpxSpriteMgr::set_collision_enabled(GdObj obj, GdBool enabled) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collision_enabled(enabled);
}

GdBool SpxSpriteMgr::is_collision_enabled(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_collision_enabled();
}

void SpxSpriteMgr::set_trigger_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_rect(center, size);
}

void SpxSpriteMgr::set_trigger_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_circle(center, radius);
}

void SpxSpriteMgr::set_trigger_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_capsule(center, size);
}

void SpxSpriteMgr::set_trigger_polygon(GdObj obj, GdVec2 center, GdArray points) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_polygon(center, points);
}

void SpxSpriteMgr::set_trigger_enabled(GdObj obj, GdBool trigger) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_enabled(trigger);
}
GdBool SpxSpriteMgr::is_trigger_enabled(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_trigger_enabled();
}

Ref<Image> SpxSpriteMgr::_get_current_frame_image(AnimatedSprite2D *sprite) {
	Ref<SpriteFrames> frames = sprite->get_sprite_frames();
	if (frames.is_null()) {
		return Ref<Texture2D>();
	}

	String current_animation = sprite->get_animation();
	int current_frame = sprite->get_frame();

	if (!frames->has_animation(current_animation)) {
		return Ref<Texture2D>();
	}

	auto texture = frames->get_frame_texture(current_animation, current_frame);
	if (texture.is_null()) {
		return Ref<Image>();
	}
	Ref<Image> image = texture->get_image();
	if (image.is_null()) {
		return Ref<Image>();
	}
	return image;
}

Rect2 SpxSpriteMgr::_get_sprite_aabb(AnimatedSprite2D *anim2d) {
	if (!anim2d)
		return Rect2();

	Ref<Texture2D> texture = anim2d->get_sprite_frames()->get_frame_texture(anim2d->get_animation(), anim2d->get_frame());
	if (texture.is_null())
		return Rect2();

	Vector2 texture_size = texture->get_size();
	Transform2D transform = anim2d->get_global_transform();

	Vector2 top_left = transform.xform(Vector2(-texture_size.x / 2, -texture_size.y / 2));
	Vector2 top_right = transform.xform(Vector2(texture_size.x / 2, -texture_size.y / 2));
	Vector2 bottom_left = transform.xform(Vector2(-texture_size.x / 2, texture_size.y / 2));
	Vector2 bottom_right = transform.xform(Vector2(texture_size.x / 2, texture_size.y / 2));

	float min_x = MIN(MIN(top_left.x, top_right.x), MIN(bottom_left.x, bottom_right.x));
	float max_x = MAX(MAX(top_left.x, top_right.x), MAX(bottom_left.x, bottom_right.x));
	float min_y = MIN(MIN(top_left.y, top_right.y), MIN(bottom_left.y, bottom_right.y));
	float max_y = MAX(MAX(top_left.y, top_right.y), MAX(bottom_left.y, bottom_right.y));

	return Rect2(Vector2(min_x, min_y), Vector2(max_x - min_x, max_y - min_y));
}

Vector2 SpxSpriteMgr::_to_image_coord(const Transform2D &trans, Vector2 image_size, Vector2 pos) {
	Vector2 xpos = trans.xform(pos);
	auto half_size = Vector2(image_size.x / 2.0, image_size.y / 2.0);
	return Vector2(xpos.x + half_size.x,  xpos.y + half_size.y);
}

GdBool SpxSpriteMgr::check_collision_with_sprite(GdObj obj, GdObj obj_b, GdFloat alpha_threshold, GdBool use_pixel_perfect){
	SPX_REQUIRE_SPRITE_RETURN(false)
	SPX_REQUIRE_TARGET_SPRITE_RETURN(obj_b, false)
	
	// If not using pixel-perfect collision, use simple collider2d collision detection
	if (!use_pixel_perfect) {
		return sprite->check_collision(sprite_target.get(), false, false);
	}
	
	// Original pixel-perfect collision logic
	AnimatedSprite2D *anim1 = sprite->anim2d;
	if (!anim1) {
		return false;
	}
	Ref<Image> image1 = _get_current_frame_image(anim1);
	if (image1.is_null()) {
		return false;
	}
	// Calculate the sprite's AABB
	Rect2 rect1 = _get_sprite_aabb(anim1);
	Transform2D transform1 = anim1->get_global_transform();
	Vector2i size1 = image1->get_size();
	auto trans1 = transform1.affine_inverse();

	AnimatedSprite2D *anim2 = sprite_target->anim2d;
	if (!anim2) {
		return false;
	}
	Ref<Image> image2 = _get_current_frame_image(anim2);
	if (image2.is_null()) {
		return false;
	}
	Rect2 rect2 = _get_sprite_aabb(anim2);
	if (!rect1.intersects(rect2)) {
		return false; // Skip if AABBs do not intersect
	}

	// Compute the overlapping region
	Rect2 overlap = rect1.intersection(rect2);
	Transform2D transform2 = anim2->get_global_transform();
	Vector2i size2 = image2->get_size();
	auto trans2 = transform2.affine_inverse();

	// Iterate through the overlapping area for pixel-perfect collision detection
	// Use sampling step for performance optimization - check every Nth pixel instead of every pixel
	for (int x = overlap.position.x; x < overlap.position.x + overlap.size.x; x += pixel_collision_sampling_step) {
		for (int y = overlap.position.y; y < overlap.position.y + overlap.size.y; y += pixel_collision_sampling_step) {
			Vector2 local_pos1 = _to_image_coord(trans1, size1, Vector2(x, y));
			Vector2 local_pos2 = _to_image_coord(trans2, size2, Vector2(x, y));

			if (local_pos1.x >= 0 && local_pos1.x <= size1.x-1 && local_pos1.y >= 0 && local_pos1.y <= size1.y-1 &&
					local_pos2.x >= 0 && local_pos2.x <= size2.x-1 && local_pos2.y >= 0 && local_pos2.y <= size2.y-1) {
				Color color1 = image1->get_pixel((int)local_pos1.x,  (int)local_pos1.y);
				Color color2 = image2->get_pixel((int)local_pos2.x,  (int)local_pos2.y);

				if (color1.a > alpha_threshold && color2.a > alpha_threshold) {
					return true; // Early exit on first collision detected
				}
			}
		}
	}
	return false;
}

GdBool SpxSpriteMgr::check_collision_by_color(GdObj obj, GdColor color, GdFloat color_threshold, GdFloat alpha_threshold) {
	return _check_collision(obj, [=](GdColor a, GdColor b) -> bool {
		auto diff = color - b;
		auto dist = Math::sqrt(diff.r * diff.r + diff.g * diff.g + diff.b * diff.b + diff.a * diff.a);
		return dist < color_threshold && a.a > alpha_threshold;
	});
}

GdBool SpxSpriteMgr::check_collision_by_alpha(GdObj obj, GdFloat alpha_threshold) {
	return _check_collision(obj, [alpha_threshold](GdColor a, GdColor b) -> bool {
		return a.a > alpha_threshold && b.a > alpha_threshold;
	});
}

GdBool SpxSpriteMgr::_check_collision(GdObj obj, ColorCheckFunc check_func) {
	SPX_REQUIRE_SPRITE_RETURN(false) // Ensure sprite exists

	AnimatedSprite2D *anim1 = sprite->anim2d;
	if (!anim1) {
		return false;
	}
	Ref<Image> image1 = _get_current_frame_image(anim1);
	if (image1.is_null()) {
		return false;
	}
	// Calculate the sprite's AABB
	Rect2 rect1 = _get_sprite_aabb(anim1);
	Transform2D transform1 = anim1->get_global_transform();
	Vector2i size1 = image1->get_size();
	auto trans1 = transform1.affine_inverse();

	// Iterate through all objects
	for (const auto &item : id_objects) {
		SpxSprite *sp2 = item.value;
		if (sprite.get() == sp2) {
			continue; // Skip itself
		}

		AnimatedSprite2D *anim2 = sp2->anim2d;
		if (!anim2) {
			continue;
		}
		Ref<Image> image2 = _get_current_frame_image(anim2);
		if (image2.is_null()) {
			continue;
		}

		Rect2 rect2 = _get_sprite_aabb(anim2);
		if (!rect1.intersects(rect2)) {
			continue; // Skip if AABBs do not intersect
		}
		// Compute the overlapping region
		Rect2 overlap = rect1.intersection(rect2);
		Transform2D transform2 = anim2->get_global_transform();
		Vector2i size2 = image2->get_size();
		auto trans2 = transform2.affine_inverse();

		// Iterate through the overlapping area for pixel-perfect collision detection
		// Use sampling step for performance optimization
		for (int x = overlap.position.x; x < overlap.position.x + overlap.size.x; x += pixel_collision_sampling_step) {
			for (int y = overlap.position.y; y < overlap.position.y + overlap.size.y; y += pixel_collision_sampling_step) {
				Vector2 local_pos1 = _to_image_coord(trans1, size1, Vector2(x, y));
				Vector2 local_pos2 = _to_image_coord(trans2, size2, Vector2(x, y));

				if (local_pos1.x >= 0 && local_pos1.x <= size1.x - 1 && local_pos1.y >= 0 && local_pos1.y <= size1.y - 1 &&
						local_pos2.x >= 0 && local_pos2.x <= size2.x - 1 && local_pos2.y >= 0 && local_pos2.y <= size2.y - 1) {
					Color color1 = image1->get_pixel((int)local_pos1.x, (int)local_pos1.y);
					Color color2 = image2->get_pixel((int)local_pos2.x, (int)local_pos2.y);
					if (check_func(color1, color2)) {
						return true; // Early exit on collision detected
					}
				}
			}
		}
	}
	return false;
}


void SpxSpriteMgr::on_trigger_enter(GdInt self_id, GdInt other_id){
	if(physicsMgr->is_collision_by_pixel){
		bounding_collision_pairs.insert(TriggerPair(self_id, other_id));
	}else{
		SPX_CALLBACK->func_on_trigger_enter(self_id, other_id);
	}
}
void SpxSpriteMgr::on_trigger_exit(GdInt self_id, GdInt other_id){
	if(physicsMgr->is_collision_by_pixel){
		bounding_collision_pairs.erase(TriggerPair(self_id, other_id));
	}else{
		SPX_CALLBACK->func_on_trigger_exit(self_id, other_id);
	}
}

void SpxSpriteMgr::_check_pixel_collision_events() {
	// trigger pixel collision events
	if(physicsMgr->is_collision_by_pixel){
		Vector<TriggerPair> triggers;
		Vector<TriggerPair> delete_triggers;
		for(auto &trigger : bounding_collision_pairs){
			auto sprite1 = get_sprite(trigger.id1);
			if(sprite1 == nullptr) {
				delete_triggers.push_back(trigger);
				continue;
			}
			auto sprite2 = get_sprite(trigger.id2);
			if(sprite2 == nullptr) {
				delete_triggers.push_back(trigger);
				continue;
			}
		}

		for(auto &trigger : delete_triggers) {
			pixel_collision_pairs.erase(trigger);
			bounding_collision_pairs.erase(trigger);
		}
		// check collision by pixel
		for(auto &trigger : bounding_collision_pairs){
			auto is_collide = check_collision_with_sprite(trigger.id1, trigger.id2, DEFAULT_COLLISION_ALPHA_THRESHOLD, true);
			if(is_collide) {
				if(pixel_collision_pairs.find(trigger) == pixel_collision_pairs.end()) {
					pixel_collision_pairs.insert(trigger);
					triggers.push_back(trigger);
				}
			}else{
				pixel_collision_pairs.erase(trigger);
			}
		}

		// trigger pixel collision enter events
		for(auto &trigger : triggers){
			SPX_CALLBACK->func_on_trigger_enter(trigger.id1, trigger.id2);
			SPX_CALLBACK->func_on_trigger_enter(trigger.id2, trigger.id1);
		}
	}
}

void SpxSpriteMgr::set_pivot(GdObj obj, GdVec2 pivot){
	SPX_REQUIRE_SPRITE_VOID()
	pivot.y = - pivot.y;
	sprite->set_pivot(pivot);
}
GdVec2 SpxSpriteMgr::get_pivot(GdObj obj){
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto pivot= sprite->get_pivot();
	return GdVec2(pivot.x,-pivot.y);
}

void SpxSpriteMgr::batch_update_transforms(GdArray buffer) {
	// Buffer format with header: [updateCount, deleteCount, update_data..., delete_ids...]
	// - Header: [updateCount, deleteCount]
	// - Update section: [id, x, y, rotation, scaleX, scaleY, offsetX, offsetY, visible, ...] (9 fields per sprite)
	// - Delete section: [id1, id2, id3, ...] (1 field per sprite)
	const int FIELDS_PER_SPRITE = 9;
	const int HEADER_SIZE = 2;
	
	if (!buffer) {
		return;
	}
	
	auto len = buffer->size;
	if (len < HEADER_SIZE) {
		return;
	}
	
	// Get pointer to buffer data for faster access
	const float* buffer_data = SpxBaseMgr::get_array<float>(buffer, 0);
	
	// Read header using direct array access
	int update_count = static_cast<int>(buffer_data[0]);
	int delete_count = static_cast<int>(buffer_data[1]);
	
	// Validate buffer size
	int expected_size = HEADER_SIZE + update_count * FIELDS_PER_SPRITE + delete_count;
	if (len != expected_size) {
		print_error("batch_update_transforms: buffer size " + itos(len) + 
		            " does not match expected size " + itos(expected_size) +
		            " (updateCount=" + itos(update_count) + ", deleteCount=" + itos(delete_count) + ")");
		return;
	}
	
	int idx = HEADER_SIZE;
	
	// Process updates
	for (int i = 0; i < update_count; i++) {
		// Extract sprite ID and data using direct array access
		auto sprite_id = static_cast<GdObj>(buffer_data[idx]);
		auto x = buffer_data[idx + 1];
		auto y = buffer_data[idx + 2];
		auto rotation = buffer_data[idx + 3];
		auto scale_x = buffer_data[idx + 4];
		auto scale_y = buffer_data[idx + 5];
		auto offset_x = buffer_data[idx + 6];
		auto offset_y = buffer_data[idx + 7];
		auto visible = buffer_data[idx + 8] != 0.0;
		
		idx += FIELDS_PER_SPRITE;
		
		// Get sprite from id_objects map
		SpxSprite* sprite = get_sprite(sprite_id);
		if (sprite == nullptr) {
			continue;
		}
		
		// Apply transforms
		// Note: Y-axis is flipped in Godot coordinate system
		sprite->set_position(GdVec2(x, -y));
		sprite->set_rotation(rotation);
		sprite->set_scale(GdVec2(scale_x, scale_y));
		sprite->set_visible(visible);
		sprite->on_set_visible(visible);
		sprite->set_pivot(GdVec2(offset_x, -offset_y));
	}
	
	// Process deletes using direct array access
	for (int i = 0; i < delete_count; i++) {
		auto sprite_id = static_cast<GdObj>(buffer_data[idx]);
		idx++;
		
		// Get sprite and destroy it
		SpxSprite* sprite = get_sprite(sprite_id);
		if (sprite != nullptr) {
			sprite->set_block_signals(true);
			sprite->queue_free();
		}
	}
}

GdArray SpxSpriteMgr::batch_update_positions(GdArray objs) {
	// Input: array of sprite IDs [id1, id2, id3, ...]
	// Output: array of positions [x1, y1, x2, y2, x3, y3, ...]
	if (!objs) {
		return nullptr;
	}
	
	int count = objs->size;
	if (count == 0) {
		return nullptr;
	}
	
	// Create result array: 2 floats (x, y) per sprite
	GdArray result = SpxBaseMgr::create_array(GD_ARRAY_TYPE_FLOAT, count * 2);
	if (!result) {
		return nullptr;
	}
	
	// Get pointer to input array (GdObj IDs) for faster access
	const GdObj* obj_data = SpxBaseMgr::get_array<GdObj>(objs, 0);
	float* result_data = SpxBaseMgr::get_array<float>(result, 0);
	
	// Check for null pointers to prevent crashes with malformed arrays
	if (count > 0 && (!obj_data || !result_data)) {
		print_error("batch_update_positions: Failed to access array data.");
		SpxBaseMgr::free_array(result);
		return nullptr;
	}
	
	// Process each sprite ID
	int result_idx = 0;
	for (int i = 0; i < count; i++) {
		GdObj sprite_id = obj_data[i];
		
		// Get sprite from id_objects map
		SpxSprite* sprite = get_sprite(sprite_id);
		if (sprite != nullptr) {
			auto pos = sprite->get_position();
			result_data[result_idx++] = pos.x;
			result_data[result_idx++] = -pos.y;
		} else {
			result_data[result_idx++] = 0.0f;
			result_data[result_idx++] = 0.0f;
		}
	}
	
	return result;
}

void SpxSpriteMgr::set_pixel_collision_sampling_step(GdInt step) {
	// Clamp to valid range (minimum 1, as 0 or negative would cause infinite loop)
	if (step < 1) {
		pixel_collision_sampling_step = 1;
		print_error("pixel_collision_sampling_step must be at least 1. Setting to 1.");
	} else {
		pixel_collision_sampling_step = step;
	}
}

GdInt SpxSpriteMgr::get_pixel_collision_sampling_step() {
	return pixel_collision_sampling_step;
}
