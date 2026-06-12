/**************************************************************************/
/*  spx_pen.cpp                                                           */
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

#include "spx_pen.h"

#include "scene/2d/line_2d.h"
#include "scene/2d/sprite_2d.h"

#include "spx_engine.h"
#include "spx_res_mgr.h"

const int PEN_PROPERTY_SATURATION = 0;
const int PEN_PROPERTY_BRIGHTNESS = 1;
const int PEN_PROPERTY_TRANSPARENCY = 2;

GdObj SpxPen::get_id() {
	return id;
}

void SpxPen::on_create(GdInt id, Node *root) {
	manager_root = root;
	this->id = id;
	pen_root = memnew(Node2D);
	pen_root->set_name("pen_" + itos(id));
	manager_root->add_child(pen_root);
	current_line = _create_new_line();
	is_pen_down = false;
	min_draw_distance = 1.0f;
	pen_properties.transparency = 1.0f;
	stamp_texture_path = String();
}

void SpxPen::_destroy_pen_root() {
	if (pen_root != nullptr) {
		pen_root->queue_free();
		pen_root = nullptr;
	}
	manager_root = nullptr;
	current_line = nullptr;
	is_pen_down = false;
	move_by_mouse = false;
}

Line2D *SpxPen::_create_new_line() {
	if (pen_root == nullptr) {
		return nullptr;
	}
	Line2D *new_line = memnew(Line2D);
	new_line->set_width(pen_properties.size);
	new_line->set_default_color(_get_current_color());
	pen_root->add_child(new_line);
	return new_line;
}

void SpxPen::_start_new_line() {
	if (is_pen_down) {
		current_line = _create_new_line();
		if (current_line != nullptr) {
			current_line->add_point(current_pen_pos);
		}
	}
}

void SpxPen::_append_current_point_if_needed(GdVec2 position) {
	current_pen_pos = position;
	if (!is_pen_down || current_line == nullptr) {
		return;
	}

	if (current_line->get_point_count() > 0) {
		Vector2 last_point = current_line->get_point_position(current_line->get_point_count() - 1);
		float distance = last_point.distance_to(current_pen_pos);
		if (distance >= min_draw_distance) {
			current_line->add_point(current_pen_pos);
		}
	} else {
		current_line->add_point(current_pen_pos);
	}
}

Color SpxPen::_get_current_color() const {
	Color final_color = pen_properties.color;
	// Apply saturation and brightness
	float h = final_color.get_h();
	float s = final_color.get_s();
	float v = final_color.get_v();
	s *= pen_properties.saturation;
	v *= pen_properties.brightness;
	final_color.set_hsv(h, s, v, pen_properties.transparency);
	return final_color;
}

void SpxPen::on_update(float delta) {
	if (move_by_mouse) {
		_append_current_point_if_needed(Input::get_singleton()->get_mouse_position());
	}
}

void SpxPen::on_reset(int reset_code) {
	_destroy_pen_root();
}

void SpxPen::on_destroy() {
	_destroy_pen_root();
}

void SpxPen::erase_all() {
	if (pen_root == nullptr) {
		return;
	}

	const bool was_pen_down = is_pen_down;

	TypedArray<Node> children = pen_root->get_children();
	for (int i = 0; i < children.size(); i++) {
		Node *child = Object::cast_to<Node>(children[i]);
		if (Object::cast_to<Line2D>(child)) {
			child->queue_free();
		}
		if (Object::cast_to<Sprite2D>(child)) {
			child->queue_free();
		}
	}
	current_line = _create_new_line();
	is_pen_down = was_pen_down;
	if (is_pen_down && current_line != nullptr) {
		current_line->add_point(current_pen_pos);
	}
}

void SpxPen::_stamp_texture(const Ref<Texture2D> &texture, GdVec2 position, GdFloat rotation_radians, GdVec2 scale) {
	if (!texture.is_valid()) {
		return;
	}
	if (pen_root == nullptr) {
		return;
	}
	Sprite2D *new_stamp = memnew(Sprite2D);
	new_stamp->set_texture(texture);
	new_stamp->set_position(position);
	new_stamp->set_rotation(rotation_radians);
	new_stamp->set_scale(scale);
	pen_root->add_child(new_stamp);
}

Ref<Texture2D> SpxPen::_resolve_stamp_texture(const String &texture_path) {
	if (stamp_texture.is_valid() && stamp_texture_path == texture_path) {
		return stamp_texture;
	}

	stamp_texture_path = texture_path;
	stamp_texture = resMgr->load_texture(texture_path, false);
	return stamp_texture;
}

void SpxPen::stamp() {
	_stamp_texture(stamp_texture, current_pen_pos, 0.0f, Vector2(1.0f, 1.0f));
}

void SpxPen::move_to(GdVec2 position) {
	_append_current_point_if_needed(position);
}

void SpxPen::on_down(GdBool p_move_by_mouse) {
	move_by_mouse = p_move_by_mouse;
	is_pen_down = true;
	_start_new_line();
}

void SpxPen::on_up() {
	is_pen_down = false;
}

void SpxPen::set_color_to(GdColor color) {
	pen_properties.color = color;
	pen_properties.transparency = color.a;
	_start_new_line();
}

void SpxPen::change_by(GdInt property, GdFloat amount) {
	if (property == PEN_PROPERTY_SATURATION) {
		pen_properties.saturation = CLAMP(pen_properties.saturation + amount, 0.0f, 1.0f);
	} else if (property == PEN_PROPERTY_BRIGHTNESS) {
		pen_properties.brightness = CLAMP(pen_properties.brightness + amount, 0.0f, 1.0f);
	} else if (property == PEN_PROPERTY_TRANSPARENCY) {
		pen_properties.transparency = CLAMP(pen_properties.transparency + amount, 0.0f, 1.0f);
	}
	_start_new_line();
}

void SpxPen::set_to(GdInt property, GdFloat value) {
	if (property == PEN_PROPERTY_SATURATION) {
		pen_properties.saturation = CLAMP(value, 0.0f, 1.0f);
	} else if (property == PEN_PROPERTY_BRIGHTNESS) {
		pen_properties.brightness = CLAMP(value, 0.0f, 1.0f);
	} else if (property == PEN_PROPERTY_TRANSPARENCY) {
		pen_properties.transparency = CLAMP(value, 0.0f, 1.0f);
	}
	_start_new_line();
}

void SpxPen::change_size_by(GdFloat amount) {
	pen_properties.size += amount;
	pen_properties.size = MAX(pen_properties.size, 1.0f);
	_start_new_line();
}

void SpxPen::set_size_to(GdFloat size) {
	pen_properties.size = MAX(size, 1.0f);
	_start_new_line();
}

void SpxPen::set_stamp_texture(GdString texture_path) {
	_resolve_stamp_texture(SpxStr(texture_path));
}

void SpxPen::stamp_with_transform(GdString texture_path, GdVec2 position, GdFloat rotation_radians, GdVec2 scale) {
	Ref<Texture2D> texture = _resolve_stamp_texture(SpxStr(texture_path));
	_stamp_texture(texture, position, rotation_radians, scale);
}
