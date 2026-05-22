/**************************************************************************/
/*  svg_utils.cpp                                                         */
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

#include "svg_utils.h"

#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "thirdparty/lunasvg/include/lunasvg.h"

#include <lunasvg.h>
#include <cstring>

namespace {

struct StoredSVGFontFace {
	String family;
	Vector<uint8_t> data;
};

Mutex svg_font_registry_mutex;
Vector<uint8_t> svg_default_font_data;
HashMap<String, Vector<uint8_t>> svg_named_font_faces;
uint64_t svg_font_registry_serial = 0;

static bool _copy_font_bytes(const void *font_data, int length, Vector<uint8_t> &r_bytes) {
	if (font_data == nullptr || length <= 0) {
		r_bytes.clear();
		return false;
	}

	r_bytes.resize(length);
	::memcpy(r_bytes.ptrw(), font_data, length);
	return true;
}

static void _register_font_bytes_for_current_thread(const String &family, const Vector<uint8_t> &font_data) {
	if (font_data.is_empty()) {
		return;
	}

	CharString utf8_family = family.utf8();
	const char *family_name = family.is_empty() ? "" : utf8_family.get_data();
	lunasvg_add_font_face_from_data(family_name, false, false, font_data.ptr(), font_data.size(), nullptr, nullptr);
}

} // namespace

void SVGUtils::set_default_font(const void *font_data, int length) {
	Vector<uint8_t> font_bytes;
	if (!_copy_font_bytes(font_data, length, font_bytes)) {
		return;
	}

	{
		MutexLock lock(svg_font_registry_mutex);
		svg_default_font_data = font_bytes;
		svg_font_registry_serial++;
	}

	ensure_font_faces_registered();
}

void SVGUtils::add_font_face(const String &family, const void *font_data, int length) {
	if (family.is_empty()) {
		return;
	}

	Vector<uint8_t> font_bytes;
	if (!_copy_font_bytes(font_data, length, font_bytes)) {
		return;
	}

	{
		MutexLock lock(svg_font_registry_mutex);
		svg_named_font_faces.insert(family, font_bytes);
		svg_font_registry_serial++;
	}

	ensure_font_faces_registered();
}

void SVGUtils::ensure_font_faces_registered() {
	thread_local uint64_t applied_serial = 0;

	Vector<uint8_t> default_font_data;
	Vector<StoredSVGFontFace> named_font_faces;
	uint64_t registry_serial = 0;
	{
		MutexLock lock(svg_font_registry_mutex);
		registry_serial = svg_font_registry_serial;
		if (registry_serial == 0 || registry_serial == applied_serial) {
			return;
		}

		default_font_data = svg_default_font_data;
		for (const KeyValue<String, Vector<uint8_t>> &E : svg_named_font_faces) {
			StoredSVGFontFace stored_font;
			stored_font.family = E.key;
			stored_font.data = E.value;
			named_font_faces.push_back(stored_font);
		}
	}

	_register_font_bytes_for_current_thread("", default_font_data);
	for (int i = 0; i < named_font_faces.size(); i++) {
		_register_font_bytes_for_current_thread(named_font_faces[i].family, named_font_faces[i].data);
	}
	applied_serial = registry_serial;
}
