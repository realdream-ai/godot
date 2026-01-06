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
#include "spx_physic_mgr.h"
#include "spx_layer_sorter.h"
#include "spx_sprite.h"
#include "core/typedefs.h"

// SIMD support for pixel collision optimization
#if defined(__wasm__) && defined(__wasm_simd128__)
	// WebAssembly SIMD support (requires -msimd128 compiler flag)
	#include <wasm_simd128.h>
	#define SPX_USE_WASM_SIMD
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
	#include <emmintrin.h> // SSE2
	#ifdef __SSSE3__
		#include <tmmintrin.h> // SSSE3
		#define SPX_USE_SSSE3
	#endif
	#define SPX_USE_SSE2
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
	#include <arm_neon.h>
	#define SPX_USE_NEON
#endif


#define DEFAULT_COLLISION_ALPHA_THRESHOLD 0.05

StringName SpxSpriteMgr::default_texture_anim;
#define check_and_get_sprite_r(VALUE) \
	auto sprite = get_sprite(obj);\
	if (sprite == nullptr) {\
		print_error("try to get property of a null sprite gid=" + itos(obj)); \
		return VALUE; \
	}

#define check_and_get_sprite_v() \
	auto sprite = get_sprite(obj);\
	if (sprite == nullptr) {\
		print_error("try to get property of a null sprite gid=" + itos(obj)); \
		return ; \
	}

#define check_and_get_target_sprite_v(TARGET) \
	auto sprite_##TARGET = get_sprite(TARGET);\
	if (sprite_##TARGET == nullptr) {\
		print_error("try to get property of a null sprite gid=" + itos(TARGET)); \
	return ; \
}
#define check_and_get_target_sprite_r(TARGET,VALUE) \
	auto sprite_##TARGET = get_sprite(TARGET);\
	if (sprite_##TARGET == nullptr) {\
		print_error("try to get property of a null sprite gid=" + itos(TARGET)); \
	return VALUE; \
}

void SpxSpriteMgr::on_awake() {
	SpxBaseMgr::on_awake();
	default_texture_anim = "default";
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
			//sprite->get_parent()->remove_child(sprite);
			//get_spx_root()->add_child(sprite);
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

	// Collect all sortable sprites from both SpxSpriteMgr and SpxExtMgr
	Vector<ISortableSprite*> all_sortables;

	// Add SpxSprites
	for (auto& pair : id_objects) {
		if (pair.value) {
			all_sortables.push_back(pair.value);
		}
	}

	// Add pure sprites from SpxExtMgr

	sceneMgr->collect_sortable_sprites(all_sortables);

	// Unified sorting
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
	check_and_get_sprite_v()
	sprite->get_parent()->remove_child(sprite);
	dont_destroy_root->add_child(sprite);
}

void SpxSpriteMgr::set_process(GdObj obj, GdBool is_on) {
	check_and_get_sprite_v()
	sprite->set_process(is_on);
}

void SpxSpriteMgr::set_physic_process(GdObj obj, GdBool is_on) {
	check_and_get_sprite_v()
	sprite->set_physics_process(is_on);
}
void SpxSpriteMgr::set_type_name(GdObj obj, GdString type_name) {
	check_and_get_sprite_v()
	sprite->set_type_name(type_name);
}

void SpxSpriteMgr::set_child_position(GdObj obj, GdString path, GdVec2 pos) {
	check_and_get_sprite_v()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_position(GdVec2{ pos.x, -pos.y });
	}
}

GdVec2 SpxSpriteMgr::get_child_position(GdObj obj, GdString path) {
	check_and_get_sprite_r(GdVec2())
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		auto pos = child->get_position();
		return GdVec2{ pos.x, -pos.y };
	}
	return GdVec2();
}

void SpxSpriteMgr::set_child_rotation(GdObj obj, GdString path, GdFloat rot) {
	check_and_get_sprite_v()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_rotation(rot);
	}
}

GdFloat SpxSpriteMgr::get_child_rotation(GdObj obj, GdString path) {
	check_and_get_sprite_r(0)
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		return child->get_rotation();
	}
	return 0;
}

void SpxSpriteMgr::set_child_scale(GdObj obj, GdString path, GdVec2 scale) {
	check_and_get_sprite_v()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_scale(scale);
	}
}

GdVec2 SpxSpriteMgr::get_child_scale(GdObj obj, GdString path) {
	check_and_get_sprite_r(GdVec2())
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		return child->get_scale();
	}
	return GdVec2();
}

GdBool SpxSpriteMgr::check_collision(GdObj obj, GdObj target, GdBool is_src_trigger, GdBool is_dst_trigger) {
	check_and_get_sprite_r(false)
	check_and_get_target_sprite_r(target,false)
	return sprite->check_collision(sprite_target,is_src_trigger,is_dst_trigger);
}

GdBool SpxSpriteMgr::check_collision_with_point(GdObj obj, GdVec2 point, GdBool is_trigger) {
	check_and_get_sprite_r(false)
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
	check_and_get_sprite_r(NULL_OBJECT_ID)
	sprite = dynamic_cast<SpxSprite *>(sprite->duplicate());
	sprite->set_gid(get_unique_id());
	sprite_root->add_child(sprite);
	sprite->on_start();
	SPX_CALLBACK->func_on_sprite_ready(sprite->get_gid());
	return sprite->get_gid();
}

GdBool SpxSpriteMgr::destroy_sprite(GdObj obj) {
	check_and_get_sprite_r(false)
	sprite->set_block_signals(true);
	sprite->queue_free();
	return true;
}

GdBool SpxSpriteMgr::is_sprite_alive(GdObj obj) {
	return get_sprite(obj) != nullptr;
}

void SpxSpriteMgr::set_position(GdObj obj, GdVec2 pos) {
	check_and_get_sprite_v()
	// flip y axis
	sprite->set_position(GdVec2(pos.x, -pos.y));
}

void SpxSpriteMgr::set_rotation(GdObj obj, GdFloat rot) {
	check_and_get_sprite_v()
	sprite->set_rotation(rot);
}

void SpxSpriteMgr::set_scale(GdObj obj, GdVec2 scale) {
	check_and_get_sprite_v()
	sprite->set_scale(scale);
}

GdVec2 SpxSpriteMgr::get_position(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	auto pos = sprite->get_position();
	// flip y axis
	return GdVec2{ pos.x, -pos.y };
}

GdFloat SpxSpriteMgr::get_rotation(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_rotation();
}

GdVec2 SpxSpriteMgr::get_scale(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_scale();
}

void SpxSpriteMgr::set_render_scale(GdObj obj, GdVec2 scale) {
	check_and_get_sprite_v()
	sprite->set_render_scale(scale);
}
GdVec2 SpxSpriteMgr::get_render_scale(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_render_scale();
}

void SpxSpriteMgr::set_color(GdObj obj, GdColor color) {
	check_and_get_sprite_v()
	sprite->set_color(color);
}

GdColor SpxSpriteMgr::get_color(GdObj obj) {
	check_and_get_sprite_r(GdColor()) return sprite->get_color();
}

void SpxSpriteMgr::set_material_shader(GdObj obj, GdString path) {
	check_and_get_sprite_v()
	sprite->set_material_shader(path);
}

GdString SpxSpriteMgr::get_material_shader(GdObj obj) {
	check_and_get_sprite_r(GdString())
	return sprite->get_material_shader();
}

void SpxSpriteMgr::set_material_params(GdObj obj, GdString effect, GdFloat amount) {
	check_and_get_sprite_v()
	sprite->set_material_params(effect, amount);
}

GdFloat SpxSpriteMgr::get_material_params(GdObj obj, GdString effect) {
	check_and_get_sprite_r(GdFloat())
	return sprite->get_material_params(effect);
}

void SpxSpriteMgr::set_material_params_vec4(GdObj obj, GdString effect, GdVec4 vec4) {
	check_and_get_sprite_v()
	sprite->set_material_params_vec4(effect, vec4);
}

void SpxSpriteMgr::set_material_params_vec(GdObj obj, GdString effect,  GdFloat x, GdFloat y, GdFloat z, GdFloat w){
	check_and_get_sprite_v()
	sprite->set_material_params_vec4(effect, GdVec4(x,y,z,w));
}


GdVec4 SpxSpriteMgr::get_material_params_vec4(GdObj obj, GdString effect) {
	check_and_get_sprite_r(GdVec4())
	return sprite->get_material_params_vec4(effect);
}

void SpxSpriteMgr::set_material_params_color(GdObj obj, GdString effect, GdColor color) {
	check_and_get_sprite_v()
	sprite->set_material_params_color(effect, color);
}

GdColor SpxSpriteMgr::get_material_params_color(GdObj obj, GdString effect) {
	check_and_get_sprite_r(GdColor())
	return sprite->get_material_params_color(effect);
}

void SpxSpriteMgr::set_texture_atlas(GdObj obj, GdString path, GdRect2 rect2) {
	check_and_get_sprite_v()
	sprite->set_texture_atlas(path, rect2);
}

void SpxSpriteMgr::set_texture(GdObj obj, GdString path) {
	check_and_get_sprite_v()
	sprite->set_texture(path);
}

void SpxSpriteMgr::set_texture_atlas_direct(GdObj obj, GdString path, GdRect2 rect2) {
	check_and_get_sprite_v()
	sprite->set_texture_atlas_direct(path, rect2, true);
}

void SpxSpriteMgr::set_texture_direct(GdObj obj, GdString path) {
	check_and_get_sprite_v()
	sprite->set_texture_direct(path, true);
}

GdString SpxSpriteMgr::get_texture(GdObj obj) {
	check_and_get_sprite_r(GdString())
	return sprite->get_texture();
}

void SpxSpriteMgr::set_visible(GdObj obj, GdBool visible) {
	check_and_get_sprite_v()
	sprite->set_visible(visible);
	sprite->on_set_visible(visible);
}

GdBool SpxSpriteMgr::get_visible(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_visible();
}

GdInt SpxSpriteMgr::get_z_index(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_z_index();
}

void SpxSpriteMgr::set_z_index(GdObj obj, GdInt z) {
	check_and_get_sprite_v()
	sprite->set_z_index(z);
}

void SpxSpriteMgr::play_anim(GdObj obj, GdString p_name, GdFloat p_speed, GdBool isLoop, GdBool p_revert) {
	check_and_get_sprite_v()
	sprite->play_anim(p_name, p_speed, isLoop, p_revert);
}

void SpxSpriteMgr::play_backwards_anim(GdObj obj, GdString p_name) {
	check_and_get_sprite_v()
	sprite->play_backwards_anim(p_name);
}

void SpxSpriteMgr::pause_anim(GdObj obj) {
	check_and_get_sprite_v()
	sprite->pause_anim();
}

void SpxSpriteMgr::stop_anim(GdObj obj) {
	check_and_get_sprite_v()
	sprite->stop_anim();
}

GdBool SpxSpriteMgr::is_playing_anim(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_playing_anim();
}

void SpxSpriteMgr::set_anim(GdObj obj, GdString p_name) {
	check_and_get_sprite_v()
	sprite->set_anim(p_name);
}

GdString SpxSpriteMgr::get_anim(GdObj obj) {
	check_and_get_sprite_r(GdString())
	return sprite->get_anim();
}

void SpxSpriteMgr::set_anim_frame(GdObj obj, GdInt p_frame) {
	check_and_get_sprite_v()
	sprite->set_anim_frame(p_frame);
}

GdInt SpxSpriteMgr::get_anim_frame(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_anim_frame();
}

void SpxSpriteMgr::set_anim_speed_scale(GdObj obj, GdFloat p_speed_scale) {
	check_and_get_sprite_v()
	sprite->set_anim_speed_scale(p_speed_scale);
}

GdFloat SpxSpriteMgr::get_anim_speed_scale(GdObj obj) {
	check_and_get_sprite_r(1.0)
	return sprite->get_anim_speed_scale();
}

GdFloat SpxSpriteMgr::get_anim_playing_speed(GdObj obj) {
	check_and_get_sprite_r(1.0)
	return sprite->get_anim_playing_speed();
}

void SpxSpriteMgr::set_anim_centered(GdObj obj, GdBool p_center) {
	check_and_get_sprite_v()
	sprite->set_anim_centered(p_center);
}

GdBool SpxSpriteMgr::is_anim_centered(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_anim_centered();
}

void SpxSpriteMgr::set_anim_offset(GdObj obj, GdVec2 p_offset) {
	check_and_get_sprite_v()
	sprite->set_anim_offset(p_offset);
}

GdVec2 SpxSpriteMgr::get_anim_offset(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_anim_offset();
}

void SpxSpriteMgr::set_anim_flip_h(GdObj obj, GdBool p_flip) {
	check_and_get_sprite_v()
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
	check_and_get_sprite_v()
	sprite->set_anim_flip_v(p_flip);
}

GdBool SpxSpriteMgr::is_anim_flipped_v(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_anim_flipped_v();
}
GdString SpxSpriteMgr::get_current_anim_name(GdObj obj) {
	check_and_get_sprite_r(GdString())
	return sprite->get_current_anim_name();
}

void SpxSpriteMgr::set_velocity(GdObj obj, GdVec2 velocity) {
	check_and_get_sprite_v()
	// flip y axis
	sprite->set_velocity(GdVec2(velocity.x, -velocity.y));
}

GdVec2 SpxSpriteMgr::get_velocity(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	auto val = sprite->get_velocity();
	// flip y axis
	return GdVec2{ val.x, -val.y };
}

GdBool SpxSpriteMgr::is_on_floor(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_on_floor();
}

GdBool SpxSpriteMgr::is_on_floor_only(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_on_floor_only();
}

GdBool SpxSpriteMgr::is_on_wall(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_on_wall();
}

GdBool SpxSpriteMgr::is_on_wall_only(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_on_wall_only();
}

GdBool SpxSpriteMgr::is_on_ceiling(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_on_ceiling();
}

GdBool SpxSpriteMgr::is_on_ceiling_only(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_on_ceiling_only();
}

GdVec2 SpxSpriteMgr::get_last_motion(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_last_motion();
}

GdVec2 SpxSpriteMgr::get_position_delta(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_position_delta();
}

GdVec2 SpxSpriteMgr::get_floor_normal(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_floor_normal();
}

GdVec2 SpxSpriteMgr::get_wall_normal(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_wall_normal();
}

GdVec2 SpxSpriteMgr::get_real_velocity(GdObj obj) {
	check_and_get_sprite_r(GdVec2())
	return sprite->get_real_velocity();
}

void SpxSpriteMgr::move_and_slide(GdObj obj) {
	check_and_get_sprite_v()
	sprite->move_and_slide();
}

void SpxSpriteMgr::set_gravity(GdObj obj, GdFloat gravity) {
	check_and_get_sprite_v()
	sprite->set_gravity(gravity);
}

GdFloat SpxSpriteMgr::get_gravity(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_gravity();
}

void SpxSpriteMgr::set_mass(GdObj obj, GdFloat mass) {
	check_and_get_sprite_v()
	sprite->set_mass(mass);
}

GdFloat SpxSpriteMgr::get_mass(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_mass();
}

void SpxSpriteMgr::add_force(GdObj obj, GdVec2 force) {
	check_and_get_sprite_v()
	sprite->add_force(force);
}

void SpxSpriteMgr::add_impulse(GdObj obj, GdVec2 impulse) {
	check_and_get_sprite_v()
	sprite->add_impulse(impulse);
}


void SpxSpriteMgr::set_physics_mode(GdObj obj, GdInt mode) {
	check_and_get_sprite_v()
	sprite->set_physics_mode(mode);
}

GdInt SpxSpriteMgr::get_physics_mode(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_physics_mode();
}

void SpxSpriteMgr::set_use_gravity(GdObj obj, GdBool enabled) {
	check_and_get_sprite_v()
	sprite->set_use_gravity(enabled);
}

GdBool SpxSpriteMgr::is_use_gravity(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_use_gravity();
}

void SpxSpriteMgr::set_gravity_scale(GdObj obj, GdFloat scale) {
	check_and_get_sprite_v()
	sprite->set_gravity_scale(scale);
}

GdFloat SpxSpriteMgr::get_gravity_scale(GdObj obj) {
	check_and_get_sprite_r(1.0f)
	return sprite->get_gravity_scale();
}

void SpxSpriteMgr::set_drag(GdObj obj, GdFloat drag) {
	check_and_get_sprite_v()
	sprite->set_drag(drag);
}

GdFloat SpxSpriteMgr::get_drag(GdObj obj) {
	check_and_get_sprite_r(0.0f)
	return sprite->get_drag();
}

void SpxSpriteMgr::set_friction(GdObj obj, GdFloat friction) {
	check_and_get_sprite_v()
	sprite->set_friction(friction);
}

GdFloat SpxSpriteMgr::get_friction(GdObj obj) {
	check_and_get_sprite_r(0.0f)
	return sprite->get_friction();
}

void SpxSpriteMgr::set_collision_layer(GdObj obj, GdInt layer) {
	check_and_get_sprite_v()
	sprite->set_collision_layer((uint32_t)layer);
}

GdInt SpxSpriteMgr::get_collision_layer(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_collision_layer();
}

void SpxSpriteMgr::set_collision_mask(GdObj obj, GdInt mask) {
	check_and_get_sprite_v()
	sprite->set_collision_mask((uint32_t)mask);
}

GdInt SpxSpriteMgr::get_collision_mask(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_collision_mask();
}


void SpxSpriteMgr::set_trigger_layer(GdObj obj, GdInt layer) {
	check_and_get_sprite_v()
	sprite->set_trigger_layer(layer);
}

GdInt SpxSpriteMgr::get_trigger_layer(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_trigger_layer();
}

void SpxSpriteMgr::set_trigger_mask(GdObj obj, GdInt mask) {
	check_and_get_sprite_v()
	sprite->set_trigger_mask(mask);
}

GdInt SpxSpriteMgr::get_trigger_mask(GdObj obj) {
	check_and_get_sprite_r(0)
	return sprite->get_trigger_mask();
}

void SpxSpriteMgr::set_collider_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	check_and_get_sprite_v()
	sprite->set_collider_rect(center, size);
}

void SpxSpriteMgr::set_collider_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	check_and_get_sprite_v()
	sprite->set_collider_circle(center, radius);
}

void SpxSpriteMgr::set_collider_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	check_and_get_sprite_v()
	sprite->set_collider_capsule(center, size);
}

void SpxSpriteMgr::set_collision_enabled(GdObj obj, GdBool enabled) {
	check_and_get_sprite_v()
	sprite->set_collision_enabled(enabled);
}

GdBool SpxSpriteMgr::is_collision_enabled(GdObj obj) {
	check_and_get_sprite_r(false)
	return sprite->is_collision_enabled();
}

void SpxSpriteMgr::set_trigger_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	check_and_get_sprite_v()
	sprite->set_trigger_rect(center, size);
}

void SpxSpriteMgr::set_trigger_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	check_and_get_sprite_v()
	sprite->set_trigger_circle(center, radius);
}

void SpxSpriteMgr::set_trigger_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	check_and_get_sprite_v()
	sprite->set_trigger_capsule(center, size);
}

void SpxSpriteMgr::set_trigger_enabled(GdObj obj, GdBool trigger) {
	check_and_get_sprite_v()
	sprite->set_trigger_enabled(trigger);
}
GdBool SpxSpriteMgr::is_trigger_enabled(GdObj obj) {
	check_and_get_sprite_r(false)
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

// SIMD-optimized alpha collision check for a row of pixels
// Returns true if any pixel pair has both alphas > threshold
// Note: This function is currently not used but prepared for future optimization
static inline bool _check_row_alpha_simd(const uint8_t* row1_data, const uint8_t* row2_data, 
                                         int width, float alpha_threshold) {
	const uint8_t threshold_u8 = (uint8_t)(alpha_threshold * 255.0f);
	int x = 0;

#ifdef SPX_USE_WASM_SIMD
	// Process 16 pixels at a time with WebAssembly SIMD
	const int simd_width = 16;
	v128_t threshold_vec = wasm_i8x16_splat(threshold_u8);
	
	for (; x <= width - simd_width; x += simd_width) {
		// Load 16 RGBA pixels from each image (64 bytes each)
		// Extract alpha channels (every 4th byte starting from byte 3)
		v128_t data1_0 = wasm_v128_load(row1_data + x * 4);
		v128_t data1_1 = wasm_v128_load(row1_data + x * 4 + 16);
		v128_t data1_2 = wasm_v128_load(row1_data + x * 4 + 32);
		v128_t data1_3 = wasm_v128_load(row1_data + x * 4 + 48);
		
		v128_t data2_0 = wasm_v128_load(row2_data + x * 4);
		v128_t data2_1 = wasm_v128_load(row2_data + x * 4 + 16);
		v128_t data2_2 = wasm_v128_load(row2_data + x * 4 + 32);
		v128_t data2_3 = wasm_v128_load(row2_data + x * 4 + 48);
		
		// Extract alpha channels (byte 3,7,11,15 from each 16-byte chunk)
		// Use shuffle to extract alphas
		v128_t shuffle_mask = wasm_i8x16_const(3, 7, 11, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
		v128_t a1_0 = wasm_i8x16_swizzle(data1_0, shuffle_mask);
		v128_t a1_1 = wasm_i8x16_swizzle(data1_1, shuffle_mask);
		v128_t a1_2 = wasm_i8x16_swizzle(data1_2, shuffle_mask);
		v128_t a1_3 = wasm_i8x16_swizzle(data1_3, shuffle_mask);
		
		v128_t a2_0 = wasm_i8x16_swizzle(data2_0, shuffle_mask);
		v128_t a2_1 = wasm_i8x16_swizzle(data2_1, shuffle_mask);
		v128_t a2_2 = wasm_i8x16_swizzle(data2_2, shuffle_mask);
		v128_t a2_3 = wasm_i8x16_swizzle(data2_3, shuffle_mask);
		
		// Pack 16 alpha values: combine 4 bytes from each chunk
		// Create one vector with all 16 alpha values
		v128_t alpha1 = wasm_i32x4_shuffle(a1_0, a1_1, 0, 4, 1, 5);
		alpha1 = wasm_i32x4_shuffle(alpha1, wasm_i32x4_shuffle(a1_2, a1_3, 0, 4, 1, 5), 0, 1, 4, 5);
		
		v128_t alpha2 = wasm_i32x4_shuffle(a2_0, a2_1, 0, 4, 1, 5);
		alpha2 = wasm_i32x4_shuffle(alpha2, wasm_i32x4_shuffle(a2_2, a2_3, 0, 4, 1, 5), 0, 1, 4, 5);
		
		// Compare with threshold (unsigned)
		v128_t cmp1 = wasm_u8x16_gt(alpha1, threshold_vec);
		v128_t cmp2 = wasm_u8x16_gt(alpha2, threshold_vec);
		
		// AND the comparisons
		v128_t both = wasm_v128_and(cmp1, cmp2);
		
		// Check if any byte is set
		if (wasm_v128_any_true(both)) {
			return true;
		}
	}
#elif defined(SPX_USE_NEON)
	// Process 16 pixels at a time with NEON (best SIMD for alpha extraction)
	const int simd_width = 16;
	uint8x16_t threshold_vec = vdupq_n_u8(threshold_u8);
	
	for (; x <= width - simd_width; x += simd_width) {
		// Load and extract alpha channels (every 4th byte)
		uint8x16x4_t data1 = vld4q_u8(row1_data + x * 4);
		uint8x16x4_t data2 = vld4q_u8(row2_data + x * 4);
		
		// data1.val[3] and data2.val[3] contain the alpha channels
		uint8x16_t alpha1 = data1.val[3];
		uint8x16_t alpha2 = data2.val[3];
		
		// Compare with threshold
		uint8x16_t cmp1 = vcgtq_u8(alpha1, threshold_vec);
		uint8x16_t cmp2 = vcgtq_u8(alpha2, threshold_vec);
		
		// AND the comparisons
		uint8x16_t both = vandq_u8(cmp1, cmp2);
		
		// Check if any byte is set
		uint64x2_t both64 = vreinterpretq_u64_u8(both);
		if (vgetq_lane_u64(both64, 0) != 0 || vgetq_lane_u64(both64, 1) != 0) {
			return true;
		}
	}
#elif defined(SPX_USE_SSE2)
	// Process 4 pixels at a time with SSE2 (simpler approach, no SSSE3 shuffle needed)
	const int simd_width = 4;
	__m128i threshold_vec = _mm_set1_epi8(threshold_u8);
	
	for (; x <= width - simd_width; x += simd_width) {
		// Load 4 RGBA pixels (16 bytes each sprite)
		__m128i data1 = _mm_loadu_si128((__m128i*)(row1_data + x * 4));
		__m128i data2 = _mm_loadu_si128((__m128i*)(row2_data + x * 4));
		
		// Shift right by 24 bits to get alpha channel in lower byte (R>>24)
		// Then AND with 0xFF to isolate alpha
		// This extracts bytes 3,7,11,15 (alpha channels)
		__m128i alpha1_shifted = _mm_srli_epi32(data1, 24);
		__m128i alpha2_shifted = _mm_srli_epi32(data2, 24);
		
		// Pack to compare (only need lower 8 bits of each 32-bit value)
		__m128i alpha1 = _mm_packus_epi16(_mm_packs_epi32(alpha1_shifted, alpha1_shifted), _mm_setzero_si128());
		__m128i alpha2 = _mm_packus_epi16(_mm_packs_epi32(alpha2_shifted, alpha2_shifted), _mm_setzero_si128());
		
		// Compare with threshold
		__m128i cmp1 = _mm_cmpgt_epi8(alpha1, threshold_vec);
		__m128i cmp2 = _mm_cmpgt_epi8(alpha2, threshold_vec);
		
		// AND the comparisons
		__m128i both = _mm_and_si128(cmp1, cmp2);
		
		// Check if any byte is set
		if (_mm_movemask_epi8(both) != 0) {
			return true;
		}
	}
#endif

	// Scalar fallback for remaining pixels
	for (; x < width; x++) {
		uint8_t a1 = row1_data[x * 4 + 3]; // Alpha is the 4th channel
		uint8_t a2 = row2_data[x * 4 + 3];
		if (a1 > threshold_u8 && a2 > threshold_u8) {
			return true;
		}
	}
	
	return false;
}

GdBool SpxSpriteMgr::check_collision_with_sprite_by_alpha(GdObj obj, GdObj obj_b, GdFloat alpha_threshold){
	check_and_get_sprite_r(false) // Ensure sprite exists

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

	auto sp2 = get_sprite(obj_b);
	if (sp2 == nullptr) {
		print_error("try to get property of a null sprite gid=" + itos(obj));
		return false;
	}

	AnimatedSprite2D *anim2 = sp2->anim2d;
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

	// Optimization 1: Coarse sampling first - check every N pixels
	// This provides early detection at ~1/N² cost
	const int COARSE_STEP = 4;
	int overlap_x_end = (int)(overlap.position.x + overlap.size.x);
	int overlap_y_end = (int)(overlap.position.y + overlap.size.y);
	int overlap_x_start = (int)overlap.position.x;
	int overlap_y_start = (int)overlap.position.y;
	
	// Pre-calculate bounds to avoid repeated checks
	int size1_x_max = size1.x - 1;
	int size1_y_max = size1.y - 1;
	int size2_x_max = size2.x - 1;
	int size2_y_max = size2.y - 1;

	// Coarse pass: sample with larger steps
	for (int x = overlap_x_start; x < overlap_x_end; x += COARSE_STEP) {
		for (int y = overlap_y_start; y < overlap_y_end; y += COARSE_STEP) {
			Vector2 local_pos1 = _to_image_coord(trans1, size1, Vector2(x, y));
			Vector2 local_pos2 = _to_image_coord(trans2, size2, Vector2(x, y));

			// Use integer comparison for speed
			int lp1_x = (int)local_pos1.x;
			int lp1_y = (int)local_pos1.y;
			int lp2_x = (int)local_pos2.x;
			int lp2_y = (int)local_pos2.y;

			if (lp1_x >= 0 && lp1_x <= size1_x_max && lp1_y >= 0 && lp1_y <= size1_y_max &&
					lp2_x >= 0 && lp2_x <= size2_x_max && lp2_y >= 0 && lp2_y <= size2_y_max) {
				Color color1 = image1->get_pixel(lp1_x, lp1_y);
				Color color2 = image2->get_pixel(lp2_x, lp2_y);

				if (color1.a > alpha_threshold && color2.a > alpha_threshold) {
					// Found potential collision in coarse pass
					// Do fine-grained check in small area around this point
					int fine_x_start = MAX(overlap_x_start, x - COARSE_STEP);
					int fine_x_end = MIN(overlap_x_end, x + COARSE_STEP);
					int fine_y_start = MAX(overlap_y_start, y - COARSE_STEP);
					int fine_y_end = MIN(overlap_y_end, y + COARSE_STEP);

					for (int fx = fine_x_start; fx < fine_x_end; fx++) {
						for (int fy = fine_y_start; fy < fine_y_end; fy++) {
							Vector2 fine_pos1 = _to_image_coord(trans1, size1, Vector2(fx, fy));
							Vector2 fine_pos2 = _to_image_coord(trans2, size2, Vector2(fx, fy));

							int fp1_x = (int)fine_pos1.x;
							int fp1_y = (int)fine_pos1.y;
							int fp2_x = (int)fine_pos2.x;
							int fp2_y = (int)fine_pos2.y;

							if (fp1_x >= 0 && fp1_x <= size1_x_max && fp1_y >= 0 && fp1_y <= size1_y_max &&
									fp2_x >= 0 && fp2_x <= size2_x_max && fp2_y >= 0 && fp2_y <= size2_y_max) {
								Color fc1 = image1->get_pixel(fp1_x, fp1_y);
								Color fc2 = image2->get_pixel(fp2_x, fp2_y);

								if (fc1.a > alpha_threshold && fc2.a > alpha_threshold) {
									return true;
								}
							}
						}
					}
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
	check_and_get_sprite_r(false) // Ensure sprite exists

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
		if (sprite == sp2) {
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
		for (int x = overlap.position.x; x < overlap.position.x + overlap.size.x; x++) {
			for (int y = overlap.position.y; y < overlap.position.y + overlap.size.y; y++) {
				Vector2 local_pos1 = _to_image_coord(trans1, size1, Vector2(x, y));
				Vector2 local_pos2 = _to_image_coord(trans2, size2, Vector2(x, y));

				if (local_pos1.x >= 0 && local_pos1.x <= size1.x - 1 && local_pos1.y >= 0 && local_pos1.y <= size1.y - 1 &&
						local_pos2.x >= 0 && local_pos2.x <= size2.x - 1 && local_pos2.y >= 0 && local_pos2.y <= size2.y - 1) {
					Color color1 = image1->get_pixel((int)local_pos1.x, (int)local_pos1.y);
					Color color2 = image2->get_pixel((int)local_pos2.x, (int)local_pos2.y);
					if (check_func(color1, color2)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}


void SpxSpriteMgr::on_trigger_enter(GdInt self_id, GdInt other_id){
	if(physicMgr->is_collision_by_pixel){
		bounding_collision_pairs.insert(TriggerPair(self_id, other_id));
	}else{
		SPX_CALLBACK->func_on_trigger_enter(self_id, other_id);
	}
}
void SpxSpriteMgr::on_trigger_exit(GdInt self_id, GdInt other_id){
	if(physicMgr->is_collision_by_pixel){
		bounding_collision_pairs.erase(TriggerPair(self_id, other_id));
	}else{
		SPX_CALLBACK->func_on_trigger_exit(self_id, other_id);
	}
}

void SpxSpriteMgr::_check_pixel_collision_events() {
	// trigger pixel collision events
	if(physicMgr->is_collision_by_pixel){
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
			auto is_collide = check_collision_with_sprite_by_alpha(trigger.id1, trigger.id2, DEFAULT_COLLISION_ALPHA_THRESHOLD);
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
	check_and_get_sprite_v()
	pivot.y = - pivot.y;
	sprite->set_pivot(pivot);
}
GdVec2 SpxSpriteMgr::get_pivot(GdObj obj){
	check_and_get_sprite_r(GdVec2())
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
	
	// Read header
	int update_count = static_cast<int>(*(SpxBaseMgr::get_array<float>(buffer, 0)));
	int delete_count = static_cast<int>(*(SpxBaseMgr::get_array<float>(buffer, 1)));
	
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
		// Extract sprite ID and data
		auto sprite_id = static_cast<GdObj>(*(SpxBaseMgr::get_array<float>(buffer, idx)));
		auto x = *(SpxBaseMgr::get_array<float>(buffer, idx + 1));
		auto y = *(SpxBaseMgr::get_array<float>(buffer, idx + 2));
		auto rotation = *(SpxBaseMgr::get_array<float>(buffer, idx + 3));
		auto scale_x = *(SpxBaseMgr::get_array<float>(buffer, idx + 4));
		auto scale_y = *(SpxBaseMgr::get_array<float>(buffer, idx + 5));
		// Note: offsetX and offsetY (idx+6, idx+7) are reserved for future use
		auto visible = *(SpxBaseMgr::get_array<float>(buffer, idx + 8)) != 0.0;
		
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
	}
	
	// Process deletes
	for (int i = 0; i < delete_count; i++) {
		auto sprite_id = static_cast<GdObj>(*(SpxBaseMgr::get_array<float>(buffer, idx)));
		idx++;
		
		// Get sprite and destroy it
		SpxSprite* sprite = get_sprite(sprite_id);
		if (sprite != nullptr) {
			sprite->set_block_signals(true);
			sprite->queue_free();
		}
	}
}
