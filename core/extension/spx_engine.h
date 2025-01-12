/**************************************************************************/
/*  spx_engine.h                                                          */
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

#ifndef SPX_ENGINE_H
#define SPX_ENGINE_H

#include "gdextension_spx_ext.h"
#include "spx_base_mgr.h"

#include <cstdint>
class SceneTree;
class Window;
class Node;
class SpxInputMgr;
class SpxAudioMgr;
class SpxPhysicMgr;
class SpxSpriteMgr;
class SpxUiMgr;
class SpxSceneMgr;
class SpxCameraMgr;
class SpxPlatformMgr;
class SpxResMgr;

class SpxEngine : SpxBaseMgr {
	static SpxEngine *singleton;

public:
	static SpxEngine *get_singleton() { return singleton; }
	static bool has_initialed() { return singleton != nullptr; }
	static void register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr);

private:
	Vector<SpxBaseMgr *> mgrs;
	SpxInputMgr *input;
	SpxAudioMgr *audio;
	SpxPhysicMgr *physic;
	SpxSpriteMgr *sprite;
	SpxUiMgr *ui;
	SpxSceneMgr *scene;
	SpxCameraMgr *camera;
	SpxPlatformMgr *platform;
	SpxResMgr *res;

public:
	SpxInputMgr *get_input() { return input; }
	SpxAudioMgr *get_audio() { return audio; }
	SpxPhysicMgr *get_physic() { return physic; }
	SpxSpriteMgr *get_sprite() { return sprite; }
	SpxUiMgr *get_ui() { return ui; }
	SpxSceneMgr *get_scene() { return scene; }
	SpxCameraMgr *get_camera() { return camera; }
	SpxPlatformMgr *get_platform() { return platform; }
	SpxResMgr *get_res() { return res; }

private:
	SceneTree *tree;
	Node *spx_root;
	GdInt global_id;
	SpxCallbackInfo callbacks;

public:
	SpxCallbackInfo *get_callbacks();

public:
	GdInt get_unique_id() override;
	Node *get_spx_root() override;
	SceneTree *get_tree() override;
	Window *get_root();
	void set_root_node(SceneTree *p_tree, Node *p_node);

	void on_awake() override;
	void on_fixed_update(float delta) override;
	void on_update(float delta) override;
	void on_destroy() override;
};

#endif // SPX_ENGINE_H
