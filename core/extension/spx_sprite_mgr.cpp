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
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/area_2d.h"
#include "scene/2d/collision_shape_2d.h"
#include "scene/2d/physics_body_2d.h"
#include "scene/resources/circle_shape_2d.h"
#include "scene/resources/packed_scene.h"

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
	SpxSprite *sprite = nullptr;
	if (path_str == "") {
		sprite = memnew(SpxSprite);
		AnimatedSprite2D *animated_sprite = memnew(AnimatedSprite2D);
		sprite->animation = animated_sprite;
		sprite->add_child(animated_sprite);
		Area2D *area = memnew(Area2D);
		sprite->add_child(area);
		CollisionShape2D *area_collision_shape = memnew(CollisionShape2D);
		const Ref<CircleShape2D> area_shape = memnew(CircleShape2D);
		area_shape->set_radius(50.0f);
		area_collision_shape->set_shape(area_shape);
		area->add_child(area_collision_shape);
		sprite->area = area;
		CollisionShape2D *body_collision_shape = memnew(CollisionShape2D);
		const Ref<CircleShape2D> body_shape = memnew(CircleShape2D);
		body_shape->set_radius(50.0f);
		body_collision_shape->set_shape(body_shape);
		sprite->add_child(body_collision_shape);
		sprite->collider = body_collision_shape;
		Node2D *shooting_point = memnew(Node2D);
		shooting_point->set_name("ShootingPoint");
		sprite->add_child(shooting_point);
		sprite->on_start();
	} else {
		// load from path
		Ref<PackedScene> scene = ResourceLoader::load(SpxStr(path));
		if (scene.is_null()) {
			print_error("Failed to load scene.");
			return NULL_SPRITE_ID;
		} else {
			sprite = (SpxSprite*)scene->instantiate();
		}
	}
	auto id = get_unique_id();
	sprite->set_id(id);
	get_root_node()->add_child(sprite);
	return id;
}

GdInt SpxSpriteMgr::clone_sprite(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		return NULL_SPRITE_ID;
	}
	auto new_node = (SpxSprite *)sprite->duplicate();
	new_node->set_id(get_unique_id());
	owner->add_child(new_node);

	return new_node->get_id();
}

GdBool SpxSpriteMgr::destroy_sprite(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		return false;
	}
	sprite->queue_free();
	return true;
}

GdBool SpxSpriteMgr::is_sprite_alive(GdObj obj) {
	return get_sprite(obj) != nullptr;
}

void SpxSpriteMgr::set_position(GdObj obj, GdVec2 pos) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		return;
	}
	sprite->set_position(pos);
	print_line(vformat("Updating position of %d to %f, %f", obj, pos.x, pos.y));
}

void SpxSpriteMgr::set_rotation(GdObj obj, float rot) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		return;
	}
	sprite->set_rotation(rot);
	print_line(vformat("Updating rotation of %d to %f", obj, rot));
}

void SpxSpriteMgr::set_scale(GdObj obj, GdVec2 scale) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		return;
	}
	sprite->set_scale(scale);
	print_line(vformat("Updating scale of %d to %f, %f", obj, scale.x, scale.y));
}

GdVec2 SpxSpriteMgr::get_position(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get position of a null sprite" + itos(obj));
		return GdVec2();
	}
	return sprite->get_position();
}

GdFloat SpxSpriteMgr::get_rotation(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get rotation of a null sprite" + itos(obj));
		return 0;
	}
	return sprite->get_rotation();
}

GdVec2 SpxSpriteMgr::get_scale(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get scale of a null sprite" + itos(obj));
		return GdVec2();
	}
	return sprite->get_scale();
}

void SpxSpriteMgr::set_color(GdObj obj, GdColor color) {
	print_line(vformat("TODO set_color of sprite Updating GdColor of %d to %f, %f, %f, %f", obj, color.r, color.g, color.b, color.a));
}

GdColor SpxSpriteMgr::get_color(GdObj obj) {
	print_line("TODO get_color of sprite");
	return GdColor();
}

void SpxSpriteMgr::update_texture(GdObj obj, GdString path) {
	print_line("TODO update_texture of sprite");
}

GdString SpxSpriteMgr::get_texture(GdObj obj) {
	print_line("TODO get_texture of sprite");
	return nullptr;
}

void SpxSpriteMgr::set_visible(GdObj obj, GdBool visible) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to set_visible of a null sprite" + itos(obj));
		return;
	}
	sprite->set_visible(visible);
}

GdBool SpxSpriteMgr::get_visible(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get_visible of a null sprite" + itos(obj));
		return false;
	}
	return sprite->is_visible();
}

void SpxSpriteMgr::update_z_index(GdObj obj, GdInt z) {
	print_line(vformat("Updating z index of %d to %d", obj, z));
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to update_z_index of a null sprite" + itos(obj));
		return;
	}
	sprite->set_z_index(z);
}
