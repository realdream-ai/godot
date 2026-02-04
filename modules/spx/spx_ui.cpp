/**************************************************************************/
/*  spx_ui.cpp                                                            */
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

#include "spx_ui.h"

#include "scene/gui/label.h"
#include "spx.h"
#include "spx_base_mgr.h"
#include "spx_engine.h"
#include "spx_ui_mgr.h"

#define UI_DEFAULT_THEME_NAME "default"

// Internal control validation macros
#define CHECK_AND_GET_NODE_R(VALUE)                                                    \
	auto node = get_control_item();                                                    \
	if (node == nullptr) {                                                             \
		print_error(String("SpxUi::") + String(__func__) + ": control is nullptr");   \
		return VALUE;                                                                  \
	}

#define CHECK_AND_GET_NODE_V()                                                         \
	auto node = get_control_item();                                                    \
	if (node == nullptr) {                                                             \
		print_error(String("SpxUi::") + String(__func__) + ": control is nullptr");   \
		return;                                                                        \
	}

#define GET_SPX_CONTROL_TYPE(VALUE)                                                    \
	if (type != (int)ESpxUiType::VALUE) {                                              \
		print_error(String("SpxUi type mismatch in ") + String(__func__) +            \
		           ": expected " #VALUE ", got " + itos(type));                        \
		return nullptr;                                                                \
	}                                                                                  \
	return (Spx##VALUE *)control;

Control *SpxUi::get_control_item() const {
	return control;
}

void SpxUi::set_control_item(Control *ctrl) {
	ctrl->spx_owner = this;
	this->control = ctrl;
}

void SpxUi::on_click_internal() {
	uiMgr->on_click(this);
}

SpxControl *SpxUi::get_control() {
	return control;
}
SpxLabel *SpxUi::get_label(){
	GET_SPX_CONTROL_TYPE(Label)
}
SpxImage *SpxUi::get_image(){
	GET_SPX_CONTROL_TYPE(Image)
}
SpxButton *SpxUi::get_button(){
	GET_SPX_CONTROL_TYPE(Button)
}
SpxToggle *SpxUi::get_toggle() {
	GET_SPX_CONTROL_TYPE(Toggle)
}
SpxInput *SpxUi::get_input() {
	GET_SPX_CONTROL_TYPE(Input)
}


void SpxUi::on_destroy_call() {
	if (!Spx::initialed)
		return;
	uiMgr->on_node_destroy(this);
}

void SpxUi::on_start() {
}

void SpxUi::set_type(GdInt etype) {
	type = etype;
}

void SpxUi::set_gid(GdObj id) {
	gid = id;
}

GdObj SpxUi::get_gid() {
	return gid;
}

GdInt SpxUi::get_type() {
	return type;
}

void SpxUi::queue_free() {
	auto node = get_control_item();
	if (node != nullptr)
		node->queue_free();
}

void SpxUi::set_interactable(GdBool interactable) {
	print_line("TODO SpxUi::set_interactable()");
	return;
}

GdBool SpxUi::is_interactable() {
	return true;
}

void SpxUi::set_rect(GdRect2 rect) {
	CHECK_AND_GET_NODE_V()
			node->set_rect(rect);
}

GdRect2 SpxUi::get_rect() {
	CHECK_AND_GET_NODE_R(GdRect2()) return node->get_rect();
}

void SpxUi::set_color(GdColor color) {
	CHECK_AND_GET_NODE_V()
	node->set_self_modulate(color);
}

GdColor SpxUi::get_color() {
	CHECK_AND_GET_NODE_R(GdColor())
	return node->get_self_modulate();
}

void SpxUi::set_font_size(GdInt size) {
	CHECK_AND_GET_NODE_V()
			node->add_theme_font_size_override(UI_DEFAULT_THEME_NAME, size);
}

GdInt SpxUi::get_font_size() {
	CHECK_AND_GET_NODE_R(0) return node->get_theme_font_size(UI_DEFAULT_THEME_NAME);
}

void SpxUi::set_font(GdString path) {
}

GdString SpxUi::get_font() {
	return SpxReturnStr("");
}

void SpxUi::set_visible(GdBool visible) {
	CHECK_AND_GET_NODE_V()
			node->set_visible(visible);
}

GdBool SpxUi::get_visible() {
	CHECK_AND_GET_NODE_R(false) return node->is_visible();
}

void SpxUi::set_text(GdString text) {
	String value = SpxStr(text);
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Label:
			get_label()->set_text(value);
			break;
		case ESpxUiType::Button:
			get_button()->set_text(value);
			break;
		case ESpxUiType::Toggle:
			get_toggle()->set_text(value);
			break;
		case ESpxUiType::Input:
			get_input()->set_text(value);
			break;
		default:
			print_error("not support set_text() type " + itos(type));
			break;
	}
}

GdString SpxUi::get_text() {
	String value = "";
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Label:
			value = get_label()->get_text();
			break;
		case ESpxUiType::Button:
			value = get_button()->get_text();
			break;
		case ESpxUiType::Toggle:
			value = get_toggle()->get_text();
			break;
		case ESpxUiType::Input:
			value = get_input()->get_text();
			break;
		default:
			print_error("not support get_text() type " + itos(type));
			break;
	}
	return SpxReturnStr(value);
}

void SpxUi::set_texture(GdString path) {
	auto path_str = SpxStr(path);
	Ref<Texture2D> value = ResourceLoader::load(path_str);
	if (value.is_valid()) {
		auto etype = (ESpxUiType)type;
		switch (etype) {
			case ESpxUiType::Button:
				get_button()->set_button_icon(value);
				break;
			case ESpxUiType::Image:
				get_image()->set_texture(value);
				break;
			case ESpxUiType::Toggle:
				get_toggle()->set_button_icon(value);
				break;
			default:
				print_error("not support set_icon() type " + itos(type));
				break;
		}
	} else {
		print_error("can not find a texture: " + path_str);
	}
}

GdString SpxUi::get_texture() {
	Ref<Texture2D> value = nullptr;
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Button:
			value = get_button()->get_button_icon();
			break;
		case ESpxUiType::Image:
			value = get_image()->get_texture();
			break;
		case ESpxUiType::Toggle:
			value = get_toggle()->get_button_icon();
			break;
		default:
			print_error("not support get_texture() type " + itos(type));
			break;
	}
	if (value == nullptr)
		return nullptr;
	return SpxReturnStr(value->get_name());
}

GdInt SpxUi::get_layout_direction() {
	return get_control()->get_layout_direction();
}
void SpxUi::set_layout_direction(GdInt value) {
	get_control()->set_layout_direction((Control::LayoutDirection)value);
}
GdInt SpxUi::get_layout_mode() {
	return get_control()->get_layout_mode();
}
void SpxUi::set_layout_mode(GdInt value) {
	get_control()->set_layout_mode((Control::LayoutMode)value);
}
GdInt SpxUi::get_anchors_preset() {
	return get_control()->get_anchors_preset();
}
void SpxUi::set_anchors_preset(GdInt value) {
	return get_control()->set_anchors_preset((Control::LayoutPreset)value);
}
GdVec2 SpxUi::get_scale() {
	return get_control()->get_scale();
}
void SpxUi::set_scale(GdVec2 value) {
	return get_control()->set_scale(value);
}

GdVec2 SpxUi::get_size() {
	return get_control()->get_size();
}
void SpxUi::set_size(GdVec2 value) {
	return get_control()->set_size(value);
}

GdVec2 SpxUi::get_position() {
	return get_control()->get_position();
}
void SpxUi::set_position(GdVec2 value) {
	return get_control()->set_position(value);
}

GdVec2 SpxUi::get_global_position() {
	return get_control()->get_global_position();
}
void SpxUi::set_global_position(GdVec2 value) {
	return get_control()->set_global_position(value);
}

GdFloat SpxUi::get_rotation() {
	return get_control()->get_rotation();
}
void SpxUi::set_rotation(GdFloat value) {
	return get_control()->set_rotation(value);
}
GdBool SpxUi::get_flip(GdBool horizontal) {
	auto image = get_image();
	return horizontal ? image->is_flipped_h() : image->is_flipped_v();
}
void SpxUi::set_flip(GdBool horizontal, GdBool is_flip) {
	auto image = get_image();
	if (horizontal) {
		image->set_flip_h(is_flip);
	} else {
		image->set_flip_v(is_flip);
	}
}
