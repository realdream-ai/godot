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

#include "scene/2d/sprite_2d.h"
#include "scene/main/viewport.h"

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
	_ensure_canvas();
	is_pen_down = false;
	has_last_draw_pos = false;
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
	canvas_sprite = nullptr;
	canvas_image.unref();
	canvas_texture.unref();
	canvas_size = Size2i();
	canvas_dirty = false;
	has_last_draw_pos = false;
	is_pen_down = false;
	move_by_mouse = false;
}

void SpxPen::_ensure_canvas() {
	if (pen_root == nullptr) {
		return;
	}
	if (canvas_sprite != nullptr && canvas_image.is_valid() && canvas_texture.is_valid()) {
		return;
	}

	Size2 size(480, 360);
	if (manager_root != nullptr && manager_root->get_viewport() != nullptr) {
		size = manager_root->get_viewport()->get_visible_rect().size;
	}
	canvas_size = Size2i(MAX(1, (int)Math::ceil(size.x)), MAX(1, (int)Math::ceil(size.y)));

	canvas_image = Image::create_empty(canvas_size.width, canvas_size.height, false, Image::FORMAT_RGBA8);
	canvas_image->fill(Color(0, 0, 0, 0));
	canvas_texture = ImageTexture::create_from_image(canvas_image);

	canvas_sprite = memnew(Sprite2D);
	canvas_sprite->set_name("pen_canvas");
	canvas_sprite->set_centered(true);
	canvas_sprite->set_texture(canvas_texture);
	pen_root->add_child(canvas_sprite);
	canvas_dirty = false;
}

void SpxPen::_clear_canvas() {
	_ensure_canvas();
	if (canvas_image.is_null()) {
		return;
	}
	canvas_image->fill(Color(0, 0, 0, 0));
	canvas_dirty = true;
	_flush_canvas();
}

void SpxPen::_flush_canvas() {
	if (!canvas_dirty || canvas_texture.is_null() || canvas_image.is_null()) {
		return;
	}
	canvas_texture->update(canvas_image);
	canvas_dirty = false;
}

Point2i SpxPen::_to_canvas_pixel(GdVec2 position) const {
	return Point2i(
			(int)Math::floor(position.x + (float)canvas_size.width * 0.5f),
			(int)Math::floor(position.y + (float)canvas_size.height * 0.5f));
}

Color SpxPen::_blend_pixel(Color src, Color dst) const {
	const float out_alpha = src.a + dst.a * (1.0f - src.a);
	if (out_alpha <= 0.0f) {
		return Color(0, 0, 0, 0);
	}
	return Color(
			(src.r * src.a + dst.r * dst.a * (1.0f - src.a)) / out_alpha,
			(src.g * src.a + dst.g * dst.a * (1.0f - src.a)) / out_alpha,
			(src.b * src.a + dst.b * dst.a * (1.0f - src.a)) / out_alpha,
			out_alpha);
}

void SpxPen::_draw_brush_at(GdVec2 position, float size, Color color) {
	_ensure_canvas();
	if (canvas_image.is_null()) {
		return;
	}

	const float radius = MAX(size * 0.5f, 0.5f);
	const float radius_sq = radius * radius;
	Point2i center = _to_canvas_pixel(position);
	const int min_x = MAX(0, center.x - (int)Math::ceil(radius));
	const int max_x = MIN(canvas_size.width - 1, center.x + (int)Math::ceil(radius));
	const int min_y = MAX(0, center.y - (int)Math::ceil(radius));
	const int max_y = MIN(canvas_size.height - 1, center.y + (int)Math::ceil(radius));

	for (int y = min_y; y <= max_y; y++) {
		for (int x = min_x; x <= max_x; x++) {
			const float dx = ((float)x + 0.5f) - (position.x + (float)canvas_size.width * 0.5f);
			const float dy = ((float)y + 0.5f) - (position.y + (float)canvas_size.height * 0.5f);
			if (dx * dx + dy * dy <= radius_sq) {
				canvas_image->set_pixel(x, y, _blend_pixel(color, canvas_image->get_pixel(x, y)));
			}
		}
	}
	canvas_dirty = true;
}

void SpxPen::_draw_line(GdVec2 from, GdVec2 to, float size, Color color) {
	const float distance = from.distance_to(to);
	if (distance <= 0.0f) {
		_draw_brush_at(to, size, color);
		return;
	}
	const int steps = MAX(1, (int)Math::ceil(distance / MAX(size * 0.25f, 0.25f)));
	for (int i = 0; i <= steps; i++) {
		const float t = (float)i / (float)steps;
		_draw_brush_at(from.lerp(to, t), size, color);
	}
}

GdVec2 SpxPen::_get_draw_position(GdVec2 position, float size) const {
	if (Math::is_equal_approx(size, 1.0f) || Math::is_equal_approx(size, 3.0f)) {
		return position + Vector2(0.5f, 0.5f);
	}
	return position;
}

void SpxPen::_draw_point(float size, Color color, GdVec2 position) {
	_draw_brush_at(_get_draw_position(position, size), size, color);
	_flush_canvas();
}

void SpxPen::_start_new_line() {
	if (is_pen_down) {
		has_last_draw_pos = true;
	}
}

void SpxPen::_append_current_point_if_needed(GdVec2 position) {
	current_pen_pos = position;
	if (!is_pen_down) {
		return;
	}
	const Vector2 draw_position = _get_draw_position(current_pen_pos, pen_properties.size);

	if (has_last_draw_pos) {
		float distance = last_draw_pos.distance_to(draw_position);
		if (distance < min_draw_distance) {
			return;
		}
		_draw_line(last_draw_pos, draw_position, pen_properties.size, _get_current_color());
	} else {
		_draw_brush_at(draw_position, pen_properties.size, _get_current_color());
	}
	last_draw_pos = draw_position;
	has_last_draw_pos = true;
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
	_flush_canvas();
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
		if (child != nullptr && child != canvas_sprite) {
			child->queue_free();
		}
	}
	_clear_canvas();
	is_pen_down = was_pen_down;
	has_last_draw_pos = is_pen_down;
	last_draw_pos = _get_draw_position(current_pen_pos, pen_properties.size);
}

void SpxPen::_stamp_texture(const Ref<Texture2D> &texture, GdVec2 position, GdFloat rotation_radians, GdVec2 scale) {
	if (!texture.is_valid()) {
		return;
	}
	if (pen_root == nullptr) {
		return;
	}
	_ensure_canvas();
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

void SpxPen::flush() {
	_flush_canvas();
}

void SpxPen::on_down(GdBool p_move_by_mouse) {
	move_by_mouse = p_move_by_mouse;
	_draw_point(pen_properties.size, _get_current_color(), current_pen_pos);
	last_draw_pos = _get_draw_position(current_pen_pos, pen_properties.size);
	has_last_draw_pos = true;
	is_pen_down = true;
}

void SpxPen::on_up() {
	is_pen_down = false;
	has_last_draw_pos = false;
	_flush_canvas();
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
