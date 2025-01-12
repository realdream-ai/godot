/**************************************************************************/
/*  spx_sprite.h                                                          */
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

#ifndef SPX_SPRITE_H
#define SPX_SPRITE_H

#include "gdextension_spx_ext.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/physics_body_2d.h"
#include "scene/2d/sprite_2d.h"
#include "spx.h"

class SpriteFrames;
class AnimatedSprite2D;
class Area2D;
class CollisionShape2D;
class VisibleOnScreenNotifier2D;
class SpxSprite : public CharacterBody2D {
	GDCLASS(SpxSprite, CharacterBody2D);

private:
	GdObj gid;

	template <typename T>
	T *get_component(Node *node, GdBool recursive = false);
	Node *get_component(Node *node, StringName name, GdBool recursive);

	bool use_default_frames;
	void set_use_default_frames(bool is_on);
	bool get_use_default_frames();

protected:
	void _notification(int p_what);
	void _draw();
	Ref<SpriteFrames> default_sprite_frames;
	AnimatedSprite2D *anim2d;
	Area2D *area2d;
	CollisionShape2D *trigger2d;
	CollisionShape2D *collider2d;
	VisibleOnScreenNotifier2D *visible_notifier;

public:
	CollisionShape2D *get_trigger() { return trigger2d; }

public:
	template <typename T>
	T *get_component(GdBool recursive = false);
	template <typename T>
	T *get_component(StringName name, GdBool recursive);

public:
	String spx_type_name;

public:
	static void _bind_methods();
	void on_destroy_call();
	SpxSprite();
	~SpxSprite() override;
	void on_start();
	void on_area_entered(Node *node);
	void on_area_exited(Node *node);

	// animation events
	void on_sprite_frames_set_changed();
	void on_sprite_animation_changed();
	void on_sprite_frame_changed();
	void on_sprite_animation_looped();
	void on_sprite_animation_finished();
	// vfx
	void on_sprite_vfx_finished();
	// visibility
	void on_sprite_screen_exited();
	void on_sprite_screen_entered();

	void set_spx_type_name(String type_name);
	String get_spx_type_name();

public:
	void set_gid(GdObj id);
	GdObj get_gid();

	void set_type_name(GdString type_name);
	// render
	void set_color(GdColor color);
	GdColor get_color();
	void set_texture_altas(GdString path, GdRect2 rect2);
	void set_texture(GdString path);

	GdString get_texture();

	// animation
	void play_anim(GdString p_name, GdFloat p_speed = 1.0, GdBool isLoop = false, GdBool p_from_end = false);
	void play_backwards_anim(GdString p_name);
	void pause_anim();
	void stop_anim();
	GdBool is_playing_anim() const;
	void set_anim(GdString p_name);
	GdString get_anim() const;
	void set_anim_frame(GdInt p_frame);
	GdInt get_anim_frame() const;
	void set_anim_speed_scale(GdFloat p_speed_scale);
	GdFloat get_anim_speed_scale() const;
	GdFloat get_anim_playing_speed() const;
	void set_anim_centered(GdBool p_center);
	GdBool is_anim_centered() const;
	void set_anim_offset(GdVec2 p_offset);
	GdVec2 get_anim_offset() const;
	void set_anim_flip_h(GdBool p_flip);
	GdBool is_anim_flipped_h() const;
	void set_anim_flip_v(GdBool p_flip);
	GdBool is_anim_flipped_v() const;

	// physics
	void set_gravity(GdFloat gravity);
	GdFloat get_gravity();
	void set_mass(GdFloat mass);
	GdFloat get_mass();
	void add_force(GdVec2 force);
	void add_impulse(GdVec2 impulse);

	void set_trigger_layer(GdInt layer);
	GdInt get_trigger_layer();
	void set_trigger_mask(GdInt mask);
	GdInt get_trigger_mask();

	void set_collider_rect(GdVec2 center, GdVec2 size);
	void set_collider_circle(GdVec2 center, GdFloat radius);
	void set_collider_capsule(GdVec2 center, GdVec2 size);
	void set_collision_enabled(GdBool enabled);
	GdBool is_collision_enabled();

	void set_trigger_rect(GdVec2 center, GdVec2 size);
	void set_trigger_circle(GdVec2 center, GdFloat radius);
	void set_trigger_capsule(GdVec2 center, GdVec2 size);
	void set_trigger_enabled(GdBool trigger);
	GdBool is_trigger_enabled();

	// collision
	CollisionShape2D *get_collider(bool is_trigger = false);
	GdBool check_collision(SpxSprite *other, GdBool is_src_trigger = true, GdBool is_dst_trigger = true);
	GdBool check_collision_with_point(GdVec2 point, GdBool is_trigger = true);

	void set_render_scale(GdVec2 scale);
	GdVec2 get_render_scale();
};

template <typename T>
T *SpxSprite::get_component(Node *node, GdBool recursive) {
	for (int i = 0; i < node->get_child_count(); ++i) {
		Node *child = node->get_child(i);
		T *component = Object::cast_to<T>(child);
		if (component != nullptr) {
			return component;
		}
		if (recursive) {
			component = get_component<T>(child, true);
			if (component != nullptr) {
				return component;
			}
		}
	}
	return nullptr;
}

template <typename T>
T *SpxSprite::get_component(GdBool recursive) {
	return get_component<T>(this, recursive);
}

template <typename T>
T *SpxSprite::get_component(StringName name, GdBool recursive) {
	Node *node = get_component(this, name, recursive);
	return Object::cast_to<T>(node);
}
#endif // SPX_SPRITE_H
