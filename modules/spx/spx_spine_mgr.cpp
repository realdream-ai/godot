/**************************************************************************/
/*  spx_spine_mgr.cpp                                                     */
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

#include "spx_spine_mgr.h"
#include "spx_engine.h"
#include "spx_res_mgr.h"

void SpxSpineMgr::on_awake() {
	SpxBaseMgr::on_awake();
}

void SpxSpineMgr::on_reset(int reset_code) {
	clear_all_caches();
}

String SpxSpineMgr::_to_abs_path(const String &path) {
	return resMgr->_to_engine_path(path);
}

String SpxSpineMgr::_make_cache_key(const String &atlas_path, const String &skeleton_path) {
	return atlas_path + "|" + skeleton_path;
}

Ref<SpineAtlasResource> SpxSpineMgr::load_atlas(const String &path) {
	String abs_path = _to_abs_path(path);

	// Check cache
	if (cached_atlas.has(abs_path)) {
		return cached_atlas[abs_path];
	}

	// Load Atlas
	Ref<SpineAtlasResource> atlas_res;
	atlas_res.instantiate();
	Error err = atlas_res->load_from_atlas_file(abs_path);

	if (err != OK) {
		print_error(vformat("[SpxSpineMgr] Failed to load atlas: %s, error: %d", abs_path, err));
		return Ref<SpineAtlasResource>();
	}

	// Store in cache
	cached_atlas[abs_path] = atlas_res;
	print_line(vformat("[SpxSpineMgr] Atlas loaded and cached: %s", abs_path));

	return atlas_res;
}

Ref<SpineSkeletonFileResource> SpxSpineMgr::load_skeleton_file(const String &path) {
	String abs_path = _to_abs_path(path);

	// Check cache
	if (cached_skeleton_file.has(abs_path)) {
		return cached_skeleton_file[abs_path];
	}

	// Load Skeleton file
	Ref<SpineSkeletonFileResource> skeleton_file;
	skeleton_file.instantiate();
	Error err = skeleton_file->load_from_file(abs_path);

	if (err != OK) {
		print_error(vformat("[SpxSpineMgr] Failed to load skeleton file: %s, error: %d", abs_path, err));
		return Ref<SpineSkeletonFileResource>();
	}

	// Store in cache
	cached_skeleton_file[abs_path] = skeleton_file;
	print_line(vformat("[SpxSpineMgr] Skeleton file loaded and cached: %s", abs_path));

	return skeleton_file;
}

Ref<SpineSkeletonDataResource> SpxSpineMgr::load_spine_data(
		const String &atlas_path,
		const String &skeleton_path,
		float default_mix) {
	String abs_atlas = _to_abs_path(atlas_path);
	String abs_skeleton = _to_abs_path(skeleton_path);
	String cache_key = _make_cache_key(abs_atlas, abs_skeleton);

	// Check SkeletonData cache
	if (cached_skeleton_data.has(cache_key)) {
		return cached_skeleton_data[cache_key];
	}

	// Load Atlas (with cache)
	Ref<SpineAtlasResource> atlas_res = load_atlas(atlas_path);
	if (!atlas_res.is_valid()) {
		return Ref<SpineSkeletonDataResource>();
	}

	// Load Skeleton File (with cache)
	Ref<SpineSkeletonFileResource> skeleton_file = load_skeleton_file(skeleton_path);
	if (!skeleton_file.is_valid()) {
		return Ref<SpineSkeletonDataResource>();
	}

	// Create SkeletonDataResource
	Ref<SpineSkeletonDataResource> data_res;
	data_res.instantiate();
	data_res->set_default_mix(default_mix);
	data_res->set_atlas_res(atlas_res);
	data_res->set_skeleton_file_res(skeleton_file);

	// Validate data was loaded successfully
	if (!data_res->is_skeleton_data_loaded()) {
		print_error(vformat("[SpxSpineMgr] Failed to create skeleton data from: %s", cache_key));
		return Ref<SpineSkeletonDataResource>();
	}

	// Store in cache
	cached_skeleton_data[cache_key] = data_res;
	print_line(vformat("[SpxSpineMgr] Spine data loaded and cached: %s", cache_key));

	return data_res;
}

void SpxSpineMgr::update_caches(const Vector<String> &files) {
	for (const String &file : files) {
		String abs_path = _to_abs_path(file);

		// Check if affects Atlas cache
		if (cached_atlas.has(abs_path)) {
			print_line(vformat("[SpxSpineMgr] Atlas cache invalidated: %s", abs_path));
			cached_atlas.erase(abs_path);
		}

		// Check if affects SkeletonFile cache
		if (cached_skeleton_file.has(abs_path)) {
			print_line(vformat("[SpxSpineMgr] SkeletonFile cache invalidated: %s", abs_path));
			cached_skeleton_file.erase(abs_path);
		}

		// Check if affects SkeletonData cache (all combinations containing this path)
		Vector<String> keys_to_remove;
		for (const auto &entry : cached_skeleton_data) {
			if (entry.key.contains(abs_path)) {
				keys_to_remove.push_back(entry.key);
			}
		}
		for (const String &key : keys_to_remove) {
			print_line(vformat("[SpxSpineMgr] SkeletonData cache invalidated: %s", key));
			cached_skeleton_data.erase(key);
		}
	}
}

void SpxSpineMgr::clear_all_caches() {
	cached_atlas.clear();
	cached_skeleton_file.clear();
	cached_skeleton_data.clear();
	print_line("[SpxSpineMgr] All caches cleared");
}

