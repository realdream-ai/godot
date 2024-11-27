/**************************************************************************/
/*  spx_platform_mgr.cpp                                                     */
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

#include "spx_platform_mgr.h"
#include "core/config/engine.h"
#include "scene/main/window.h"
#include "spx.h"

void SpxPlatformMgr::set_window_position(GdVec2 pos) {
	DisplayServer::get_singleton()->window_set_position(Size2i(pos.x, pos.y));
}
GdVec2 SpxPlatformMgr::get_window_position() {
	auto pos = DisplayServer::get_singleton()->window_get_position();
	return GdVec2(pos.x, pos.y);
}
void SpxPlatformMgr::set_window_size(GdInt width, GdInt height) {
	DisplayServer::get_singleton()->window_set_size(Size2i(width, height));
}

GdVec2 SpxPlatformMgr::get_window_size() {
	auto size = DisplayServer::get_singleton()->window_get_size();
	return GdVec2(size.x, size.y);
}

void SpxPlatformMgr::set_window_title(GdString title) {
	DisplayServer::get_singleton()->window_set_title(SpxStr(title));
}

GdString SpxPlatformMgr::get_window_title() {
	auto title = "";
	SpxBaseMgr::temp_return_str = title;
	return &SpxBaseMgr::temp_return_str;
}

void SpxPlatformMgr::set_window_fullscreen(GdBool enable) {
	print_line("SpxPlatformMgr::set_window_fullscreen", enable);
	auto mode = enable ? DisplayServer::WINDOW_MODE_FULLSCREEN : DisplayServer::WINDOW_MODE_WINDOWED;
	DisplayServer::get_singleton()->window_set_mode(mode);
}

GdBool SpxPlatformMgr::is_window_fullscreen() {
	return get_root()->get_mode() == Window::MODE_FULLSCREEN;
}

void SpxPlatformMgr::set_debug_mode(GdBool enable) {
	Spx::debug_mode = enable;
}

GdBool SpxPlatformMgr::is_debug_mode() {
	return Spx::debug_mode;
}

void SpxPlatformMgr::set_time_scale(GdFloat time_scale) {
	Engine::get_singleton()->set_time_scale(time_scale);
}

GdFloat SpxPlatformMgr::get_time_scale() {
	return Engine::get_singleton()->get_time_scale();
}
