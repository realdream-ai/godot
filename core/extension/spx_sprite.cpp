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

#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/area_2d.h"
#include "scene/2d/collision_shape_2d.h"
#include "scene/2d/visible_on_screen_notifier_2d.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/capsule_shape_2d.h"
#include "scene/resources/circle_shape_2d.h"
#include "scene/resources/rectangle_shape_2d.h"
#include "spx.h"
#include "spx_engine.h"
#include "spx_sprite_mgr.h"
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
	ClassDB::bind_method(D_METHOD("set_gid", "gid"), &SpxSprite::set_gid);
	ClassDB::bind_method(D_METHOD("get_gid"), &SpxSprite::get_gid);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gid"), "set_gid", "get_gid");

	ClassDB::bind_method(D_METHOD("set_spx_type_name", "spx_type_name"), &SpxSprite::set_spx_type_name);
	ClassDB::bind_method(D_METHOD("get_spx_type_name"), &SpxSprite::get_spx_type_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "spx_type_name"), "set_spx_type_name", "get_spx_type_name");

	ClassDB::bind_method(D_METHOD("on_destroy_call"), &SpxSprite::on_destroy_call);

	ClassDB::bind_method(D_METHOD("on_area_entered", "area"), &SpxSprite::on_area_entered);
	ClassDB::bind_method(D_METHOD("on_area_exited", "area"), &SpxSprite::on_area_exited);

	ClassDB::bind_method(D_METHOD("on_sprite_frames_set_changed"), &SpxSprite::on_sprite_frames_set_changed);
	ClassDB::bind_method(D_METHOD("on_sprite_animation_changed"), &SpxSprite::on_sprite_animation_changed);
	ClassDB::bind_method(D_METHOD("on_sprite_frame_changed"), &SpxSprite::on_sprite_frame_changed);
	ClassDB::bind_method(D_METHOD("on_sprite_animation_looped"), &SpxSprite::on_sprite_animation_looped);
	ClassDB::bind_method(D_METHOD("on_sprite_animation_finished"), &SpxSprite::on_sprite_animation_finished);
	ClassDB::bind_method(D_METHOD("on_sprite_vfx_finished"), &SpxSprite::on_sprite_vfx_finished);
	ClassDB::bind_method(D_METHOD("on_sprite_screen_exited"), &SpxSprite::on_sprite_screen_exited);
	ClassDB::bind_method(D_METHOD("on_sprite_screen_entered"), &SpxSprite::on_sprite_screen_entered);
}

void SpxSprite::on_destroy_call() {
	if (!Spx::initialed)
		return;
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
		case NOTIFICATION_DRAW: {
			_draw();
			break;
		}
		default:
			break;
	}
}

void SpxSprite::_draw() {
	if (!Spx::debug_mode) {
		return;
	}
	if(trigger2d != nullptr) {
		trigger2d->set_spx_debug_color(Color(1, 0, 0, 0.2));
	}
	if(collider2d != nullptr) {
		collider2d->set_spx_debug_color(Color(0, 0, 1, 0.2));
	}
}

void SpxSprite::on_start() {
	collider2d = (get_component<CollisionShape2D>());
	anim2d = (get_component<AnimatedSprite2D>());
	if (anim2d->get_sprite_frames() == nullptr) {
		anim2d->set_sprite_frames(memnew(SpriteFrames));
	}
	visible_notifier = (get_component<VisibleOnScreenNotifier2D>());
	if (visible_notifier == nullptr) {
		visible_notifier = memnew(VisibleOnScreenNotifier2D);
		add_child(visible_notifier);
	}
	//anim2d->get_sprite_frames()->add_animation(SpxSpriteMgr::default_texture_anim);
	area2d = (get_component<Area2D>());
	if (area2d != nullptr) {
		trigger2d = get_component<CollisionShape2D>(area2d);
	}

	if (area2d != nullptr) {
		area2d->connect("area_entered", Callable(this, "on_area_entered"));
		area2d->connect("area_exited", Callable(this, "on_area_exited"));
	}
	if (anim2d != nullptr) {
		anim2d->connect("sprite_frames_changed", Callable(this, "on_sprite_frames_set_changed"));
		anim2d->connect("animation_changed", Callable(this, "on_sprite_animation_changed"));
		anim2d->connect("frame_changed", Callable(this, "on_sprite_frame_changed"));
		anim2d->connect("animation_looped", Callable(this, "on_sprite_animation_looped"));
		anim2d->connect("animation_finished", Callable(this, "on_sprite_animation_finished"));
	}
	if (visible_notifier != nullptr) {
		visible_notifier->connect("screen_exited", Callable(this, "on_sprite_screen_exited"));
		visible_notifier->connect("screen_entered", Callable(this, "on_sprite_screen_entered"));
	}
}

void SpxSprite::set_gid(GdObj id) {
	this->gid = id;
}

GdObj SpxSprite::get_gid() {
	return gid;
}

void SpxSprite::set_spx_type_name(String type_name) {
	spx_type_name = type_name;
}

String SpxSprite::get_spx_type_name() {
	return spx_type_name;
}

void SpxSprite::on_area_entered(Node *node) {
	if (!Spx::initialed) {
		return;
	}
	Node *parent_node = node->get_parent();
	const SpxSprite *other = Object::cast_to<SpxSprite>(parent_node);
	if (other != nullptr) {
		SPX_CALLBACK->func_on_trigger_enter(this->gid, other->gid);
	}
}

void SpxSprite::on_area_exited(Node *node) {
	if (!Spx::initialed) {
		return;
	}
	Node *parent_node = node->get_parent();
	const SpxSprite *other = Object::cast_to<SpxSprite>(parent_node);
	if (other != nullptr) {
		SPX_CALLBACK->func_on_trigger_exit(this->gid, other->gid);
	}
}

void SpxSprite::on_sprite_frames_set_changed() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_frames_set_changed(this->gid);
}

void SpxSprite::on_sprite_animation_changed() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_animation_changed(this->gid);
}

void SpxSprite::on_sprite_frame_changed() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_frame_changed(this->gid);
}

void SpxSprite::on_sprite_animation_looped() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_animation_looped(this->gid);
}

void SpxSprite::on_sprite_animation_finished() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_animation_finished(this->gid);
}

void SpxSprite::on_sprite_vfx_finished() {
	if (!Spx::initialed) {
		return;
	}
}

void SpxSprite::on_sprite_screen_exited() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_screen_exited(this->gid);
}

void SpxSprite::on_sprite_screen_entered() {
	if (!Spx::initialed) {
		return;
	}
	SPX_CALLBACK->func_on_sprite_screen_entered(this->gid);
}

void SpxSprite::set_color(GdColor color) {
	anim2d->set_self_modulate(color);
}

GdColor SpxSprite::get_color() {
	return anim2d->get_self_modulate();
}
void SpxSprite::set_texture_altas(GdString path, GdRect2 rect2) {
	auto path_str = SpxStr(path);
	Ref<Texture2D> texture = ResourceLoader::load(path_str);

	Ref<AtlasTexture> atlas_texture_frame = memnew(AtlasTexture);
	atlas_texture_frame->set_atlas(texture);
	atlas_texture_frame->set_region(rect2);

	if (texture.is_valid()) {
		auto frames = anim2d->get_sprite_frames();
		if (frames->get_frame_count(SpxSpriteMgr::default_texture_anim) == 0) {
			frames->add_frame(SpxSpriteMgr::default_texture_anim, atlas_texture_frame);
		} else {
			frames->set_frame(SpxSpriteMgr::default_texture_anim, 0, atlas_texture_frame);
		}
		anim2d->set_animation(SpxSpriteMgr::default_texture_anim);
	} else {
		print_error("can not find a texture: " + path_str);
	}
}

void SpxSprite::set_texture(GdString path) {
	auto path_str = SpxStr(path);
	Ref<Texture2D> texture = ResourceLoader::load(path_str);
	if (texture.is_valid()) {
		auto frames = anim2d->get_sprite_frames();
		if (frames->get_frame_count(SpxSpriteMgr::default_texture_anim) == 0) {
			frames->add_frame(SpxSpriteMgr::default_texture_anim, texture);
		} else {
			frames->set_frame(SpxSpriteMgr::default_texture_anim, 0, texture);
		}
		anim2d->set_animation(SpxSpriteMgr::default_texture_anim);
	} else {
		print_error("can not find a texture: " + path_str);
	}
}

GdString SpxSprite::get_texture() {
	auto tex = anim2d->get_sprite_frames()->get_frame_texture(SpxSpriteMgr::default_texture_anim, 0);
	if (tex == nullptr)
		return nullptr;
	SpxBaseMgr::temp_return_str = tex->get_name();
	return &SpxBaseMgr::temp_return_str;
}

void SpxSprite::play_anim(GdString p_name, GdFloat p_custom_scale, GdBool p_from_end) {
	anim2d->play(SpxStrName(p_name), p_custom_scale, p_from_end);
}

void SpxSprite::play_backwards_anim(GdString p_name) {
	anim2d->play_backwards(SpxStrName(p_name));
}

void SpxSprite::pause_anim() {
	anim2d->pause();
}

void SpxSprite::stop_anim() {
	anim2d->stop();
}

GdBool SpxSprite::is_playing_anim() const {
	return anim2d->is_playing();
}

void SpxSprite::set_anim(GdString p_name) {
	anim2d->set_animation(SpxStrName(p_name));
}

GdString SpxSprite::get_anim() const {
	auto name = anim2d->get_animation();
	return name;
}

void SpxSprite::set_anim_frame(GdInt p_frame) {
	anim2d->set_frame(p_frame);
}

GdInt SpxSprite::get_anim_frame() const {
	return anim2d->get_frame();
}

void SpxSprite::set_anim_speed_scale(GdFloat p_speed_scale) {
	anim2d->set_speed_scale(p_speed_scale);
}

GdFloat SpxSprite::get_anim_speed_scale() const {
	return anim2d->get_speed_scale();
}

GdFloat SpxSprite::get_anim_playing_speed() const {
	return anim2d->get_playing_speed();
}

void SpxSprite::set_anim_centered(GdBool p_center) {
	anim2d->set_centered(p_center);
}

GdBool SpxSprite::is_anim_centered() const {
	return anim2d->is_centered();
}

void SpxSprite::set_anim_offset(GdVec2 p_offset) {
	anim2d->set_offset(p_offset);
}

GdVec2 SpxSprite::get_anim_offset() const {
	return anim2d->get_offset();
}

void SpxSprite::set_anim_flip_h(GdBool p_flip) {
	anim2d->set_flip_h(p_flip);
}

GdBool SpxSprite::is_anim_flipped_h() const {
	return anim2d->is_flipped_h();
}

void SpxSprite::set_anim_flip_v(GdBool p_flip) {
	anim2d->set_flip_v(p_flip);
}

GdBool SpxSprite::is_anim_flipped_v() const {
	return anim2d->is_flipped_v();
}

void SpxSprite::set_gravity(GdFloat gravity) {
	// TODO
}

GdFloat SpxSprite::get_gravity() {
	return 0;
}

void SpxSprite::set_mass(GdFloat mass) {
}

GdFloat SpxSprite::get_mass() {
	return 0;
}

void SpxSprite::add_force(GdVec2 force) {
	return;
}

void SpxSprite::add_impulse(GdVec2 impulse) {
	return;
}

void SpxSprite::set_trigger_layer(GdInt layer) {
	area2d->set_collision_layer((uint32_t)layer);
}

GdInt SpxSprite::get_trigger_layer() {
	return area2d->get_collision_layer();
}

void SpxSprite::set_trigger_mask(GdInt mask) {
	area2d->set_collision_mask((uint32_t)mask);
}

GdInt SpxSprite::get_trigger_mask() {
	return area2d->get_collision_mask();
}

void SpxSprite::set_collider_rect(GdVec2 center, GdVec2 size) {
	Ref<RectangleShape2D> rect = memnew(RectangleShape2D);
	rect->set_size(size);
	collider2d->set_shape(rect);
	collider2d->set_position(center);
}

void SpxSprite::set_collider_circle(GdVec2 center, GdFloat radius) {
	Ref<CircleShape2D> circle = memnew(CircleShape2D);
	circle->set_radius(radius);
	collider2d->set_shape(circle);
	collider2d->set_position(center);
}

void SpxSprite::set_collider_capsule(GdVec2 center, GdVec2 size) {
	Ref<CapsuleShape2D> capsule = memnew(CapsuleShape2D);
	capsule->set_radius(size.x / 2);
	capsule->set_height(size.y);
	collider2d->set_shape(capsule);
	collider2d->set_position(center);
}

void SpxSprite::set_collision_enabled(GdBool enabled) {
	collider2d->set_visible(enabled);
}

GdBool SpxSprite::is_collision_enabled() {
	return collider2d->is_visible();
}

void SpxSprite::set_trigger_capsule(GdVec2 center, GdVec2 size) {
	Ref<CapsuleShape2D> capsule = memnew(CapsuleShape2D);
	capsule->set_radius(size.x / 2);
	capsule->set_height(size.y);
	trigger2d->set_shape(capsule);
	trigger2d->set_position(center);
}

void SpxSprite::set_trigger_rect(GdVec2 center, GdVec2 size) {
	Ref<RectangleShape2D> rect = memnew(RectangleShape2D);
	rect->set_size(size);
	trigger2d->set_shape(rect);
	trigger2d->set_position(center);
}

void SpxSprite::set_trigger_circle(GdVec2 center, GdFloat radius) {
	Ref<CircleShape2D> circle = memnew(CircleShape2D);
	circle->set_radius(radius);
	trigger2d->set_shape(circle);
	trigger2d->set_position(center);
}

void SpxSprite::set_trigger_enabled(GdBool trigger) {
	area2d->set_visible(trigger);
}

GdBool SpxSprite::is_trigger_enabled() {
	return area2d->is_visible();
}

CollisionShape2D *SpxSprite::get_collider(bool is_trigger) {
	return is_trigger ? trigger2d : collider2d;
}

GdBool SpxSprite::check_collision(SpxSprite *other, GdBool is_src_trigger, GdBool is_dst_trigger) {
	if (other == nullptr)
		return false;
	auto this_shape = is_src_trigger ? this->trigger2d : this->collider2d;
	auto other_shape = is_dst_trigger ? other->trigger2d : other->collider2d;
	if (!this_shape->get_shape().is_valid()) {
		return false;
	}
	if (!other_shape->get_shape().is_valid()) {
		return false;
	}
	return this_shape->get_shape()->collide(this_shape->get_global_transform(), other_shape->get_shape(), other_shape->get_global_transform());
}

GdBool SpxSprite::check_collision_with_point(GdVec2 point, GdBool is_trigger) {
	auto pos = this->get_position();
	auto this_shape = is_trigger ? this->trigger2d : this->collider2d;
	if (!this_shape->get_shape().is_valid()) {
		return false;
	}

	Ref<CircleShape2D> point_shape;
	point_shape.instantiate();
	point_shape->set_radius(3);

	Transform2D point_transform(0, point);
	Transform2D sprite_transform = get_global_transform();
	bool is_colliding = this_shape->get_shape()->collide(sprite_transform, point_shape, point_transform);
	return is_colliding;
}
void SpxSprite::set_render_scale(GdVec2 scale) {
	anim2d->set_scale(scale);
}
GdVec2 SpxSprite::get_render_scale() {
	return anim2d->get_scale();
}
