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

#include "scene/main/node.h"
#include "spx_engine.h"
#include "spx_sprite.h"
#include "core/templates/rb_map.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/area_2d.h"
#include "scene/2d/collision_shape_2d.h"
#include "scene/2d/physics_body_2d.h"
#include "scene/resources/circle_shape_2d.h"
#include "scene/resources/packed_scene.h"

StringName SpxSpriteMgr::default_texture_anim = "";
#define get_check_sprite_r(VALUE) \
	auto sprite = get_sprite(obj);\
	if (sprite == nullptr) {\
		print_error("try to get property of a null sprite gid=" + itos(obj)); \
		return VALUE; \
	}

#define get_check_sprite_v() \
	auto sprite = get_sprite(obj);\
	if (sprite == nullptr) {\
		print_error("try to get property of a null sprite gid=" + itos(obj)); \
		return ; \
	}


#define SPX_CALLBACK SpxEngine::get_singleton()->get_callbacks()

void SpxSpriteMgr::on_start() {
	SpxBaseMgr::on_start();
	default_texture_anim = "default";
}

void SpxSpriteMgr::on_destroy() {
	SpxBaseMgr::on_destroy();
}

void SpxSpriteMgr::on_update(float delta) {
	SpxBaseMgr::on_update(delta);
}

SpxSprite *SpxSpriteMgr::get_sprite(GdObj obj) {
	if (id_sprites.has(obj)) {
		return id_sprites[obj];
	}
	return nullptr;
}

void SpxSpriteMgr::on_sprite_destroy(SpxSprite *sprite) {
	if (id_sprites.erase(sprite->get_gid())) {
		SPX_CALLBACK->func_on_sprite_destroyed(sprite->get_gid());
	}
}


// sprite
GdInt SpxSpriteMgr::create_sprite(GdString path) {
	const String path_str = String(*(const String *)path);
	SpxSprite *sprite = nullptr;
	if (path_str == "") {
		sprite = memnew(SpxSprite);
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
			return NULL_SPRITE_ID;
		} else {
			sprite = (SpxSprite *)scene->instantiate();
		}
	}
	sprite->set_gid(get_unique_id());
	get_root_node()->add_child(sprite);
	sprite->on_start();
	spriteMgr->id_sprites[sprite->get_gid()] = sprite;
	SPX_CALLBACK->func_on_sprite_ready(sprite->get_gid());
	return sprite->get_gid();
}

GdInt SpxSpriteMgr::clone_sprite(GdObj obj) {
	get_check_sprite_r(NULL_SPRITE_ID)
	sprite = dynamic_cast<SpxSprite *>(sprite->duplicate());
	sprite->set_gid(get_unique_id());
	get_root_node()->add_child(sprite);
	sprite->on_start();
	SPX_CALLBACK->func_on_sprite_ready(sprite->get_gid());
	return sprite->get_gid();
}

GdBool SpxSpriteMgr::destroy_sprite(GdObj obj) {
	get_check_sprite_r(false)
	sprite->queue_free();
	return true;
}

GdBool SpxSpriteMgr::is_sprite_alive(GdObj obj) {
	return get_sprite(obj) != nullptr;
}

void SpxSpriteMgr::set_position(GdObj obj, GdVec2 pos) {
	get_check_sprite_v()
	// flip y axis
	sprite->set_position(GdVec2(pos.x,-pos.y));
}

void SpxSpriteMgr::set_rotation(GdObj obj, float rot) {
	get_check_sprite_v()
	sprite->set_rotation(rot);
}

void SpxSpriteMgr::set_scale(GdObj obj, GdVec2 scale) {
	get_check_sprite_v()
	sprite->set_scale(scale);
}

GdVec2 SpxSpriteMgr::get_position(GdObj obj) {
	get_check_sprite_r(GdVec2())
	auto pos = sprite->get_position();
	// flip y axis
	return GdVec2{pos.x,-pos.y};
}

GdFloat SpxSpriteMgr::get_rotation(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_rotation();
}
GdVec2 SpxSpriteMgr::get_scale(GdObj obj) {
	get_check_sprite_r(GdVec2())
	return sprite->get_scale();
}

void SpxSpriteMgr::set_color(GdObj obj, GdColor color) {
	get_check_sprite_v()
	sprite->set_color(color);
}

GdColor SpxSpriteMgr::get_color(GdObj obj) {
	get_check_sprite_r(GdColor())
	return sprite->get_color();
}

void SpxSpriteMgr::set_texture(GdObj obj, GdString path) {
	get_check_sprite_v()
	sprite->set_texture(path);
}

GdString SpxSpriteMgr::get_texture(GdObj obj) {
	get_check_sprite_r(GdString())
	return sprite->get_texture();
}

void SpxSpriteMgr::set_visible(GdObj obj, GdBool visible) {
	get_check_sprite_v()
	sprite->set_visible(visible);
}

GdBool SpxSpriteMgr::get_visible(GdObj obj) {
	get_check_sprite_r(false)
	return sprite->is_visible();
}

GdInt SpxSpriteMgr::get_z_index(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_z_index();
}

void SpxSpriteMgr::set_z_index(GdObj obj, GdInt z) {
	get_check_sprite_v()
	sprite->set_z_index(z);
}

void SpxSpriteMgr::play_anim(GdObj obj, GdString p_name, GdFloat p_custom_scale, GdBool p_from_end) {
	get_check_sprite_v()
	sprite->play_anim(p_name, p_custom_scale, p_from_end);
}

void SpxSpriteMgr::play_backwards_anim(GdObj obj, GdString p_name) {
	get_check_sprite_v()
	sprite->play_backwards_anim(p_name);
}

void SpxSpriteMgr::pause_anim(GdObj obj) {
	get_check_sprite_v()
	sprite->pause_anim();
}

void SpxSpriteMgr::stop_anim(GdObj obj) {
	get_check_sprite_v()
	sprite->stop_anim();
}

GdBool SpxSpriteMgr::is_playing_anim(GdObj obj) {
	get_check_sprite_r(false)
	return sprite->is_playing_anim();
}

void SpxSpriteMgr::set_anim(GdObj obj, GdString p_name) {
	get_check_sprite_v()
	sprite->set_anim(p_name);
}

GdString SpxSpriteMgr::get_anim(GdObj obj) {
	get_check_sprite_r(GdString())
	return  sprite->get_anim();
}

void SpxSpriteMgr::set_anim_frame(GdObj obj, GdInt p_frame) {
	get_check_sprite_v()
	sprite->set_anim_frame(p_frame);
}

GdInt SpxSpriteMgr::get_anim_frame(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_anim_frame();
}

void SpxSpriteMgr::set_anim_speed_scale(GdObj obj, GdFloat p_speed_scale) {
	get_check_sprite_v()
	sprite->set_anim_speed_scale(p_speed_scale);
}

GdFloat SpxSpriteMgr::get_anim_speed_scale(GdObj obj) {
	get_check_sprite_r(1.0)
	return sprite->get_anim_speed_scale();
}

GdFloat SpxSpriteMgr::get_anim_playing_speed(GdObj obj) {
	get_check_sprite_r(1.0)
	return sprite->get_anim_playing_speed();
}

void SpxSpriteMgr::set_anim_centered(GdObj obj, GdBool p_center) {
	get_check_sprite_v()
	sprite->set_anim_centered(p_center);
}

GdBool SpxSpriteMgr::is_anim_centered(GdObj obj) {
	get_check_sprite_r(false)
	return sprite->is_anim_centered();
}

void SpxSpriteMgr::set_anim_offset(GdObj obj, GdVec2 p_offset) {
	get_check_sprite_v()
	sprite->set_anim_offset(p_offset);
}

GdVec2 SpxSpriteMgr::get_anim_offset(GdObj obj) {
	get_check_sprite_r(GdVec2())
	return sprite->get_anim_offset();
}

void SpxSpriteMgr::set_anim_flip_h(GdObj obj, GdBool p_flip) {
	get_check_sprite_v()
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
	get_check_sprite_v()
	sprite->set_anim_flip_v(p_flip);
}

GdBool SpxSpriteMgr::is_anim_flipped_v(GdObj obj) {
	get_check_sprite_r(false)
	return sprite->is_anim_flipped_v();
}

void SpxSpriteMgr::set_gravity(GdObj obj, GdFloat gravity) {
	get_check_sprite_v()
	sprite->set_gravity(gravity);
}

GdFloat SpxSpriteMgr::get_gravity(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_gravity();
}

void SpxSpriteMgr::set_mass(GdObj obj, GdFloat mass) {
	get_check_sprite_v()
	sprite->set_mass(mass);
}

GdFloat SpxSpriteMgr::get_mass(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_mass();
}

void SpxSpriteMgr::add_force(GdObj obj, GdVec2 force) {
	get_check_sprite_v()
	sprite->add_force(force);
}

void SpxSpriteMgr::add_impulse(GdObj obj, GdVec2 impulse) {
	get_check_sprite_v()
	sprite->add_impulse(impulse);
}

void SpxSpriteMgr::set_collision_layer(GdObj obj, GdInt layer) {
	get_check_sprite_v()
	sprite->set_collision_layer((uint32_t)layer);
}

GdInt SpxSpriteMgr::get_collision_layer(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_collision_layer();
}

void SpxSpriteMgr::set_collision_mask(GdObj obj, GdInt mask) {
	get_check_sprite_v()
	sprite->set_collision_mask((uint32_t)mask);
}

GdInt SpxSpriteMgr::get_collision_mask(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_collision_mask();
}


void SpxSpriteMgr::set_trigger_layer(GdObj obj, GdInt layer) {
	get_check_sprite_v()
	sprite->set_trigger_layer(layer);
}

GdInt SpxSpriteMgr::get_trigger_layer(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_trigger_layer();
}

void SpxSpriteMgr::set_trigger_mask(GdObj obj, GdInt mask) {
	get_check_sprite_v()
	sprite->set_trigger_mask(mask);
}

GdInt SpxSpriteMgr::get_trigger_mask(GdObj obj) {
	get_check_sprite_r(0)
	return sprite->get_trigger_mask();
}

void SpxSpriteMgr::set_collider_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	get_check_sprite_v()
	sprite->set_collider_rect(center, size);
}

void SpxSpriteMgr::set_collider_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	get_check_sprite_v()
	sprite->set_collider_circle(center, radius);
}

void SpxSpriteMgr::set_collider_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	get_check_sprite_v()
	sprite->set_collider_capsule(center, size);
}

void SpxSpriteMgr::set_collision_enabled(GdObj obj, GdBool enabled) {
	get_check_sprite_v()
	sprite->set_collision_enabled(enabled);
}

GdBool SpxSpriteMgr::is_collision_enabled(GdObj obj) {
	get_check_sprite_r(false)
	return sprite->is_collision_enabled();
}

void SpxSpriteMgr::set_trigger_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	get_check_sprite_v()
	sprite->set_trigger_rect(center, size);
}

void SpxSpriteMgr::set_trigger_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	get_check_sprite_v()
	sprite->set_trigger_circle(center, radius);
}

void SpxSpriteMgr::set_trigger_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	get_check_sprite_v()
	sprite->set_trigger_capsule(center, size);
}

void SpxSpriteMgr::set_trigger_enabled(GdObj obj, GdBool trigger) {
	get_check_sprite_v()
	sprite->set_trigger_enabled(trigger);
}

GdBool SpxSpriteMgr::is_trigger_enabled(GdObj obj) {
	get_check_sprite_r(false)
	return sprite->is_trigger_enabled();
}
