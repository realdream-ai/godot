/**************************************************************************/
/*  spx_ui_mgr.cpp                                                        */
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

#include "spx_ui_mgr.h"

// ui
GdInt SpxUIMgr::create_button(GdString path, GdRect2 rect, GdString text) {
	return 0;
}

GdInt SpxUIMgr::create_label(GdString path, GdRect2 rect, GdString text) {
	return 0;
}

GdInt SpxUIMgr::create_image(GdString path, GdRect2 rect, GdColor color) {
	return 0;
}

GdInt SpxUIMgr::create_slider(GdString path, GdRect2 rect, GdFloat value) {
	return 0;
}

GdInt SpxUIMgr::create_toggle(GdString path, GdRect2 rect, GdBool value) {
	return 0;
}

GdInt SpxUIMgr::create_input(GdString path, GdRect2 rect, GdString text) {
	return 0;
}

GdInt SpxUIMgr::get_type(GdObj id) {
	return 0;
}

void SpxUIMgr::set_interactable(GdObj id, GdBool interactable) {
	print_line(vformat("Updating interactable of %d to %s", id, interactable ? "true" : "false"));
}

GdBool SpxUIMgr::get_interactable(GdObj id) {
	return false;
}

void SpxUIMgr::set_text(GdObj id, GdString text) {
}

GdString SpxUIMgr::get_text(GdObj id) {
	return nullptr;
}

void SpxUIMgr::set_rect(GdObj id, GdRect2 rect) {
	print_line(vformat("Updating rect of %d to %f, %f, %f, %f", id, rect.position.x, rect.position.y, rect.size.x, rect.size.y));
}

GdRect2 SpxUIMgr::get_rect(GdObj id) {
	return GdRect2();
}

void SpxUIMgr::set_color(GdObj id, GdColor color) {
	print_line(vformat("Updating GdColor of %d to %f, %f, %f, %f", id, color.r, color.g, color.b, color.a));
}

GdColor SpxUIMgr::get_color(GdObj id) {
	return GdColor();
}

void SpxUIMgr::set_font_size(GdObj id, GdFloat size) {
	print_line(vformat("Updating font size of %d to %f", id, size));
}

GdFloat SpxUIMgr::get_font_size(GdObj id) {
	return 0;
}

void SpxUIMgr::set_visible(GdObj id, GdBool visible) {
	print_line(vformat("Updating visibility of %d to %s", id, visible ? "true" : "false"));
}

GdBool SpxUIMgr::get_visible(GdObj id) {
	return false;
}
