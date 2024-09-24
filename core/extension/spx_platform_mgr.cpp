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

void SpxPlatformMgr::set_window_size(GdInt width, GdInt height) {
	get_tree()->set_window_size(width, height);
}

GdVec2 SpxPlatformMgr::get_window_size() {
	return get_tree()->get_window_size();
}

void SpxPlatformMgr::set_window_title(GdString title) {
	get_tree()->set_window_title(SpxStr(title));
}

GdString SpxPlatformMgr::get_window_title() {
	SpxBaseMgr::temp_return_str = String("Godot");
	return &SpxBaseMgr::temp_return_str;
}
void SpxPlatformMgr::set_window_fullscreen(GdBool enable) {
	get_tree()->set_window_fullscreen(enable);
}

void SpxPlatformMgr::is_window_fullscreen() {
	return get_tree()->is_window_fullscreen();
}

