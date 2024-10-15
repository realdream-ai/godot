/**************************************************************************/
/*  spx_engine.cpp                                                        */
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

#include "spx_engine.h"
#include "core/extension/gdextension.h"
#include "core/os/memory.h"
#include "gdextension_spx_ext.h"
#include "scene/main/window.h"
#include "spx_input_mgr.h"
#include "spx_audio_mgr.h"
#include "spx_physic_mgr.h"
#include "spx_sprite_mgr.h"
#include "spx_ui_mgr.h"
#include "spx_camera_mgr.h"
#include "spx_scene_mgr.h"
#include "spx_platform_mgr.h"
#include "spx_res_mgr.h"
SpxEngine *SpxEngine::singleton = nullptr;

void SpxEngine::register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr) {
	if (singleton != nullptr) {
		print_error("SpxEngine::register_callbacks failed, already initialed! ");
		return;
	}
	singleton = new SpxEngine();
	singleton->mgrs.clear();
	singleton->input = memnew(SpxInputMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->input);
	singleton->audio = memnew(SpxAudioMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->audio);
	singleton->physic = memnew(SpxPhysicMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->physic);
	singleton->sprite = memnew(SpxSpriteMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->sprite);
	singleton->ui = memnew(SpxUiMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->ui);
	singleton->scene = memnew(SpxSceneMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->scene);
	singleton->camera = memnew(SpxCameraMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->camera);
	singleton->platform = memnew(SpxPlatformMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->platform);
	singleton->res = memnew(SpxResMgr);
	singleton->mgrs.append((SpxBaseMgr *)singleton->res);


	singleton->callbacks = *(SpxCallbackInfo *)callback_ptr;
	singleton->global_id = 1;
}

SpxCallbackInfo *SpxEngine::get_callbacks() {
	return &callbacks;
}

GdInt SpxEngine::get_unique_id() {
	return global_id++;
}

Node *SpxEngine::get_spx_root() {
	return spx_root;
}

SceneTree * SpxEngine::get_tree() {
	return tree;
}

Window *SpxEngine::get_root() {
	return tree->get_root();
}

void SpxEngine::set_root_node(SceneTree *p_tree, Node *p_node) {
	this->tree = p_tree;
	spx_root = p_node;
}

void SpxEngine::on_awake() {
	for (auto mgr : mgrs) {
		mgr->on_awake();
	}
	for (auto mgr : mgrs) {
		mgr->on_start();
	}
	if (callbacks.func_on_engine_start != nullptr) {
		callbacks.func_on_engine_start();
	}
}

void SpxEngine::on_fixed_update(float delta) {
	for (auto mgr : mgrs) {
		mgr->on_fixed_update(delta);
	}
	if (callbacks.func_on_engine_fixed_update != nullptr) {
		callbacks.func_on_engine_fixed_update(delta);
	}
}

void SpxEngine::on_update(float delta) {
	for (auto mgr : mgrs) {
		mgr->on_update(delta);
	}
	if (callbacks.func_on_engine_update != nullptr) {
		callbacks.func_on_engine_update(delta);
	}
}

void SpxEngine::on_destroy() {
	for (auto mgr : mgrs) {
		mgr->on_destroy();
	}
	if (callbacks.func_on_engine_destroy != nullptr) {
		callbacks.func_on_engine_destroy();
	}
	callbacks = SpxCallbackInfo();
	memdelete(input);
	memdelete(audio);
	memdelete(physic);
	memdelete(sprite);
	memdelete(ui);
	memdelete(scene);
	memdelete(camera);
	memdelete(platform);
	memdelete(res);
	mgrs.clear();
	singleton = nullptr;
}
