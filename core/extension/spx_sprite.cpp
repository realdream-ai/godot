/**************************************************************************/
/*  spx_sprite.cpp                                                        */
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

#include "spx_sprite.h"

#include "spx_engine.h"
#include "spx_sprite_mgr.h"
#include "scene/2d/area_2d.h"

void SpxSprite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "id"), &SpxSprite::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &SpxSprite::get_id);
	ClassDB::bind_method(D_METHOD("on_destroy_call"), &SpxSprite::on_destroy_call);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "obj_id"), "set_id", "get_id");
	ClassDB::bind_method(D_METHOD("on_area_entered", "area"), &SpxSprite::on_area_entered);
	ClassDB::bind_method(D_METHOD("on_area_exited", "area"), &SpxSprite::on_area_exited);

}

void SpxSprite::set_id(int64_t id) {
	this->obj_id = id;
}

int64_t SpxSprite::get_id() {
	return obj_id;
}

void SpxSprite::on_destroy_call() {
	spriteMgr->on_sprite_destroy(this);
}

SpxSprite::SpxSprite() {
}

SpxSprite::~SpxSprite() {
}

void SpxSprite::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PREDELETE: {
			on_destroy_call();
			break;
		}
		case NOTIFICATION_READY: {
			//set_process(true);
			break;
		}
		default:
			break;
	}
}

void SpxSprite::on_start() {
	if (area) {
		area->connect("area_entered", Callable(this, "on_area_entered"));
		area->connect("area_exited", Callable(this, "on_area_exited"));
	}
}

void SpxSprite::on_area_entered(Node *node) {
	print_line("on_area_entered " + node->get_name());
}

void SpxSprite::on_area_exited(Node *node) {
	print_line("on_area_exited " + node->get_name());
}
