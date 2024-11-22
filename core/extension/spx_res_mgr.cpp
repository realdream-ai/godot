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

#include "spx_res_mgr.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "editor/import/resource_importer_wav.h"
#include "modules/minimp3/audio_stream_mp3.h"
#include "modules/minimp3/resource_importer_mp3.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/main/window.h"
#include "scene/resources/audio_stream_wav.h"
#include "scene/resources/image_texture.h"
#include "spx.h"

void SpxResMgr::on_awake() {
	SpxBaseMgr::on_awake();
	is_load_direct = true;
}

Ref<AudioStreamWAV> SpxResMgr::_load_wav(const String &path) {
	Ref<ResourceImporterWAV> importer = memnew(ResourceImporterWAV);
	List<ResourceImporter::ImportOption> options_list;
	importer->get_import_options("", &options_list);
	HashMap<StringName, Variant> options_map;
	for (const ResourceImporter::ImportOption &E : options_list) {
		options_map[E.option.name] = E.default_value;
	}
	Ref<AudioStreamWAV> sample;
	importer->import_asset(sample, path, options_map, nullptr);
	return sample;
}

Ref<AudioStreamMP3> SpxResMgr::_load_mp3(const String &path) {
	return ResourceImporterMP3::import_mp3(path);
}

Ref<AudioStream> SpxResMgr::_load_audio_direct(const String &path) {
	if (cached_audio.has(path)) {
		return cached_audio[path];
	}
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	if (file.is_null()) {
		print_line("Failed to open audio file: " + path);
		return Ref<AudioStreamWAV>();
	}
	Ref<AudioStream> res;
	const String ext = path.get_extension().to_lower();
	if (ext == "mp3") {
		res = _load_mp3(path);
	} else if (ext == "wav") {
		res = _load_wav(path);
	} else {
		print_error("unknown audio extension " + ext + " path=" + path);
	}
	cached_audio.insert(path, res);
	return res;
}

Ref<Texture2D> SpxResMgr::_load_texture_direct(const String &path) {
	if (cached_texture.has(path)) {
		return cached_texture[path];
	}

	Ref<Image> image;
	image.instantiate();
	Error err = image->load(path);
	if (err != OK) {
		print_line("Failed to load image: " + String::num_int64(err));
		return Ref<Texture2D>();
	}

	Ref<ImageTexture> texture;
	texture.instantiate();
	texture->create_from_image(image);
	cached_texture.insert(path, texture);
	return texture;
}

Ref<Texture2D> SpxResMgr::load_texture(String path) {
	if (!is_load_direct) {
		Ref<Resource> res = ResourceLoader::load(path);
		if (res.is_null()) {
			print_line("load texture failed !", path);
			return Ref<Texture2D>();
		}
		return res;
	} else {
		return _load_texture_direct(path);
	}
}

Ref<AudioStream> SpxResMgr::load_audio(String path) {
	if (!is_load_direct) {
		Ref<Resource> res = ResourceLoader::load(path);
		if (res.is_null()) {
			print_line("load audio failed !", path);
			return Ref<AudioStream>();
		}
		return res;
	} else {
		return _load_audio_direct(path);
	}
}

void SpxResMgr::set_load_mode(GdBool is_direct_mode) {
	is_load_direct = is_direct_mode;
}

GdRect2 SpxResMgr::get_bound_from_alpha(GdString path) {
	auto path_str = SpxStr(path);

	Ref<Texture2D> image = ResourceLoader::load(path_str);

	int width = image->get_width();
	int height = image->get_height();

	int min_x = width;
	int min_y = height;
	int max_x = 0;
	int max_y = 0;
	bool has_alpha = false;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (image->is_pixel_opaque(x, y)) { // Check if the pixel is not fully transparent
				has_alpha = true;
				if (x < min_x)
					min_x = x;
				if (y < min_y)
					min_y = y;
				if (x > max_x)
					max_x = x;
				if (y > max_y)
					max_y = y;
			}
		}
	}
	if (!has_alpha) {
		return Rect2();
	}

	return Rect2(Vector2(min_x, min_y), Vector2(max_x - min_x + 1, max_y - min_y + 1));
}

GdVec2 SpxResMgr::get_image_size(GdString path) {
	auto path_str = SpxStr(path);
	Ref<Texture2D> value = ResourceLoader::load(path_str);
	if (value.is_valid()) {
		return value->get_size();
	} else {
		print_error("can not find a texture: " + path_str);
	}
	return GdVec2(1, 1);
}

GdString SpxResMgr::read_all_text(GdString p_path) {
	auto path = SpxStr(p_path);
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	SpxBaseMgr::temp_return_str = "";
	if (file.is_null()) {
		print_line("Unable to open file.", path);
	} else {
		String file_content;
		while (!file->eof_reached()) {
			String line = file->get_line();
			file_content += line + "\n";
		}
		SpxBaseMgr::temp_return_str = file_content.utf8();
	}
	file->close();
	return &SpxBaseMgr::temp_return_str;
}

GdBool SpxResMgr::has_file(GdString p_path) {
	auto path = SpxStr(p_path);
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	return !file.is_null();
}
