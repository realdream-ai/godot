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

class Node;
class SpxInputMgr;
class SpxAudioMgr;
class SpxPhysicMgr;
class SpxSpriteMgr;
class SpxUIMgr;

class SpxEngine : SpxBaseMgr {
	static SpxEngine *singleton;

public:
	static SpxEngine *get_singleton() { return singleton; }
	static bool has_initialed() { return singleton != nullptr; }
	static void register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr);

private:
	SpxInputMgr *input;
	SpxAudioMgr *audio;
	SpxPhysicMgr *physic;
	SpxSpriteMgr *sprite;
	SpxUIMgr *ui;
	Vector<SpxBaseMgr *> mgrs;

public:
	SpxInputMgr *get_input() { return input; }
	SpxAudioMgr *get_audio() { return audio; }
	SpxPhysicMgr *get_physic() { return physic; }
	SpxSpriteMgr *get_sprite() { return sprite; }
	SpxUIMgr *get_ui() { return ui; }

private:
	Node *root_node;
	GdInt global_id;
	SpxCallbackInfo callbacks;

public:
	SpxCallbackInfo* get_callbacks() ;
public:
	GdInt get_unique_id() override;
	Node *get_root_node() override;
	void set_root_node(Node *node) { root_node = node; }

	void on_start() override;
	void on_update(float delta) override;
	void on_destroy() override;
};

#endif // SPX_ENGINE_H
