#include "spx_ui.h"
#include "spx_base_mgr.h"
#include "spx_engine.h"
#include "spx_ui_mgr.h"
#include "scene/gui/label.h"

#define UI_DEFAULT_THEME_NAME "default"


void SpxLabel::_bind_methods() {
}

void SpxLabel::_notification(int p_what) {
	if (p_what == NOTIFICATION_PREDELETE) {
		if (owner != nullptr)
			owner->on_destroy_call();
	}
}

void SpxButton::_bind_methods() {
}

void SpxButton::_notification(int p_what) {
	if (p_what == NOTIFICATION_PREDELETE) {
		if (owner != nullptr)
			owner->on_destroy_call();
	}
}

void SpxImage::_bind_methods() {
}

void SpxImage::_notification(int p_what) {
	if (p_what == NOTIFICATION_PREDELETE) {
		if (owner != nullptr)
			owner->on_destroy_call();
	}
}

void SpxToggle::_bind_methods() {
}

void SpxToggle::_notification(int p_what) {
	if (p_what == NOTIFICATION_PREDELETE) {
		if (owner != nullptr)
			owner->on_destroy_call();
	}
}

#define check_and_get_node_r(VALUE) \
	auto node = get_canvas_item();\
	if (node == nullptr) {\
		print_error("convert ui node error" ); \
		return VALUE; \
	}

#define check_and_get_node_v() \
	auto node = get_canvas_item();\
	if (node == nullptr) {\
		print_error("convert ui node error" ); \
		return ; \
	}

Control *SpxUi::get_canvas_item() const {
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Label:
			return label;
		case ESpxUiType::Button:
			return button;
		case ESpxUiType::Image:
			return image;
		case ESpxUiType::Toggle:
			return toggle;
	}
	print_error("Unknown type " + itos(type));
	return nullptr;
}

void SpxUi::set_canvas_item(Control *control) {
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Label:
			label = (SpxLabel *)(control);
			label->owner = this;
			break;
		case ESpxUiType::Button:
			button = (SpxButton *)(control);
			button->owner = this;
			break;
		case ESpxUiType::Image:
			image = (SpxImage *)(control);
			image->owner = this;
			break;
		case ESpxUiType::Toggle:
			toggle = (SpxToggle *)(control);
			toggle->owner = this;
			break;
	}
}


void SpxUi::on_destroy_call() {
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
	auto node = get_canvas_item();
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
	check_and_get_node_v()
	node->set_rect(rect);
}

GdRect2 SpxUi::get_rect() {
	check_and_get_node_r(GdRect2())
	return node->get_rect();
}

void SpxUi::set_color(GdColor color) {
	check_and_get_node_v()
	node->add_theme_color_override(UI_DEFAULT_THEME_NAME, color);
}

GdColor SpxUi::get_color() {
	check_and_get_node_r(GdColor())
	return node->get_theme_color(UI_DEFAULT_THEME_NAME);
}

void SpxUi::set_font_size(GdInt size) {
	check_and_get_node_v()
	node->add_theme_font_size_override(UI_DEFAULT_THEME_NAME, size);
}

GdInt SpxUi::get_font_size() {
	check_and_get_node_r(0)
	return node->get_theme_font_size(UI_DEFAULT_THEME_NAME);
}

void SpxUi::set_font(GdString path) {
}

GdString SpxUi::get_font() {
	SpxBaseMgr::temp_return_str = "";
	return &SpxBaseMgr::temp_return_str;
}

void SpxUi::set_visible(GdBool visible) {
	check_and_get_node_v()
	node->set_visible(visible);
}

GdBool SpxUi::get_visible() {
	check_and_get_node_r(false)
	return node->is_visible();
}

void SpxUi::set_text(GdString text) {
	String value = SpxStr(text);
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Label:
			label->set_text(value);
			break;
		case ESpxUiType::Button:
			button->set_text(value);
			break;
		case ESpxUiType::Image:
			print_error("ui type Image not support set_text() ");
			break;
		case ESpxUiType::Toggle:
			toggle->set_text(value);
			break;
	}
}

GdString SpxUi::get_text() {
	String value = "";
	auto etype = (ESpxUiType)type;
	switch (etype) {
		case ESpxUiType::Label:
			value = label->get_text();
			break;
		case ESpxUiType::Button:
			value = button->get_text();
			break;
		case ESpxUiType::Image:
			print_error("ui type Image not support get_text() ");
			break;
		case ESpxUiType::Toggle:
			value = toggle->get_text();
			break;
	}
	SpxBaseMgr::temp_return_str = value;
	return &SpxBaseMgr::temp_return_str;
}

void SpxUi::set_texture(GdString path) {
	auto path_str = SpxStr(path);
	Ref<Texture2D> value = ResourceLoader::load(path_str);
	if (value.is_valid()) {
		auto etype = (ESpxUiType)type;
		switch (etype) {
			case ESpxUiType::Label:
				print_error("ui type Label not set_texture set_text() ");
				break;
			case ESpxUiType::Button:
				button->set_icon(value);
				break;
			case ESpxUiType::Image:
				image->set_texture(value);
				break;
			case ESpxUiType::Toggle:
				toggle->set_icon(value);
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
		case ESpxUiType::Label:
			print_error("ui type Label not support get_texture() ");
			break;
		case ESpxUiType::Button:
			value = button->get_icon();
			break;
		case ESpxUiType::Image:
			value = image->get_texture();
			break;
		case ESpxUiType::Toggle:
			value = toggle->get_icon();
			break;
	}
	if (value == nullptr)
		return nullptr;
	SpxBaseMgr::temp_return_str = value->get_name();
	return &SpxBaseMgr::temp_return_str;
}
