/**************************************************************************/
/*  spx_pen_surface.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including  */
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

#include "spx_pen_surface.h"

#include "servers/rendering_server.h"

void SpxPenCanvas::_draw_line_batch(int p_begin, int p_end) {
	constexpr int CAP_SEGMENTS = 12;
	Vector<int> indices;
	Vector<Point2> vertices;
	Vector<Color> colors;

	auto append_vertex = [&](const Vector2 &p_position, const Color &p_color) {
		vertices.push_back(p_position);
		colors.push_back(p_color);
	};
	auto append_disc = [&](const Vector2 &p_center, float p_radius, const Color &p_color) {
		const int center_index = vertices.size();
		append_vertex(p_center, p_color);
		for (int i = 0; i < CAP_SEGMENTS; i++) {
			const float angle = Math_TAU * (float)i / (float)CAP_SEGMENTS;
			append_vertex(p_center + Vector2(Math::cos(angle), Math::sin(angle)) * p_radius, p_color);
		}
		for (int i = 0; i < CAP_SEGMENTS; i++) {
			indices.push_back(center_index);
			indices.push_back(center_index + 1 + i);
			indices.push_back(center_index + 1 + ((i + 1) % CAP_SEGMENTS));
		}
	};

	for (int i = p_begin; i < p_end; i++) {
		const DrawCommand &command = pending_commands[i];
		const float radius = MAX(command.width * 0.5f, 0.5f);
		const Vector2 delta = command.to - command.from;
		if (delta.is_zero_approx()) {
			append_disc(command.from, radius, command.color);
			continue;
		}

		const Vector2 normal = Vector2(-delta.y, delta.x).normalized() * radius;
		const int quad_index = vertices.size();
		append_vertex(command.from + normal, command.color);
		append_vertex(command.from - normal, command.color);
		append_vertex(command.to + normal, command.color);
		append_vertex(command.to - normal, command.color);
		indices.push_back(quad_index);
		indices.push_back(quad_index + 1);
		indices.push_back(quad_index + 2);
		indices.push_back(quad_index + 2);
		indices.push_back(quad_index + 1);
		indices.push_back(quad_index + 3);

		// Scratch uses round pen caps. Each cap is deliberately low-poly: the
		// complete frame is sent as one colored triangle array, so segment
		// count affects vertices rather than draw-call count.
		append_disc(command.from, radius, command.color);
		append_disc(command.to, radius, command.color);
	}

	if (!indices.is_empty()) {
		RenderingServer::get_singleton()->canvas_item_add_triangle_array(get_canvas_item(), indices, vertices, colors);
	}
}

void SpxPenCanvas::_bind_methods() {
}

void SpxPenCanvas::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW) {
		return;
	}

	int line_batch_begin = -1;
	for (int i = 0; i < pending_commands.size(); i++) {
		const DrawCommand &command = pending_commands[i];
		if (command.type == DrawCommand::LINE) {
			if (line_batch_begin < 0) {
				line_batch_begin = i;
			}
			continue;
		}

		if (line_batch_begin >= 0) {
			_draw_line_batch(line_batch_begin, i);
			line_batch_begin = -1;
		}
		if (command.texture.is_valid()) {
			draw_set_transform(command.from, command.rotation, command.scale);
			draw_texture(command.texture, -command.texture->get_size() * 0.5f);
			draw_set_transform(Vector2(), 0.0f, Vector2(1.0f, 1.0f));
		}
	}
	if (line_batch_begin >= 0) {
		_draw_line_batch(line_batch_begin, pending_commands.size());
	}

	// CanvasItem has copied the commands into RenderingServer at this point.
	// The next redraw replaces those commands while the SubViewport keeps the
	// already-rasterized pixels.
	pending_commands.clear();
}

void SpxPenCanvas::add_line(const Vector2 &p_from, const Vector2 &p_to, float p_width, const Color &p_color) {
	DrawCommand command;
	command.type = DrawCommand::LINE;
	command.from = p_from;
	command.to = p_to;
	command.width = MAX(p_width, 1.0f);
	command.color = p_color;
	pending_commands.push_back(command);
}

void SpxPenCanvas::add_stamp(const Ref<Texture2D> &p_texture, const Vector2 &p_position, float p_rotation, const Vector2 &p_scale) {
	DrawCommand command;
	command.type = DrawCommand::STAMP;
	command.from = p_position;
	command.texture = p_texture;
	command.rotation = p_rotation;
	command.scale = p_scale;
	pending_commands.push_back(command);
}

void SpxPenCanvas::discard_pending() {
	pending_commands.clear();
}

void SpxPenSurface::_bind_methods() {
}

void SpxPenSurface::initialize(const Size2i &p_size) {
	ERR_FAIL_COND(render_target != nullptr);

	canvas_size = Size2i(MAX(1, p_size.x), MAX(1, p_size.y));

	render_target = memnew(SubViewport);
	render_target->set_name("pen_render_target");
	render_target->set_size(canvas_size);
	render_target->set_transparent_background(true);
#ifndef _3D_DISABLED
	render_target->set_disable_3d(true);
#endif
	render_target->set_handle_input_locally(false);
	render_target->set_clear_mode(SubViewport::CLEAR_MODE_ONCE);
	render_target->set_update_mode(SubViewport::UPDATE_ONCE);
	add_child(render_target);

	canvas = memnew(SpxPenCanvas);
	canvas->set_name("pen_canvas_drawer");
	render_target->add_child(canvas);

	canvas_sprite = memnew(Sprite2D);
	canvas_sprite->set_name("pen_canvas");
	canvas_sprite->set_centered(true);
	canvas_sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	canvas_sprite->set_texture(render_target->get_texture());
	add_child(canvas_sprite);
}

void SpxPenSurface::draw_line(const Vector2 &p_from, const Vector2 &p_to, float p_width, const Color &p_color) {
	ERR_FAIL_NULL(canvas);
	const Vector2 canvas_origin = Vector2(canvas_size) * 0.5f;
	canvas->add_line(p_from + canvas_origin, p_to + canvas_origin, p_width, p_color);
	dirty = true;
}

void SpxPenSurface::draw_stamp(const Ref<Texture2D> &p_texture, const Vector2 &p_position, float p_rotation, const Vector2 &p_scale) {
	ERR_FAIL_NULL(canvas);
	if (p_texture.is_null()) {
		return;
	}
	const Vector2 canvas_origin = Vector2(canvas_size) * 0.5f;
	canvas->add_stamp(p_texture, p_position + canvas_origin, p_rotation, p_scale);
	dirty = true;
}

void SpxPenSurface::clear() {
	if (canvas != nullptr) {
		canvas->discard_pending();
	}
	clear_requested = true;
	dirty = true;
}

void SpxPenSurface::flush() {
	if (!dirty || render_target == nullptr || canvas == nullptr) {
		return;
	}

	render_target->set_clear_mode(clear_requested ? SubViewport::CLEAR_MODE_ONCE : SubViewport::CLEAR_MODE_NEVER);
	canvas->queue_redraw();
	render_target->set_update_mode(SubViewport::UPDATE_ONCE);
	clear_requested = false;
	dirty = false;
}

Size2i SpxPenSurface::get_canvas_size() const {
	return canvas_size;
}

SpxPenSurface::~SpxPenSurface() {
	render_target = nullptr;
	canvas = nullptr;
	canvas_sprite = nullptr;
}
