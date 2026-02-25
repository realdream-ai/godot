/**************************************************************************/
/*  spx_pen_mgr.cpp                                                       */
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

#include "spx_pen_mgr.h"

void SpxPenMgr::on_awake() {
	SpxBaseMgr::on_awake();
	_create_root("pen_root");
}

void SpxPenMgr::on_update(float delta) {
	SpxBaseMgr::on_update(delta);
	_update_all(delta);
}

void SpxPenMgr::on_destroy() {
	_destroy_all();
	SpxBaseMgr::on_destroy();
}

void SpxPenMgr::on_reset(int reset_code) {
	_reset_all(reset_code);
}

GdObj SpxPenMgr::create_pen() {
	return _create_object();
}

void SpxPenMgr::destroy_pen(GdObj obj) {
	destroy_object(obj);
}

void SpxPenMgr::destroy_all_pens() {
	rw_lock.read_lock();
	for (const auto &[id, pen] : id_objects) {
		pen->erase_all();
	}
	rw_lock.read_unlock();
}

void SpxPenMgr::move_pen_to(GdObj obj, GdVec2 position) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->move_to(position);
}

void SpxPenMgr::pen_stamp(GdObj obj) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->stamp();
}

void SpxPenMgr::pen_down(GdObj obj, GdBool move_by_mouse) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->on_down(move_by_mouse);
}

void SpxPenMgr::pen_up(GdObj obj) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->on_up();
}

void SpxPenMgr::set_pen_color_to(GdObj obj, GdColor color) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->set_color_to(color);
}

void SpxPenMgr::change_pen_by(GdObj obj, GdInt property, GdFloat amount) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->change_by(property, amount);
}

void SpxPenMgr::set_pen_to(GdObj obj, GdInt property, GdFloat value) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->set_to(property, value);
}

void SpxPenMgr::change_pen_size_by(GdObj obj, GdFloat amount) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->change_size_by(amount);
}

void SpxPenMgr::set_pen_size_to(GdObj obj, GdFloat size) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->set_size_to(size);
}

void SpxPenMgr::set_pen_stamp_texture(GdObj obj, GdString texture_path) {
	SPX_CHECK_AND_GET_OBJECT_V(pen, get_object(obj), SpxPen)
	pen->set_stamp_texture(texture_path);
}
