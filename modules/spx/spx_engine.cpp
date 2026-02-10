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

void SpxEngine::register_runtime_panic_callbacks(GDExtensionSpxGlobalRuntimePanicCallback callback) {
	singleton->on_runtime_panic = callback;
}

void SpxEngine::register_runtime_exit_callbacks(GDExtensionSpxGlobalRuntimeExitCallback callback) {
	singleton->on_runtime_exit = callback;
}

void SpxEngine::register_runtime_reset_callbacks(GDExtensionSpxGlobalRuntimeResetCallback callback) {
	singleton->on_runtime_reset = callback;
}

static SpxCallbackInfo get_default_spx_callbacks() {
	SpxCallbackInfo callbacks;
	callbacks.func_on_engine_start = [](){};
	callbacks.func_on_engine_fixed_update = [](GdFloat delta){};
	callbacks.func_on_engine_update = [](GdFloat delta){};
	callbacks.func_on_engine_destroy = [](){};
	callbacks.func_on_engine_reset = [](){};
	callbacks.func_on_engine_pause = [](GdBool is_paused){};
	callbacks.func_on_scene_sprite_instantiated = [](GdObj obj, GdString type_name){};
	callbacks.func_on_sprite_ready = [](GdObj obj){};
	callbacks.func_on_sprite_updated = [](GdFloat delta){};
	callbacks.func_on_sprite_fixed_updated = [](GdFloat delta){};
	callbacks.func_on_sprite_destroyed = [](GdObj obj){};
	callbacks.func_on_sprite_frames_set_changed = [](GdObj obj){};
	callbacks.func_on_sprite_animation_changed = [](GdObj obj){};
	callbacks.func_on_sprite_frame_changed = [](GdObj obj){};
	callbacks.func_on_sprite_animation_looped = [](GdObj obj){};
	callbacks.func_on_sprite_animation_finished = [](GdObj obj){};
	callbacks.func_on_sprite_vfx_finished = [](GdObj obj){};
	callbacks.func_on_sprite_screen_exited = [](GdObj obj){};
	callbacks.func_on_sprite_screen_entered = [](GdObj obj){};
	callbacks.func_on_mouse_pressed = [](GdInt keyid){};
	callbacks.func_on_mouse_released = [](GdInt keyid){};
	callbacks.func_on_key_pressed = [](GdInt keyid){};
	callbacks.func_on_key_released = [](GdInt keyid){};
	callbacks.func_on_action_pressed = [](GdString action_name){};
	callbacks.func_on_action_just_pressed = [](GdString action_name){};
	callbacks.func_on_action_just_released = [](GdString action_name){};
	callbacks.func_on_axis_changed = [](GdString action_name, GdFloat value){};
	callbacks.func_on_collision_enter = [](GdInt self_id, GdInt other_id){};
	callbacks.func_on_collision_stay = [](GdInt self_id, GdInt other_id){};
	callbacks.func_on_collision_exit = [](GdInt self_id, GdInt other_id){};
	callbacks.func_on_trigger_enter = [](GdInt self_id, GdInt other_id){};
	callbacks.func_on_trigger_stay = [](GdInt self_id, GdInt other_id){};
	callbacks.func_on_trigger_exit = [](GdInt self_id, GdInt other_id){};
	callbacks.func_on_ui_ready = [](GdObj obj){};
	callbacks.func_on_ui_updated = [](GdObj obj){};
	callbacks.func_on_ui_destroyed = [](GdObj obj){};
	callbacks.func_on_ui_pressed = [](GdObj obj){};
	callbacks.func_on_ui_released = [](GdObj obj){};
	callbacks.func_on_ui_hovered = [](GdObj obj){};
	callbacks.func_on_ui_clicked = [](GdObj obj){};
	callbacks.func_on_ui_toggle = [](GdObj obj, GdBool is_on){};
	callbacks.func_on_ui_text_changed = [](GdObj obj, GdString text){};
	return callbacks;
}


void SpxEngine::register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr) {
	if (singleton != nullptr) {
		print_error("SpxEngine::register_callbacks failed, already initialed! ");
		return;
	}
	singleton = new SpxEngine();
	singleton->mgrs.clear();
	
	// Initialize all managers using factory pattern
	singleton->_initialize_managers();
	
	singleton->callbacks = *(SpxCallbackInfo *)callback_ptr;
	singleton->global_id = 1;
	singleton->is_spx_paused = false;
	singleton->should_execute_single_frame = false;
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

	if(!delay_proxy){
		delay_proxy = memnew(SpxCallbackProxy);
        tree->get_root()->add_child(delay_proxy);
		on_timeout_callable = Callable(delay_proxy, "_on_timeout");
	}
}

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
	if (callbacks.func_on_engine_start != nullptr) {
		callbacks.func_on_engine_start();
	}
}

void SpxEngine::on_fixed_update(float delta) {
	if (has_exit) {
		return;
	}
	
	// Single frame debugging mode: execute one frame even when paused
	if (is_spx_paused && !should_execute_single_frame) {
		return; // Normal pause, don't execute
	}
	
	for (auto mgr : mgrs) {
		mgr->on_fixed_update(delta);
	}
	if (callbacks.func_on_engine_fixed_update != nullptr) {
		callbacks.func_on_engine_fixed_update(delta);
	}
}

void SpxEngine::on_update(float delta) {
	if (has_exit) {
		return;
	}
	
	// Single frame debugging mode: execute one frame even when paused
	if (is_spx_paused && !should_execute_single_frame) {
		return; // Normal pause, don't execute
	}
	
	if (should_execute_single_frame) {
		should_execute_single_frame = false;
		// Execute single frame logic, then re-pause
		// We'll re-pause at the end of this method
	}
	
	if(is_defer_call_pause){
		_on_godot_pause_changed(defer_pause_value);
		is_defer_call_pause = false;
	}

	for (auto mgr : mgrs) {
		mgr->on_update(delta);
	}
	if (callbacks.func_on_engine_update != nullptr) {
		callbacks.func_on_engine_update(delta);
	}
	
	// Re-pause after single frame execution
	if (is_spx_paused && !tree->is_paused()) {
		if (Thread::is_main_thread()) {
			tree->set_pause(true);
		} else {
			tree->call_deferred("set_pause", true);
		}
	}
}

void SpxEngine::on_exit(int exit_code) {
	if (has_exit) {
		return;
	}

	capture_last_frame();
	has_exit = true;
	for (auto mgr : mgrs) {
		mgr->on_exit(exit_code);
	}
	// remove all runtime callbacks
	callbacks = get_default_spx_callbacks();
}

void SpxEngine::on_destroy() {
	// Call on_destroy for all managers
	for (auto mgr : mgrs) {
		mgr->on_destroy();
	}

	if (!has_exit) {
		if (callbacks.func_on_engine_destroy != nullptr) {
			callbacks.func_on_engine_destroy();
		}
	}

	if(delay_proxy){
		delay_proxy->queue_free();
		delay_proxy = nullptr;
	}

	callbacks = get_default_spx_callbacks();
	
	// Destroy svg global manager
	svgMgr->destroy();

	// Use the centralized destroy method for proper cleanup
	destroy_all_managers();
	singleton = nullptr;
}

bool SpxEngine::is_reset() {
	return is_spx_reset;
}

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

	if (callbacks.func_on_engine_start != nullptr) {
		callbacks.func_on_engine_start();
	}
}

void SpxEngine::on_reset(int reset_code) {
	if (is_spx_reset) {
		return;
	}

	is_spx_reset = true;
	capture_last_frame();
	_do_reset(reset_code);
}

// SPX Pause functionality implementation with thread safety
void SpxEngine::pause() {
	if (tree != nullptr) {
		if (Thread::is_main_thread()) {
			// Direct call on main thread
			tree->set_pause(true);
			// Directly notify about pause state change
			_on_godot_pause_changed(true);
		} else {
			// Use SceneTree to defer call to main thread
			tree->call_deferred("set_pause", true);
			is_defer_call_pause = true;
			defer_pause_value = true;
			// Defer the pause notification as well
			//callable_mp(this, &SpxEngine::_on_godot_pause_changed).call_deferred(true);
		}
	}
}

void SpxEngine::resume() {
	if (tree != nullptr) {
		if (Thread::is_main_thread()) {
			// Direct call on main thread
			tree->set_pause(false);
			// Directly notify about pause state change
			_on_godot_pause_changed(false);
		} else {
			// Use SceneTree to defer call to main thread
			tree->call_deferred("set_pause", false);
			is_defer_call_pause = true;
			defer_pause_value = false;
		}
	}
}

bool SpxEngine::is_paused() const {
	return is_spx_paused;
}

void SpxEngine::next_frame() {
	if (!is_spx_paused) {
		return; // Only effective when paused
	}
	
	// Temporarily unpause Godot engine to allow physics and signals
	if (tree != nullptr) {
		if (Thread::is_main_thread()) {
			// Unpause to allow one frame of physics processing
			tree->set_pause(false);
			should_execute_single_frame = true;
			// Note: We'll re-pause after processing in on_update()
		} else {
			// Defer unpause to main thread
			tree->call_deferred("set_pause", false);
			should_execute_single_frame = true;
		}
	}
}

void SpxEngine::capture_last_frame() {
	if (is_frozen_frame) return;
	if (!tree) return;

	Viewport *vp = tree->get_root();
	if (!vp) return;

	Ref<Image> img = vp->get_texture()->get_image();
	if (img.is_null()) return;

   	Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
    freeze_screen = memnew(TextureRect);
    freeze_screen->set_texture(tex);
    freeze_screen->set_stretch_mode(TextureRect::STRETCH_SCALE);
    freeze_screen->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    freeze_screen->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);

	if (!freeze_layer) {
		freeze_layer = memnew(CanvasLayer);
		freeze_layer->set_layer(1);
		vp->add_child(freeze_layer);
	}

	freeze_layer->add_child(freeze_screen);
	is_frozen_frame = true;
}

void SpxEngine::clear_frozen_frame() {
	if (!is_frozen_frame) return;

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

// Internal method for Godot pause synchronization
void SpxEngine::_on_godot_pause_changed(bool is_godot_paused) {
	if (is_godot_paused != is_spx_paused) {
		is_spx_paused = is_godot_paused;
		
		// Notify all managers about pause/resume
		for (auto mgr : mgrs) {
			if (is_spx_paused) {
				mgr->on_pause();
			} else {
				mgr->on_resume();
			}
		}
		
		// Call the pause callback to notify SPX users
		if (callbacks.func_on_engine_pause != nullptr) {
			callbacks.func_on_engine_pause(is_spx_paused);
		}
	}
}

void SpxEngine::_do_reset(int reset_code) {
	if (callbacks.func_on_engine_reset != nullptr) {
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
		if (callback != nullptr) {
			callback(reset_code);
		}
	};

	reset_timer->connect(
		"timeout",
		on_timeout_callable
	);
}

void SpxEngine::_pause_pure() {
	print_error("_pause_pure..........");
	if (tree != nullptr) {
		if (Thread::is_main_thread()) {
			tree->set_pause(true);
		} else {
			tree->call_deferred("set_pause", true);
		}
	}
	is_spx_paused = true;
}

void SpxEngine::_resume_pure() {
	if (tree != nullptr) {
		if (Thread::is_main_thread()) {
			tree->set_pause(false);
		} else {
			tree->call_deferred("set_pause", false);
		}
	}

	is_spx_paused = false;
}

void SpxEngine::_disconnect_reset_timer() {
	if (!reset_timer.is_null() && reset_timer.is_valid() && reset_timer->has_connections("timeout")) {
		reset_timer->disconnect("timeout", on_timeout_callable);
	}
}

void SpxEngine::_initialize_managers() {
	// Initialize managers using the factory pattern
	// This ensures consistent creation and registration
	input = create_manager<SpxInputMgr>();
	audio = create_manager<SpxAudioMgr>();
	physics = create_manager<SpxPhysicsMgr>();
	sprite = create_manager<SpxSpriteMgr>();
	ui = create_manager<SpxUiMgr>();
	scene = create_manager<SpxSceneMgr>();
	camera = create_manager<SpxCameraMgr>();
	platform = create_manager<SpxPlatformMgr>();
	res = create_manager<SpxResMgr>();
	ext = create_manager<SpxExtMgr>();
	debug = create_manager<SpxDebugMgr>();
	navigation = create_manager<SpxNavigationMgr>();
	pen = create_manager<SpxPenMgr>();
	tilemap = create_manager<SpxTilemapMgr>();
	tilemapparser = create_manager<SpxTilemapparserMgr>();
}

void SpxEngine::destroy_all_managers() {
	// Destroy managers in reverse order to ensure proper cleanup
	// This matches the RAII pattern and handles dependencies correctly
	if (tilemapparser) { memdelete(tilemapparser); tilemapparser = nullptr; }
	if (tilemap) { memdelete(tilemap); tilemap = nullptr; }
	if (pen) { memdelete(pen); pen = nullptr; }
	if (navigation) { memdelete(navigation); navigation = nullptr; }
	if (debug) { memdelete(debug); debug = nullptr; }
	if (ext) { memdelete(ext); ext = nullptr; }
	if (res) { memdelete(res); res = nullptr; }
	if (platform) { memdelete(platform); platform = nullptr; }
	if (camera) { memdelete(camera); camera = nullptr; }
	if (scene) { memdelete(scene); scene = nullptr; }
	if (ui) { memdelete(ui); ui = nullptr; }
	if (sprite) { memdelete(sprite); sprite = nullptr; }
	if (physics) { memdelete(physics); physics = nullptr; }
	if (audio) { memdelete(audio); audio = nullptr; }
	if (input) { memdelete(input); input = nullptr; }
	
	mgrs.clear();
}
