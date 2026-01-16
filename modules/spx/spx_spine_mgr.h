/**************************************************************************/
/*  spx_spine_mgr.h                                                       */
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

#ifndef SPX_SPINE_MGR_H
#define SPX_SPINE_MGR_H

#include "spx_base_mgr.h"
#include "modules/spine_godot/SpineAtlasResource.h"
#include "modules/spine_godot/SpineSkeletonFileResource.h"
#include "modules/spine_godot/SpineSkeletonDataResource.h"

class SpxSpineMgr : public SpxBaseMgr {
	SPXCLASS(SpxSpineMgr, SpxBaseMgr)

public:
	virtual ~SpxSpineMgr() = default;

private:
	// First layer cache: Atlas resources (Key: absolute path)
	HashMap<String, Ref<SpineAtlasResource>> cached_atlas;

	// Second layer cache: Skeleton file resources (Key: absolute path)
	HashMap<String, Ref<SpineSkeletonFileResource>> cached_skeleton_file;

	// Third layer cache: Combined SkeletonData (Key: atlas_path|skeleton_path)
	HashMap<String, Ref<SpineSkeletonDataResource>> cached_skeleton_data;

	String _make_cache_key(const String &atlas_path, const String &skeleton_path);
	String _to_abs_path(const String &path);

public:
	void on_awake() override;
	void on_reset(int reset_code) override;

	// Main load method - external entry point
	Ref<SpineSkeletonDataResource> load_spine_data(
			const String &atlas_path,
			const String &skeleton_path,
			float default_mix = 0.1f);

	// Individual load methods - with caching
	Ref<SpineAtlasResource> load_atlas(const String &path);
	Ref<SpineSkeletonFileResource> load_skeleton_file(const String &path);

	// Cache management
	void update_caches(const Vector<String> &files);
	
public:
	void clear_all_caches();
};

#endif // SPX_SPINE_MGR_H

