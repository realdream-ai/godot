/**************************************************************************/
/*  spx.cpp                                                               */
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

#include "spx.h"

#include "core/extension/spx_util.h"
#include "core/io/dir_access.h"
#include "core/object/class_db.h"
#include "core/os/thread.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

#include "gdextension_spx_ext.h"
#include "spx_callback_proxy.h"
#include "spx_draw_tiles.h"
#include "spx_engine.h"
#include "spx_input_proxy.h"
#include "spx_path_finder.h"
#include "spx_sprite.h"
#include "spx_ui.h"

//#define MINIZIP_ENABLED
#ifdef MINIZIP_ENABLED
#include "modules/zip/zip_reader.h"
#endif

// Simple node class for initialization
class SpxEngineNode : public Node {
	GDCLASS(SpxEngineNode, Node);
};

   
#define SPX_ENGINE SpxEngine::get_singleton()

void Spx::register_extension_functions() {
	SpxUtil::register_func = &gdextension_spx_setup_interface;
	SpxUtil::debug_mode = debug_mode;
}

void Spx::set_debug_mode(bool enable) {
	debug_mode = enable;
	SpxUtil::debug_mode = enable;
}

void Spx::register_types() {
	ClassDB::register_class<SpxSprite>();
	ClassDB::register_class<SpxInputProxy>();
	ClassDB::register_class<SpxDrawTiles>();
	ClassDB::register_class<SpxPathFinder>();
	ClassDB::register_class<PathDebugDrawer>();
	ClassDB::register_class<SpxCallbackProxy>();
}

void Spx::on_start(void *p_tree) {
	initialed = true;
	restart_requested.clear();  // Initialize restart flag to false
	reset_requested.clear();  // Initialize reset flag to false
	reset_exit_code.set(0);  // Initialize reset exit code to 0
	pause_requested.clear();  // Initialize pause flag to false
	resume_requested.clear();  // Initialize resume flag to false
	next_frame_requested.clear();  // Initialize next_frame flag to false
	if (!SpxEngine::has_initialed()) {
		return;
	}
	auto tree = (SceneTree *)p_tree;
	if (tree == nullptr)
		return;
	Window *root = tree->get_root();
	if (root == nullptr) {
		return;
	}

	SpxEngineNode *new_node = memnew(SpxEngineNode);
	new_node->set_name("SpxEngineNode");
	root->add_child(new_node);
	SPX_ENGINE->set_root_node(tree, new_node);
	SPX_ENGINE->on_awake();
}

void Spx::on_fixed_update(double delta) {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	SPX_ENGINE->on_fixed_update(delta);
}

void Spx::on_update(double delta) {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	// Check if restart was requested from a non-main thread
	if (restart_requested.is_set()) {
		restart_requested.clear();  // Clear the flag first
		SPX_ENGINE->restart();
		return;  // Skip the normal update after restart
	}

	// Check if reset was requested from a non-main thread
	if (reset_requested.is_set()) {
		// Read exit code BEFORE clearing the flag to avoid TOCTOU race condition
		int exit_code = reset_exit_code.get();
		reset_requested.clear();  // Clear the flag after reading
		SPX_ENGINE->on_reset(exit_code);
		auto callback = SPX_ENGINE->get_on_runtime_reset();
		if (callback != nullptr) {
			callback(exit_code);
		}
		return;  // Skip the normal update after reset
	}

	// Check if pause was requested from a non-main thread
	if (pause_requested.is_set()) {
		pause_requested.clear();  // Clear the flag first
		SPX_ENGINE->pause();
	}

	// Check if resume was requested from a non-main thread
	if (resume_requested.is_set()) {
		resume_requested.clear();  // Clear the flag first
		SPX_ENGINE->resume();
	}

	// Check if next_frame was requested from a non-main thread
	if (next_frame_requested.is_set()) {
		next_frame_requested.clear();  // Clear the flag first
		SPX_ENGINE->next_frame();
	}

	SPX_ENGINE->on_update(delta);
}

void Spx::on_destroy() {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	SPX_ENGINE->on_destroy();
	initialed = false;
	restart_requested.clear();
	reset_requested.clear();
	reset_exit_code.set(0);
	pause_requested.clear();
	resume_requested.clear();
	next_frame_requested.clear();
}

void Spx::reset(int exit_code) {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}


	// Check if we're on the main thread
	if (Thread::is_main_thread()) {
		// If we're on the main thread, reset immediately
		SPX_ENGINE->on_reset(exit_code);
	} else {
		// If we're not on the main thread, set the reset flag
		// Write data first, then set flag to ensure happens-before ordering
		reset_exit_code.set(exit_code);
		reset_requested.set();
	}
}

void Spx::restart() {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	// Check if we're on the main thread
	if (Thread::is_main_thread()) {
		// If we're on the main thread, restart immediately
		SPX_ENGINE->restart();
	} else {
		// If we're not on the main thread, set the restart flag
		restart_requested.set();
	}
}

void Spx::pause() {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	// Check if we're on the main thread
	if (Thread::is_main_thread()) {
		// If we're on the main thread, pause immediately
		SPX_ENGINE->pause();
	} else {
		// If we're not on the main thread, set the pause flag
		pause_requested.set();
	}
}

void Spx::resume() {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	// Check if we're on the main thread
	if (Thread::is_main_thread()) {
		// If we're on the main thread, resume immediately
		SPX_ENGINE->resume();
	} else {
		// If we're not on the main thread, set the resume flag
		resume_requested.set();
	}
}

bool Spx::is_paused() {
	if (!initialed || !SpxEngine::has_initialed()) {
		return false;
	}
	// Query operations are generally safe from any thread
	return SPX_ENGINE->is_paused();
}

void Spx::next_frame() {
	if (!initialed || !SpxEngine::has_initialed()) {
		return;
	}

	// Check if we're on the main thread
	if (Thread::is_main_thread()) {
		// If we're on the main thread, execute next_frame immediately
		SPX_ENGINE->next_frame();
	} else {
		// If we're not on the main thread, set the next_frame flag
		next_frame_requested.set();
	}
}
