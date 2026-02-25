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

#include "core/config/project_settings.h"
#include "core/extension/gdextension.h"
#include "core/os/memory.h"
#include "core/os/thread.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/rendering_server.h"

#include "gdextension_spx_ext.h"
#include "spx_audio_mgr.h"
#include "spx_callback_proxy.h"
#include "spx_camera_mgr.h"
#include "spx_debug_mgr.h"
#include "spx_ext_mgr.h"
#include "spx_input_mgr.h"
#include "spx_navigation_mgr.h"
#include "spx_pen_mgr.h"
#include "spx_physics_mgr.h"
#include "spx_platform_mgr.h"
#include "spx_res_mgr.h"
#include "spx_scene_mgr.h"
#include "spx_sprite_mgr.h"
#include "spx_tilemap_mgr.h"
#include "spx_tilemapparser_mgr.h"
#include "spx_ui_mgr.h"

// Register runtime panic callback.
void SpxEngine::register_runtime_panic_callbacks(GDExtensionSpxGlobalRuntimePanicCallback callback) {
	singleton->on_runtime_panic = callback;
}

// Register runtime exit callback.
void SpxEngine::register_runtime_exit_callbacks(GDExtensionSpxGlobalRuntimeExitCallback callback) {
	singleton->on_runtime_exit = callback;
}

// Register runtime reset callback.
void SpxEngine::register_runtime_reset_callbacks(GDExtensionSpxGlobalRuntimeResetCallback callback) {
	singleton->on_runtime_reset = callback;
}

// Get default SPX callbacks with empty lambdas.
static SpxCallbackInfo get_default_spx_callbacks() {
	SpxCallbackInfo callbacks;
	callbacks.func_on_engine_start = []() {};
	callbacks.func_on_engine_fixed_update = [](GdFloat delta) {};
	callbacks.func_on_engine_update = [](GdFloat delta) {};
	callbacks.func_on_engine_destroy = []() {};
	callbacks.func_on_engine_reset = []() {};
	callbacks.func_on_engine_pause = [](GdBool is_paused) {};
	callbacks.func_on_scene_sprite_instantiated = [](GdObj obj, GdString type_name) {};
	callbacks.func_on_sprite_ready = [](GdObj obj) {};
	callbacks.func_on_sprite_updated = [](GdFloat delta) {};
	callbacks.func_on_sprite_fixed_updated = [](GdFloat delta) {};
	callbacks.func_on_sprite_destroyed = [](GdObj obj) {};
	callbacks.func_on_sprite_frames_set_changed = [](GdObj obj) {};
	callbacks.func_on_sprite_animation_changed = [](GdObj obj) {};
	callbacks.func_on_sprite_frame_changed = [](GdObj obj) {};
	callbacks.func_on_sprite_animation_looped = [](GdObj obj) {};
	callbacks.func_on_sprite_animation_finished = [](GdObj obj) {};
	callbacks.func_on_sprite_vfx_finished = [](GdObj obj) {};
	callbacks.func_on_sprite_screen_exited = [](GdObj obj) {};
	callbacks.func_on_sprite_screen_entered = [](GdObj obj) {};
	callbacks.func_on_mouse_pressed = [](GdInt keyid) {};
	callbacks.func_on_mouse_released = [](GdInt keyid) {};
	callbacks.func_on_key_pressed = [](GdInt keyid) {};
	callbacks.func_on_key_released = [](GdInt keyid) {};
	callbacks.func_on_action_pressed = [](GdString action_name) {};
	callbacks.func_on_action_just_pressed = [](GdString action_name) {};
	callbacks.func_on_action_just_released = [](GdString action_name) {};
	callbacks.func_on_axis_changed = [](GdString action_name, GdFloat value) {};
	callbacks.func_on_collision_enter = [](GdInt self_id, GdInt other_id) {};
	callbacks.func_on_collision_stay = [](GdInt self_id, GdInt other_id) {};
	callbacks.func_on_collision_exit = [](GdInt self_id, GdInt other_id) {};
	callbacks.func_on_trigger_enter = [](GdInt self_id, GdInt other_id) {};
	callbacks.func_on_trigger_stay = [](GdInt self_id, GdInt other_id) {};
	callbacks.func_on_trigger_exit = [](GdInt self_id, GdInt other_id) {};
	callbacks.func_on_ui_ready = [](GdObj obj) {};
	callbacks.func_on_ui_updated = [](GdObj obj) {};
	callbacks.func_on_ui_destroyed = [](GdObj obj) {};
	callbacks.func_on_ui_pressed = [](GdObj obj) {};
	callbacks.func_on_ui_released = [](GdObj obj) {};
	callbacks.func_on_ui_hovered = [](GdObj obj) {};
	callbacks.func_on_ui_clicked = [](GdObj obj) {};
	callbacks.func_on_ui_toggle = [](GdObj obj, GdBool is_on) {};
	callbacks.func_on_ui_text_changed = [](GdObj obj, GdString text) {};
	return callbacks;
}

// Register callbacks and initialize engine singleton.
void SpxEngine::register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr) {
	if (singleton != nullptr) {
		print_error("SpxEngine::register_callbacks failed, already initialed! ");
		return;
	}
	singleton = new SpxEngine();
	singleton->mgrs.clear();
	singleton->_initialize_managers();
	singleton->callbacks = *(SpxCallbackInfo *)callback_ptr;
	singleton->global_id = 1;
	singleton->is_spx_paused = false;
	singleton->should_execute_single_frame = false;
}

// Get callbacks struct.
SpxCallbackInfo *SpxEngine::get_callbacks() {
	return &callbacks;
}

// Get next unique ID.
GdInt SpxEngine::get_unique_id() {
	return global_id++;
}

// Get SPX root node.
Node *SpxEngine::get_spx_root() {
	return spx_root;
}

// Get scene tree.
SceneTree *SpxEngine::get_tree() {
	return tree;
}

// Get root window.
Window *SpxEngine::get_root() {
	return tree->get_root();
}

// Set root node and initialize callback proxy.
void SpxEngine::set_root_node(SceneTree *p_tree, Node *p_node) {
	this->tree = p_tree;
	spx_root = p_node;
	if (!delay_proxy) {
		delay_proxy = memnew(SpxCallbackProxy);
		tree->get_root()->add_child(delay_proxy);
		on_timeout_callable = Callable(delay_proxy, "_on_timeout");
	}
}

// Called when engine awakes.
void SpxEngine::on_awake() {
	if (has_exit) {
		return;
	}

	for (auto mgr : mgrs) {
		mgr->on_awake();
	}

	for (auto mgr : mgrs) {
		mgr->on_start();
	}

	if (callbacks.func_on_engine_start) {
		callbacks.func_on_engine_start();
	}
}

// Called on fixed update.
void SpxEngine::on_fixed_update(float delta) {
	if (has_exit) {
		return;
	}

	if (is_spx_paused && !should_execute_single_frame) {
		return;
	}

	for (auto mgr : mgrs) {
		mgr->on_fixed_update(delta);
	}

	if (callbacks.func_on_engine_fixed_update) {
		callbacks.func_on_engine_fixed_update(delta);
	}
}

// Called on frame update.
void SpxEngine::on_update(float delta) {
	if (has_exit) {
		return;
	}

	if (is_spx_paused && !should_execute_single_frame) {
		return;
	}

	// ---- internal state sync ----
	if (should_execute_single_frame) {
		should_execute_single_frame = false;
	}

	if (is_defer_call_pause) {
		_on_godot_pause_changed(defer_pause_value);
		is_defer_call_pause = false;
	}

	// ---- managers ----
	for (auto mgr : mgrs) {
		mgr->on_update(delta);
	}

	// ---- callbacks ----
	if (callbacks.func_on_engine_update) {
		callbacks.func_on_engine_update(delta);
	}

	// ---- pause sync ----
	if (is_spx_paused && !tree->is_paused()) {
		if (Thread::is_main_thread()) {
			tree->set_pause(true);
		} else {
			tree->call_deferred("set_pause", true);
		}
	}
}

// Called when engine exits.
void SpxEngine::on_exit(int exit_code) {
	if (has_exit) {
		return;
	}

	capture_last_frame();
	has_exit = true;

	for (auto mgr : mgrs) {
		mgr->on_exit(exit_code);
	}

	callbacks = get_default_spx_callbacks();
}

// Called when engine destroys.
void SpxEngine::on_destroy() {
	for (auto mgr : mgrs) {
		mgr->on_destroy();
	}

	if (!has_exit) {
		if (callbacks.func_on_engine_destroy) {
			callbacks.func_on_engine_destroy();
		}
	}

	if (delay_proxy) {
		delay_proxy->queue_free();
		delay_proxy = nullptr;
	}

	callbacks = get_default_spx_callbacks();
	svgMgr->destroy();
	destroy_all_managers();
	singleton = nullptr;
}

// Check if engine is in reset state.
bool SpxEngine::is_reset() {
	return is_spx_reset;
}

// Restart engine from reset state.
void SpxEngine::restart() {
	if (!is_spx_reset) {
		return;
	}

	_disconnect_reset_timer();
	clear_frozen_frame();
	_resume_pure();
	is_spx_reset = false;

	for (auto mgr : mgrs) {
		mgr->on_start();
	}

	if (callbacks.func_on_engine_start) {
		callbacks.func_on_engine_start();
	}
}

// Called when engine resets.
void SpxEngine::on_reset(int reset_code) {
	if (is_spx_reset) {
		return;
	}

	is_spx_reset = true;
	capture_last_frame();
	_do_reset(reset_code);
}

// Pause the engine.
void SpxEngine::pause() {
	if (!tree) {
		return;
	}

	if (Thread::is_main_thread()) {
		tree->set_pause(true);
		_on_godot_pause_changed(true);
	} else {
		tree->call_deferred("set_pause", true);
		is_defer_call_pause = true;
		defer_pause_value = true;
	}
}

// Resume the engine.
void SpxEngine::resume() {
	if (!tree) {
		return;
	}

	if (Thread::is_main_thread()) {
		tree->set_pause(false);
		_on_godot_pause_changed(false);
	} else {
		tree->call_deferred("set_pause", false);
		is_defer_call_pause = true;
		defer_pause_value = false;
	}
}

// Check if engine is paused.
bool SpxEngine::is_paused() const {
	return is_spx_paused;
}

// Execute single frame when paused.
void SpxEngine::next_frame() {
	if (!is_spx_paused) {
		return;
	}

	if (!tree) {
		return;
	}

	if (Thread::is_main_thread()) {
		tree->set_pause(false);
		should_execute_single_frame = true;
	} else {
		tree->call_deferred("set_pause", false);
		should_execute_single_frame = true;
	}
}

// Capture current frame as frozen texture.
void SpxEngine::capture_last_frame() {
	if (is_frozen_frame || !tree) {
		return;
	}

	Ref<Image> img = _get_viewport_image();
	if (img.is_null()) {
		return;
	}

	freeze_screen = _create_freeze_texture(img);
	_attach_freeze_node(freeze_screen);
	is_frozen_frame = true;
}

// Clear frozen frame texture.
void SpxEngine::clear_frozen_frame() {
	if (!is_frozen_frame) {
		return;
	}

	if (freeze_screen) {
		freeze_screen->queue_free();
		freeze_screen = nullptr;
	}

	if (freeze_layer) {
		freeze_layer->queue_free();
		freeze_layer = nullptr;
	}

	is_frozen_frame = false;
}

// Handle Godot pause state change.
void SpxEngine::_on_godot_pause_changed(bool is_godot_paused) {
	if (is_godot_paused != is_spx_paused) {
		is_spx_paused = is_godot_paused;

		for (auto mgr : mgrs) {
			if (is_spx_paused) {
				mgr->on_pause();
			} else {
				mgr->on_resume();
			}
		}

		if (callbacks.func_on_engine_pause) {
			callbacks.func_on_engine_pause(is_spx_paused);
		}
	}
}

// Execute reset logic.
void SpxEngine::_do_reset(int reset_code) {
	if (callbacks.func_on_engine_reset) {
		callbacks.func_on_engine_reset();
	}

	for (auto mgr : mgrs) {
		mgr->on_reset(reset_code);
	}

	SvgManager::get_singleton()->reset(false);

	if (!tree || !delay_proxy) {
		return;
	}

	_disconnect_reset_timer();
	reset_timer = tree->create_timer(RESET_PAUSE_DELAY_SEC);
	delay_proxy->callback = [this, reset_code]() {
		this->_pause_pure();
		auto callback = get_on_runtime_reset();
		if (callback) {
			callback(reset_code);
		}
	};
	reset_timer->connect("timeout", on_timeout_callable);
}

// Pause without callbacks.
void SpxEngine::_pause_pure() {
	if (!tree) {
		is_spx_paused = true;
		return;
	}

	if (Thread::is_main_thread()) {
		tree->set_pause(true);
	} else {
		tree->call_deferred("set_pause", true);
	}

	is_spx_paused = true;
}

// Resume without callbacks.
void SpxEngine::_resume_pure() {
	if (!tree) {
		is_spx_paused = false;
		return;
	}

	if (Thread::is_main_thread()) {
		tree->set_pause(false);
	} else {
		tree->call_deferred("set_pause", false);
	}

	is_spx_paused = false;
}

// Disconnect reset timer signal.
void SpxEngine::_disconnect_reset_timer() {
	if (!reset_timer.is_null() && reset_timer.is_valid() && reset_timer->has_connections("timeout")) {
		reset_timer->disconnect("timeout", on_timeout_callable);
	}
}

Ref<Image> SpxEngine::_get_viewport_image() const {
	Viewport *vp = tree->get_root();
	if (!vp) {
		return Ref<Image>();
	}
	return vp->get_texture()->get_image();
}

TextureRect *SpxEngine::_create_freeze_texture(const Ref<Image> &img) const {
	Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
	TextureRect *screen = memnew(TextureRect);
	screen->set_texture(tex);
	screen->set_stretch_mode(TextureRect::STRETCH_SCALE);
	screen->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	screen->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return screen;
}

void SpxEngine::_attach_freeze_node(TextureRect *screen) {
	Viewport *vp = tree->get_root();
	if (!vp || !screen) {
		return;
	}

	if (!freeze_layer) {
		freeze_layer = memnew(CanvasLayer);
		freeze_layer->set_layer(1);
		vp->add_child(freeze_layer);
	}

	freeze_layer->add_child(screen);
}

// Initialize all managers.
void SpxEngine::_initialize_managers() {
	// Core.
	input = create_manager<SpxInputMgr>();
	audio = create_manager<SpxAudioMgr>();
	physics = create_manager<SpxPhysicsMgr>();

	// Rendering.
	sprite = create_manager<SpxSpriteMgr>();
	ui = create_manager<SpxUiMgr>();
	scene = create_manager<SpxSceneMgr>();
	camera = create_manager<SpxCameraMgr>();

	// System.
	platform = create_manager<SpxPlatformMgr>();
	res = create_manager<SpxResMgr>();
	ext = create_manager<SpxExtMgr>();
	debug = create_manager<SpxDebugMgr>();

	// Scene.
	navigation = create_manager<SpxNavigationMgr>();
	pen = create_manager<SpxPenMgr>();
	tilemap = create_manager<SpxTilemapMgr>();
	tilemapparser = create_manager<SpxTilemapparserMgr>();
}

// Destroy all managers in reverse order.
void SpxEngine::destroy_all_managers() {
	for (int i = mgrs.size() - 1; i >= 0; --i) {
		memdelete(mgrs[i]);
	}
	mgrs.clear();

	input = nullptr;
	audio = nullptr;
	physics = nullptr;
	sprite = nullptr;
	ui = nullptr;
	scene = nullptr;
	camera = nullptr;
	platform = nullptr;
	res = nullptr;
	ext = nullptr;
	debug = nullptr;
	navigation = nullptr;
	pen = nullptr;
	tilemap = nullptr;
	tilemapparser = nullptr;
}
