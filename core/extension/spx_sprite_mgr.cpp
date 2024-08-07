/**************************************************************************/
/*  spx_sprite_mgr.cpp                                                    */
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

#include "spx_sprite_mgr.h"

#include "scene/main/node.h"
#include "spx_engine.h"
#include "spx_sprite.h"

#include "core/templates/rb_map.h"

void SpxSpriteMgr::on_start() {
	SpxBaseMgr::on_start();
}

void SpxSpriteMgr::on_destroy() {
	SpxBaseMgr::on_destroy();
}

void SpxSpriteMgr::on_update(float delta) {
	SpxBaseMgr::on_update(delta);
}

SpxSprite *SpxSpriteMgr::get_sprite(GdObj obj) {
	if (id_sprites.has(obj)) {
		return id_sprites[obj];
	}
	return nullptr;
}

void SpxSpriteMgr::on_sprite_destroy(SpxSprite *sprite) {
	id_sprites.erase(sprite->get_id());
}

// sprite
GdInt SpxSpriteMgr::create_sprite(GdString path) {
	const String path_str = String(*(const String *)path);
	SpxSprite *new_node = memnew(SpxSprite);
	auto id = get_unique_id();
	new_node->set_name(path_str);
	new_node->set_id(id);
	owner->add_child(new_node);
	print_line("cpp SpxSprite::create_sprite " + path_str);
	return id;
}

GdInt SpxSpriteMgr::clone_sprite(GdObj id) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		return NULL_SPRITE_ID;
	}
	auto new_node = (SpxSprite *)sprite->duplicate();
	new_node->set_id(get_unique_id());
	owner->add_child(new_node);

	return new_node->get_id();
}

GdBool SpxSpriteMgr::destroy_sprite(GdObj id) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		return false;
	}
	sprite->queue_free();
	return true;
}

GdBool SpxSpriteMgr::is_sprite_alive(GdObj id) {
	return get_sprite(id) != nullptr;
}

void SpxSpriteMgr::set_position(GdObj id, GdVec2 pos) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		return;
	}
	sprite->set_position(pos);
	print_line(vformat("Updating position of %d to %f, %f", id, pos.x, pos.y));
}

void SpxSpriteMgr::set_rotation(GdObj id, float rot) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		return;
	}
	sprite->set_rotation(rot);
	print_line(vformat("Updating rotation of %d to %f", id, rot));
}

void SpxSpriteMgr::set_scale(GdObj id, GdVec2 scale) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		return;
	}
	sprite->set_scale(scale);
	print_line(vformat("Updating scale of %d to %f, %f", id, scale.x, scale.y));
}

GdVec2 SpxSpriteMgr::get_position(GdObj id) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		print_error("try to get position of a null sprite" + itos(id));
		return GdVec2();
	}
	return sprite->get_position();
}

GdFloat SpxSpriteMgr::get_rotation(GdObj id) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		print_error("try to get rotation of a null sprite" + itos(id));
		return 0;
	}
	return sprite->get_rotation();
}

GdVec2 SpxSpriteMgr::get_scale(GdObj id) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		print_error("try to get scale of a null sprite" + itos(id));
		return GdVec2();
	}
	return sprite->get_scale();
}

void SpxSpriteMgr::set_color(GdObj id, GdColor color) {
	print_line(vformat("TODO set_color of sprite Updating GdColor of %d to %f, %f, %f, %f", id, color.r, color.g, color.b, color.a));
}

GdColor SpxSpriteMgr::get_color(GdObj id) {
	print_line("TODO get_color of sprite");
	return GdColor();
}

void SpxSpriteMgr::update_texture(GdObj id, GdString path) {
	print_line("TODO update_texture of sprite");
}

GdString SpxSpriteMgr::get_texture(GdObj id) {
	print_line("TODO get_texture of sprite");
	return nullptr;
}

void SpxSpriteMgr::set_visible(GdObj id, GdBool visible) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		print_error("try to set_visible of a null sprite" + itos(id));
		return;
	}
	sprite->set_visible(visible);
}

GdBool SpxSpriteMgr::get_visible(GdObj id) {
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		print_error("try to get_visible of a null sprite" + itos(id));
		return false;
	}
	return sprite->is_visible();
}

void SpxSpriteMgr::update_z_index(GdObj id, GdInt z) {
	print_line(vformat("Updating z index of %d to %d", id, z));
	auto sprite = get_sprite(id);
	if (sprite == nullptr) {
		print_error("try to update_z_index of a null sprite" + itos(id));
		return;
	}
	sprite->set_z_index(z);
}
