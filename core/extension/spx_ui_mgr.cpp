/**************************************************************************/
/*  spx_ui_mgr.cpp                                                        */
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

#include "spx_ui_mgr.h"

#include "scene/main/canvas_layer.h"
#include "scene/resources/packed_scene.h"

#define SPX_CALLBACK SpxEngine::get_singleton()->get_callbacks()
#define check_and_get_node_r(VALUE) \
	auto node = get_node(obj);\
	if (node == nullptr) {\
		print_error("try to get property of a null node gid=" + itos(obj)); \
	return VALUE; \
}

#define check_and_get_node_v() \
	auto node = get_node(obj);\
	if (node == nullptr) {\
		print_error("try to get property of a null node gid=" + itos(obj)); \
	return ; \
}


SpxUi *SpxUIMgr::get_node(GdObj obj) {
	if (id_objects.has(obj)) {
		return id_objects[obj];
	}
	return nullptr;
}

ESpxUiType SpxUIMgr::get_type(Control *obj) {
	if (obj == nullptr) {
		return ESpxUiType::None;
	}
	if (dynamic_cast<SpxLabel *>(obj)) {
		return ESpxUiType::Label;
	}
	if (dynamic_cast<SpxButton *>(obj)) {
		return ESpxUiType::Button;
	}
	if (dynamic_cast<SpxImage *>(obj)) {
		return ESpxUiType::Image;
	}
	if (dynamic_cast<SpxToggle *>(obj)) {
		return ESpxUiType::Toggle;
	}
	return ESpxUiType::None;
}


void SpxUIMgr::on_start() {
	SpxBaseMgr::on_start();
	owner = memnew(CanvasLayer);
	owner->set_name(get_class_name());
	get_root_node()->add_child(owner);
}

void SpxUIMgr::on_node_destroy(SpxUi *node) {
	if (id_objects.erase(node->get_gid())) {
		SPX_CALLBACK->func_on_ui_destroyed(node->get_gid());
	}
}

Control *SpxUIMgr::create_control(GdString path) {
	const String path_str = SpxStr(path);
	Control *node = nullptr;
	if (path_str != "") {
		Ref<PackedScene> scene = ResourceLoader::load(path_str);
		if (scene.is_null()) {
			print_error("Failed to load sprite scene " + path_str);
			return NULL_OBJECT_ID;
		} else {
			node = dynamic_cast<Control *>(scene->instantiate());
			if (node == nullptr) {
				print_error("Failed to load sprite scene , type invalid " + path_str);
			}
		}
	}
	return node;
}



SpxUi *SpxUIMgr::on_create_node(Control *control, GdInt type) {
	SpxUi *node = memnew(SpxUi);
	owner->add_child(control);
	node->set_type(type);
	node->set_canvas_item(control);
	node->set_gid(get_unique_id());
	node->on_start();
	uiMgr->id_objects[node->get_gid()] = node;
	SPX_CALLBACK->func_on_ui_ready(node->get_gid());
	return node;
}

#define CREATE_UI_NODE(TYPE) \
	Spx##TYPE *control = dynamic_cast<Spx##TYPE *>(create_control(path)); \
	if (control == nullptr) { \
		control = memnew(Spx##TYPE); \
	} \
	auto node = on_create_node(control,(GdInt)ESpxUiType::##TYPE);

GdObj SpxUIMgr::create_node(GdString path) {
	auto control = create_control(path);
	if (control == nullptr) {
		print_error("Failed to create node " + SpxStr(path));
		return NULL_OBJECT_ID;
	}
	auto type = get_type(control);
	auto node = on_create_node(control, (GdInt)type);
	return node->get_gid();
}

GdObj SpxUIMgr::create_button(GdString path, GdString text) {
	CREATE_UI_NODE(Button)
	control->set_text(SpxStr(text));
	return node->get_gid();
}

GdObj SpxUIMgr::create_label(GdString path, GdString text) {
	CREATE_UI_NODE(Label)
	control->set_text(SpxStr(text));
	return node->get_gid();
}

GdObj SpxUIMgr::create_image(GdString path) {
	CREATE_UI_NODE(Image)
	return node->get_gid();
}

GdObj SpxUIMgr::create_toggle(GdString path, GdBool value) {
	CREATE_UI_NODE(Toggle)
	return node->get_gid();
}

GdObj SpxUIMgr::create_slider(GdString path, GdFloat value) {
	return 0;
}


GdObj SpxUIMgr::create_input(GdString path, GdString text) {
	return 0;
}

GdBool SpxUIMgr::destroy_node(GdObj obj) {
	check_and_get_node_r(false)
	node->queue_free();
	return true;
}


GdInt SpxUIMgr::get_type(GdObj obj) {
	check_and_get_node_r(0)
	return node->get_type();
}

void SpxUIMgr::set_interactable(GdObj obj, GdBool interactable) {
	check_and_get_node_v()
	node->set_interactable(interactable);
}

GdBool SpxUIMgr::get_interactable(GdObj obj) {
	check_and_get_node_r(false)
	return node->is_interactable();
}

void SpxUIMgr::set_text(GdObj obj, GdString text) {
	check_and_get_node_v()
	node->set_text(text);
}

GdString SpxUIMgr::get_text(GdObj obj) {
	check_and_get_node_r(GdString())
	return node->get_text();
}

void SpxUIMgr::set_rect(GdObj obj, GdRect2 rect) {
	check_and_get_node_v()
	node->set_rect(rect);
}

GdRect2 SpxUIMgr::get_rect(GdObj obj) {
	check_and_get_node_r(GdRect2())
	return node->get_rect();
}

void SpxUIMgr::set_color(GdObj obj, GdColor color) {
	check_and_get_node_v()
	node->set_color(color);
}

GdColor SpxUIMgr::get_color(GdObj obj) {
	check_and_get_node_r(GdColor())
	return node->get_color();
}

void SpxUIMgr::set_font_size(GdObj obj, GdInt size) {
	check_and_get_node_v()
	node->set_font_size(size);
}

GdInt SpxUIMgr::get_font_size(GdObj obj) {
	check_and_get_node_r(0)
	return node->get_font_size();
}

void SpxUIMgr::set_visible(GdObj obj, GdBool visible) {
	check_and_get_node_v()
	node->set_visible(visible);
}

GdBool SpxUIMgr::get_visible(GdObj obj) {
	check_and_get_node_r(false)
	return node->get_visible();
}

void SpxUIMgr::set_texture(GdObj obj, GdString path) {
	check_and_get_node_v()
	node->set_texture(path);
}

GdString SpxUIMgr::get_texture(GdObj obj) {
	check_and_get_node_r(nullptr)
	return node->get_texture();
}
