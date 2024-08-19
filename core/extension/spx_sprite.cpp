/**************************************************************************/
/*  spx_sprite.cpp                                                        */
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

#include "spx_sprite.h"

#include "spx_engine.h"
#include "spx_sprite_mgr.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/area_2d.h"
#include "scene/2d/collision_shape_2d.h"
#define SPX_CALLBACK SpxEngine::get_singleton()->get_callbacks()

Node *SpxSprite::get_component(Node *node, StringName name, GdBool recursive) {
	for (int i = 0; i < node->get_child_count(); ++i) {
		Node *child = node->get_child(i);

		if (child->get_name() == name) {
			return child;
		}

		if (recursive) {
			Node *found_node = get_component(child, name, true);
			if (found_node != nullptr) {
				return found_node;
			}
		}
	}
	return nullptr;
}


void SpxSprite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "id"), &SpxSprite::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &SpxSprite::get_id);
	ClassDB::bind_method(D_METHOD("on_destroy_call"), &SpxSprite::on_destroy_call);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "obj_id"), "set_id", "get_id");
	ClassDB::bind_method(D_METHOD("on_area_entered", "area"), &SpxSprite::on_area_entered);
	ClassDB::bind_method(D_METHOD("on_area_exited", "area"), &SpxSprite::on_area_exited);
}

void SpxSprite::set_id(GdObj id) {
	this->obj_id = id;
}

GdObj SpxSprite::get_id() {
	return obj_id;
}

void SpxSprite::on_destroy_call() {
	spriteMgr->on_sprite_destroy(this);
}

SpxSprite::SpxSprite() {
}

SpxSprite::~SpxSprite() {
}

void SpxSprite::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PREDELETE: {
			on_destroy_call();
			break;
		}
		case NOTIFICATION_READY: {
			//set_process(true);
			break;
		}
		default:
			break;
	}
}

void SpxSprite::on_start() {
	collider = get_component<CollisionShape2D>();
	animation = get_component<AnimatedSprite2D>();
	area = get_component<Area2D>();

	if (area) {
		area->connect("area_entered", Callable(this, "on_area_entered"));
		area->connect("area_exited", Callable(this, "on_area_exited"));
	}
}

void SpxSprite::on_area_entered(Node *node) {
	print_line("on_area_entered " + node->get_name());
	Node *parent_node = node->get_parent();
	const SpxSprite *other = Object::cast_to<SpxSprite>(parent_node);
	if (other != nullptr) {
		SPX_CALLBACK->func_on_collision_enter(this->obj_id, other->obj_id);
	}
}

void SpxSprite::on_area_exited(Node *node) {
	print_line("on_area_exited " + node->get_name());
	Node *parent_node = node->get_parent();
	const SpxSprite *other = Object::cast_to<SpxSprite>(parent_node);
	if (other != nullptr) {
		SPX_CALLBACK->func_on_trigger_exit(this->obj_id, other->obj_id);
	}
}

void SpxSprite::set_color(GdColor color) {
	animation->set_self_modulate(color);
}

GdColor SpxSprite::get_color() {
	return animation->get_self_modulate();
}

void SpxSprite::set_texture(GdString path) {
	return animation->set_animation(SpxStr(path));
}

GdString SpxSprite::get_texture() {
	return animation->get_animation();
}

void SpxSprite::play_anim(const StringName &p_name, GdFloat p_custom_scale, GdBool p_from_end) {
	animation->play(p_name, p_custom_scale, p_from_end);
}

void SpxSprite::play_backwards_anim(const StringName &p_name) {
	animation->play_backwards(p_name);
}

void SpxSprite::pause_anim() {
	animation->pause();
}

void SpxSprite::stop_anim() {
	animation->stop();
}

GdBool SpxSprite::is_playing_anim() const {
	return animation->is_playing();
}

void SpxSprite::set_anim(const StringName &p_name) {
	animation->set_animation(p_name);
}

StringName SpxSprite::get_anim() const {
	return animation->get_animation();
}

void SpxSprite::set_anim_frame(GdInt p_frame) {
	animation->set_frame(p_frame);
}

GdInt SpxSprite::get_anim_frame() const {
	return animation->get_frame();
}

void SpxSprite::set_anim_speed_scale(GdFloat p_speed_scale) {
	animation->set_speed_scale(p_speed_scale);
}

GdFloat SpxSprite::get_anim_speed_scale() const {
	return animation->get_speed_scale();
}

GdFloat SpxSprite::get_anim_playing_speed() const {
	return animation->get_playing_speed();
}

void SpxSprite::set_anim_centered(GdBool p_center) {
	animation->set_centered(p_center);
}

GdBool SpxSprite::is_anim_centered() const {
	return animation->is_centered();
}

void SpxSprite::set_anim_offset(const GdVec2 &p_offset) {
	animation->set_offset(p_offset);
}

GdVec2 SpxSprite::get_anim_offset() const {
	return animation->get_offset();
}

void SpxSprite::set_anim_flip_h(GdBool p_flip) {
	animation->set_flip_h(p_flip);
}

GdBool SpxSprite::is_anim_flipped_h() const {
	return animation->is_flipped_h();
}

void SpxSprite::set_anim_flip_v(GdBool p_flip) {
	animation->set_flip_v(p_flip);
}

GdBool SpxSprite::is_anim_flipped_v() const {
	return animation->is_flipped_v();
}
