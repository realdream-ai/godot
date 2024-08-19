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
#include "scene/2d/sprite_2d.h"
class AnimatedSprite2D;
class Area2D;
class CollisionShape2D;

class SpxSprite : public Sprite2D {
	GDCLASS(SpxSprite, Node2D);

private:
	GdObj obj_id;

	template <typename T>
	T *get_component(Node *node, GdBool recursive = false);
	Node *get_component(Node *node, StringName name, GdBool recursive);

protected:
	void _notification(int p_what);

public:
	AnimatedSprite2D *animation;
	Area2D *area;
	CollisionShape2D *collider;

	template <typename T>
	T *get_component(GdBool recursive = false);
	template <typename T>
	T *get_component(StringName name, GdBool recursive);

public:
	static void _bind_methods();
	void set_id(GdObj id);
	GdObj get_id();
	void on_destroy_call();
	SpxSprite();
	~SpxSprite() override;
	void on_start();
	void on_area_entered(Node *node);
	void on_area_exited(Node *node);

	// render
	void set_color(GdColor color);
	GdColor get_color();
	void set_texture(GdString path);
	GdString get_texture();

	// animation
	void play_anim(const StringName &p_name = StringName(), GdFloat p_custom_scale = 1.0, GdBool p_from_end = false);
	void play_backwards_anim(const StringName &p_name = StringName());
	void pause_anim();
	void stop_anim();
	GdBool is_playing_anim() const;
	void set_anim(const StringName &p_name);
	StringName get_anim() const;
	void set_anim_frame(GdInt p_frame);
	GdInt get_anim_frame() const;

	void set_anim_speed_scale(GdFloat p_speed_scale);
	GdFloat get_anim_speed_scale() const;
	GdFloat get_anim_playing_speed() const;

	void set_anim_centered(GdBool p_center);
	GdBool is_anim_centered() const;

	void set_anim_offset(const GdVec2 &p_offset);
	GdVec2 get_anim_offset() const;

	void set_anim_flip_h(GdBool p_flip);
	GdBool is_anim_flipped_h() const;

	void set_anim_flip_v(GdBool p_flip);
	GdBool is_anim_flipped_v() const;
};

template <typename T> T *SpxSprite::get_component(Node *node, GdBool recursive) {
	for (GdInt i = 0; i < node->get_child_count(); ++i) {
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

template <typename T> T *SpxSprite::get_component(GdBool recursive) {
	return get_component<T>(this, recursive);
}

template <typename T>
T *SpxSprite::get_component(StringName name, GdBool recursive) {
	Node *node = get_component(this, name, recursive);
	return Object::cast_to<T>(node);
}
#endif // SPX_SPRITE_H
