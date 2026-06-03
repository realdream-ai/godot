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

#include "core/math/math_funcs.h"
#include "core/templates/rb_map.h"
#include "core/typedefs.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/packed_scene.h"

#include "spx_engine.h"
#include "spx_ext_mgr.h"
#include "spx_layer_sorter.h"
#include "spx_object_guard.h"
#include "spx_physics_mgr.h"
#include "spx_res_mgr.h"
#include "spx_scene_mgr.h"
#include "spx_sprite.h"

#include <cstdint>
#include <cstring>

#define DEFAULT_COLLISION_ALPHA_THRESHOLD 0.05

enum SpxPhysicsBatchCmd {
	SPX_PHYSICS_CMD_VELOCITY = 1,
	SPX_PHYSICS_CMD_GRAVITY = 2,
	SPX_PHYSICS_CMD_MASS = 3,
	SPX_PHYSICS_CMD_MODE = 4,
	SPX_PHYSICS_CMD_USE_GRAVITY = 5,
	SPX_PHYSICS_CMD_GRAVITY_SCALE = 6,
	SPX_PHYSICS_CMD_DRAG = 7,
	SPX_PHYSICS_CMD_FRICTION = 8,
	SPX_PHYSICS_CMD_COLLISION_LAYER = 9,
	SPX_PHYSICS_CMD_COLLISION_MASK = 10,
	SPX_PHYSICS_CMD_TRIGGER_LAYER = 11,
	SPX_PHYSICS_CMD_TRIGGER_MASK = 12,
	SPX_PHYSICS_CMD_COLLISION_ENABLED = 13,
	SPX_PHYSICS_CMD_TRIGGER_ENABLED = 14,
};

static constexpr int SPX_PHYSICS_BATCH_FIELDS = 6;

StringName SpxSpriteMgr::default_texture_anim;

// Refactored sprite validation using unified SpxObjectGuard (RAII pattern)
// See spx_object_guard.h for details
#define SPX_REQUIRE_SPRITE_VOID() \
	SPX_SPRITE_GUARD_VOID(obj, __func__)

#define SPX_REQUIRE_SPRITE_RETURN(VALUE) \
	SPX_SPRITE_GUARD_RETURN(obj, __func__, VALUE)

#define SPX_REQUIRE_TARGET_SPRITE_VOID(TARGET) \
	SPX_TARGET_SPRITE_GUARD_VOID(TARGET, __func__)

#define SPX_REQUIRE_TARGET_SPRITE_RETURN(TARGET, VALUE) \
	SPX_TARGET_SPRITE_GUARD_RETURN(TARGET, __func__, VALUE)

static _FORCE_INLINE_ GdFloat color_distance_squared(const GdColor &p_a, const GdColor &p_b) {
	const GdFloat dr = p_a.r - p_b.r;
	const GdFloat dg = p_a.g - p_b.g;
	const GdFloat db = p_a.b - p_b.b;
	const GdFloat da = p_a.a - p_b.a;
	return dr * dr + dg * dg + db * db + da * da;
}

static _FORCE_INLINE_ Ref<Texture2D> get_current_frame_texture(AnimatedSprite2D *p_anim2d) {
	if (!p_anim2d) {
		return Ref<Texture2D>();
	}

	Ref<SpriteFrames> frames = p_anim2d->get_sprite_frames();
	if (frames.is_null()) {
		return Ref<Texture2D>();
	}

	const StringName current_animation = p_anim2d->get_animation();
	const int current_frame = p_anim2d->get_frame();
	if (!frames->has_animation(current_animation)) {
		return Ref<Texture2D>();
	}

	return frames->get_frame_texture(current_animation, current_frame);
}

static _FORCE_INLINE_ Rect2 get_anim_local_rect(AnimatedSprite2D *p_anim2d, const Vector2 &p_texture_size) {
	Vector2 ofs = p_anim2d->get_offset();
	if (p_anim2d->is_centered()) {
		ofs -= p_texture_size / 2.0;
	}
	return Rect2(ofs, p_texture_size);
}

static _FORCE_INLINE_ Rect2 get_transformed_rect_aabb(const Transform2D &p_transform, const Rect2 &p_local_rect) {
	const Vector2 top_left = p_transform.xform(p_local_rect.position);
	const Vector2 top_right = p_transform.xform(p_local_rect.position + Vector2(p_local_rect.size.x, 0));
	const Vector2 bottom_left = p_transform.xform(p_local_rect.position + Vector2(0, p_local_rect.size.y));
	const Vector2 bottom_right = p_transform.xform(p_local_rect.position + p_local_rect.size);

	const float min_x = MIN(MIN(top_left.x, top_right.x), MIN(bottom_left.x, bottom_right.x));
	const float max_x = MAX(MAX(top_left.x, top_right.x), MAX(bottom_left.x, bottom_right.x));
	const float min_y = MIN(MIN(top_left.y, top_right.y), MIN(bottom_left.y, bottom_right.y));
	const float max_y = MAX(MAX(top_left.y, top_right.y), MAX(bottom_left.y, bottom_right.y));

	return Rect2(Vector2(min_x, min_y), Vector2(max_x - min_x, max_y - min_y));
}

struct PixelCollisionQuery {
	Ref<Texture2D> texture;
	Ref<Image> image;
	Rect2 bounds;
	Rect2 local_rect;
	Transform2D inverse_transform;
	Vector2i image_size;
	bool flip_h = false;
	bool flip_v = false;
};

static _FORCE_INLINE_ Rect2i snap_rect_to_pixel_rect(const Rect2 &p_rect) {
	const Vector2i begin(
			(int)Math::ceil(p_rect.position.x - 0.5f),
			(int)Math::ceil(p_rect.position.y - 0.5f));
	const Vector2i end(
			(int)Math::floor(p_rect.position.x + p_rect.size.x - 0.5f) + 1,
			(int)Math::floor(p_rect.position.y + p_rect.size.y - 0.5f) + 1);

	return Rect2i(begin, Vector2i(MAX(end.x - begin.x, 0), MAX(end.y - begin.y, 0)));
}

static _FORCE_INLINE_ Rect2i get_pixel_overlap_rect(const Rect2 &p_a, const Rect2 &p_b) {
	return snap_rect_to_pixel_rect(p_a).intersection(snap_rect_to_pixel_rect(p_b));
}

static _FORCE_INLINE_ bool read_image_pixel(const Ref<Image> &p_image, const Vector2i &p_image_size, const Vector2 &p_local_pos, Color &r_color) {
	const int px = (int)Math::floor(p_local_pos.x);
	const int py = (int)Math::floor(p_local_pos.y);
	if (px < 0 || px >= p_image_size.x || py < 0 || py >= p_image_size.y) {
		return false;
	}

	r_color = p_image->get_pixel(px, py);
	return true;
}

static _FORCE_INLINE_ bool build_pixel_collision_query(AnimatedSprite2D *p_anim2d, PixelCollisionQuery &r_query) {
	r_query.texture = get_current_frame_texture(p_anim2d);
	if (r_query.texture.is_null()) {
		return false;
	}

	const Vector2 texture_size = r_query.texture->get_size();
	r_query.local_rect = get_anim_local_rect(p_anim2d, texture_size);
	r_query.bounds = get_transformed_rect_aabb(p_anim2d->get_global_transform(), r_query.local_rect);
	r_query.inverse_transform = p_anim2d->get_global_transform().affine_inverse();
	r_query.image_size = Vector2i((int)texture_size.x, (int)texture_size.y);
	r_query.flip_h = p_anim2d->is_flipped_h();
	r_query.flip_v = p_anim2d->is_flipped_v();
	return true;
}

static _FORCE_INLINE_ bool ensure_query_image(PixelCollisionQuery &r_query) {
	if (r_query.image.is_valid()) {
		return true;
	}

	r_query.image = r_query.texture->get_image();
	if (r_query.image.is_null()) {
		return false;
	}

	r_query.image_size = r_query.image->get_size();
	return true;
}

static _FORCE_INLINE_ Vector2 to_image_coord(const PixelCollisionQuery &p_query, const Vector2 &p_world_pos) {
	Vector2 image_pos = p_query.inverse_transform.xform(p_world_pos) - p_query.local_rect.position;
	if (p_query.flip_h) {
		image_pos.x = (real_t)p_query.image_size.x - image_pos.x;
	}
	if (p_query.flip_v) {
		image_pos.y = (real_t)p_query.image_size.y - image_pos.y;
	}
	return image_pos;
}

static _FORCE_INLINE_ bool read_query_pixel(
		const PixelCollisionQuery &p_query,
		const Vector2 &p_world_pos,
		Color &r_color) {
	const Vector2 local_pos = to_image_coord(p_query, p_world_pos);
	return read_image_pixel(p_query.image, p_query.image_size, local_pos, r_color);
}

void SpxSpriteMgr::on_awake() {
	SpxBaseMgr::on_awake();
	default_texture_anim = "default";

	// Initialize pixel collision sampling step with default value of 2 (good balance between performance and accuracy)
	pixel_collision_sampling_step = 2;

	dont_destroy_root = memnew(Node2D);
	dont_destroy_root->set_name("dont_destroy_root");
	get_spx_root()->add_child(dont_destroy_root);

	sprite_root = memnew(Node2D);
	sprite_root->set_name("sprite_root");
	get_spx_root()->add_child(sprite_root);
}

void SpxSpriteMgr::on_start() {
	SpxBaseMgr::on_start();
	auto nodes = get_root()->find_children("*", "SpxSprite", true, false);
	for (int i = 0; i < nodes.size(); i++) {
		auto sprite = Object::cast_to<SpxSprite>(nodes[i]);
		if (sprite != nullptr) {
			sprite->set_gid(get_unique_id());
			sprite->on_start();
			spriteMgr->id_objects[sprite->get_gid()] = sprite;
			auto value = sprite->get_spx_type_name();
			auto data = SpxReturnStr(value);
			SPX_CALLBACK->func_on_scene_sprite_instantiated(sprite->get_gid(), data);
		}
	}
}

void SpxSpriteMgr::on_destroy() {
	SpxBaseMgr::on_destroy();
}

void SpxSpriteMgr::on_update(float delta) {
	SpxBaseMgr::on_update(delta);
	_check_pixel_collision_events();

	Vector<ISortableSprite *> all_sortables;

	for (auto &pair : id_objects) {
		if (pair.value) {
			all_sortables.push_back(pair.value);
		}
	}

	sceneMgr->collect_sortable_sprites(all_sortables);
	SpxLayerSorter::instance().update(all_sortables);
}

void SpxSpriteMgr::on_reset(int reset_code) {
	default_texture_anim = "default";
	dont_destroy_root->queue_free();
	dont_destroy_root = memnew(Node2D);
	dont_destroy_root->set_name("dont_destroy_root");
	get_spx_root()->add_child(dont_destroy_root);

	destroy_all_sprites();
}

void SpxSpriteMgr::collect_sortable_sprites(Vector<ISortableSprite *> &out) {
	for (auto &pair : id_objects) {
		if (pair.value) {
			out.push_back(pair.value);
		}
	}
}

SpxSprite *SpxSpriteMgr::get_sprite(GdObj obj) {
	// Use single-lookup pattern: find() returns Element*, avoiding double hash lookup
	auto element = id_objects.find(obj);
	if (element != nullptr) {
		return element->value();
	}
	return nullptr;
}

void SpxSpriteMgr::on_sprite_destroy(SpxSprite *sprite) {
	_remove_collision_pairs_for_sprite(sprite->get_gid());
	if (id_objects.erase(sprite->get_gid())) {
		SPX_CALLBACK->func_on_sprite_destroyed(sprite->get_gid());
	}
}

void SpxSpriteMgr::set_dont_destroy_on_load(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite.get()->get_parent()->remove_child(sprite.get());
	dont_destroy_root->add_child(sprite.get());
}

void SpxSpriteMgr::set_process(GdObj obj, GdBool is_on) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_process(is_on);
}

void SpxSpriteMgr::set_physic_process(GdObj obj, GdBool is_on) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_physics_process(is_on);
}
void SpxSpriteMgr::set_type_name(GdObj obj, GdString type_name) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_type_name(type_name);
}

void SpxSpriteMgr::set_child_position(GdObj obj, GdString path, GdVec2 pos) {
	SPX_REQUIRE_SPRITE_VOID()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_position(GdVec2{ pos.x, -pos.y });
	}
}

GdVec2 SpxSpriteMgr::get_child_position(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		auto pos = child->get_position();
		return GdVec2{ pos.x, -pos.y };
	}
	return GdVec2();
}

void SpxSpriteMgr::set_child_rotation(GdObj obj, GdString path, GdFloat rot) {
	SPX_REQUIRE_SPRITE_VOID()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_rotation(rot);
	}
}

GdFloat SpxSpriteMgr::get_child_rotation(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		return child->get_rotation();
	}
	return 0;
}

void SpxSpriteMgr::set_child_scale(GdObj obj, GdString path, GdVec2 scale) {
	SPX_REQUIRE_SPRITE_VOID()
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		child->set_scale(scale);
	}
}

GdVec2 SpxSpriteMgr::get_child_scale(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto child = (Node2D *)sprite->get_node(SpxStr(path));
	if (child != nullptr) {
		return child->get_scale();
	}
	return GdVec2();
}

GdBool SpxSpriteMgr::check_collision(GdObj obj, GdObj target, GdBool is_src_trigger, GdBool is_dst_trigger) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	SPX_REQUIRE_TARGET_SPRITE_RETURN(target, false)
	return sprite->check_collision(sprite_target.get(), is_src_trigger, is_dst_trigger);
}

GdBool SpxSpriteMgr::check_collision_with_point(GdObj obj, GdVec2 point, GdBool is_trigger) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	point.y = -point.y;
	return sprite->check_collision_with_point(point, is_trigger);
}

void SpxSpriteMgr::set_debug_collision_visible(GdObj obj, GdBool visible) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_debug_collision_visible(visible);
}

GdBool SpxSpriteMgr::is_debug_collision_visible(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_debug_collision_visible();
}

GdInt SpxSpriteMgr::create_backdrop(GdString path) {
	return _create_sprite(path, GdVec2(), true);
}

GdInt SpxSpriteMgr::create_sprite(GdString path, GdVec2 pos) {
	return _create_sprite(path, pos, false);
}

GdObj SpxSpriteMgr::create_bare_sprite(GdVec2 pos) {
	return _create_sprite("", pos, false);
}

// sprite
GdInt SpxSpriteMgr::_create_sprite(GdString path, GdVec2 pos, GdBool is_backdrop) {
	const String path_str = SpxStr(path);
	SpxSprite *sprite = nullptr;
	if (path_str == "") {
		sprite = memnew(SpxSprite);
		sprite->set_position(GdVec2(pos.x, -pos.y));
		AnimatedSprite2D *animated_sprite = memnew(AnimatedSprite2D);
		animated_sprite->set_name("Anim2D");
		sprite->add_child(animated_sprite);
		Area2D *area = memnew(Area2D);
		area->set_name("Area2D");
		sprite->add_child(area);
		CollisionShape2D *area_collision_shape = memnew(CollisionShape2D);
		area_collision_shape->set_name("Trigger2D");
		const Ref<CircleShape2D> area_shape = memnew(CircleShape2D);
		area_shape->set_radius(10.0f);
		area_collision_shape->set_shape(area_shape);
		area->add_child(area_collision_shape);
		CollisionShape2D *body_collision_shape = memnew(CollisionShape2D);
		body_collision_shape->set_name("Collider2D");
		const Ref<CircleShape2D> body_shape = memnew(CircleShape2D);
		body_shape->set_radius(10.0f);
		body_collision_shape->set_shape(body_shape);
		sprite->add_child(body_collision_shape);
		Node2D *shooting_point = memnew(Node2D);
		shooting_point->set_name("ShootingPoint");
		sprite->add_child(shooting_point);
	} else {
		// load from path
		Ref<PackedScene> scene = ResourceLoader::load(path_str);
		if (scene.is_null()) {
			print_error("Failed to load sprite scene " + path_str);
			return NULL_OBJECT_ID;
		} else {
			Node *node = scene->instantiate();
			if (node == nullptr) {
				print_error("Failed to instantiate sprite scene " + path_str);
				return NULL_OBJECT_ID;
			}
			sprite = dynamic_cast<SpxSprite *>(node);
			if (sprite == nullptr) {
				print_error("Failed to load sprite scene, type invalid " + path_str);
				memdelete(node);
				return NULL_OBJECT_ID;
			}
		}
	}

	sprite->set_backdrop(is_backdrop);
	sprite->set_gid(get_unique_id());
	sprite_root->add_child(sprite);
	sprite->on_start();
	id_objects[sprite->get_gid()] = sprite;
	SPX_CALLBACK->func_on_sprite_ready(sprite->get_gid());
	return sprite->get_gid();
}

void SpxSpriteMgr::destroy_all_sprites() {
	sprite_root->queue_free();
	sprite_root = memnew(Node2D);
	sprite_root->set_name("sprite_root");
	get_spx_root()->add_child(sprite_root);

	id_objects.clear();
	bounding_collision_pairs.clear();
	pixel_collision_pairs.clear();
}

GdInt SpxSpriteMgr::clone_sprite(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(NULL_OBJECT_ID)
	SpxSprite *cloned = dynamic_cast<SpxSprite *>(sprite->duplicate());
	if (unlikely(!cloned)) {
		ERR_PRINT("Failed to clone sprite with GID: " + itos(obj));
		return NULL_OBJECT_ID;
	}
	cloned->set_gid(get_unique_id());
	sprite_root->add_child(cloned);
	id_objects[cloned->get_gid()] = cloned;
	cloned->on_start();
	SPX_CALLBACK->func_on_sprite_ready(cloned->get_gid());
	return cloned->get_gid();
}

GdBool SpxSpriteMgr::destroy_sprite(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	sprite->set_block_signals(true);
	sprite->queue_free();
	return true;
}

GdBool SpxSpriteMgr::is_sprite_alive(GdObj obj) {
	return get_sprite(obj) != nullptr;
}

void SpxSpriteMgr::set_position(GdObj obj, GdVec2 pos) {
	SPX_REQUIRE_SPRITE_VOID()
	// flip y axis
	sprite->set_position(GdVec2(pos.x, -pos.y));
}

void SpxSpriteMgr::set_transform(GdObj obj, GdVec2 pos, GdFloat rot, GdVec2 scale, GdBool visible, GdVec2 pivot) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_position(GdVec2(pos.x, -pos.y));
	sprite->set_rotation(rot);
	sprite->set_scale(scale);
	sprite->set_visible(visible);
	sprite->on_set_visible(visible);
	pivot.y = -pivot.y;
	sprite->set_pivot(pivot);
}

void SpxSpriteMgr::set_rotation(GdObj obj, GdFloat rot) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_rotation(rot);
}

void SpxSpriteMgr::set_scale(GdObj obj, GdVec2 scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_scale(scale);
}

GdVec2 SpxSpriteMgr::get_position(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto pos = sprite->get_position();
	// flip y axis
	return GdVec2{ pos.x, -pos.y };
}

GdFloat SpxSpriteMgr::get_rotation(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_rotation();
}

GdVec2 SpxSpriteMgr::get_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_scale();
}

void SpxSpriteMgr::set_render_scale(GdObj obj, GdVec2 scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_render_scale(scale);
}
GdVec2 SpxSpriteMgr::get_render_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_render_scale();
}

void SpxSpriteMgr::set_color(GdObj obj, GdColor color) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_color(color);
}

GdColor SpxSpriteMgr::get_color(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdColor())
	return sprite->get_color();
}

void SpxSpriteMgr::set_material_shader(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_shader(path);
}

GdString SpxSpriteMgr::get_material_shader(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_material_shader();
}

void SpxSpriteMgr::set_material_params(GdObj obj, GdString effect, GdFloat amount) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params(effect, amount);
}

GdFloat SpxSpriteMgr::get_material_params(GdObj obj, GdString effect) {
	SPX_REQUIRE_SPRITE_RETURN(GdFloat())
	return sprite->get_material_params(effect);
}

void SpxSpriteMgr::set_material_params_vec4(GdObj obj, GdString effect, GdVec4 vec4) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params_vec4(effect, vec4);
}

void SpxSpriteMgr::set_material_params_vec(GdObj obj, GdString effect, GdFloat x, GdFloat y, GdFloat z, GdFloat w) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params_vec4(effect, GdVec4(x, y, z, w));
}

GdVec4 SpxSpriteMgr::get_material_params_vec4(GdObj obj, GdString effect) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec4())
	return sprite->get_material_params_vec4(effect);
}

void SpxSpriteMgr::set_material_params_color(GdObj obj, GdString effect, GdColor color) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_material_params_color(effect, color);
}

GdColor SpxSpriteMgr::get_material_params_color(GdObj obj, GdString effect) {
	SPX_REQUIRE_SPRITE_RETURN(GdColor())
	return sprite->get_material_params_color(effect);
}

void SpxSpriteMgr::set_texture_atlas(GdObj obj, GdString path, GdRect2 rect2) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture_atlas(path, rect2);
}

void SpxSpriteMgr::set_texture(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture(path);
}

void SpxSpriteMgr::set_texture_atlas_direct(GdObj obj, GdString path, GdRect2 rect2) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture_atlas_direct(path, rect2, true);
}

void SpxSpriteMgr::set_texture_direct(GdObj obj, GdString path) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_texture_direct(path, true);
}

GdString SpxSpriteMgr::get_texture(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_texture();
}

void SpxSpriteMgr::set_visible(GdObj obj, GdBool visible) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_visible(visible);
	sprite->on_set_visible(visible);
}

GdBool SpxSpriteMgr::get_visible(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_visible();
}

GdInt SpxSpriteMgr::get_z_index(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_z_index();
}

void SpxSpriteMgr::set_z_index(GdObj obj, GdInt z) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_z_index(z);
}

void SpxSpriteMgr::play_anim(GdObj obj, GdString p_name, GdFloat p_speed, GdBool isLoop, GdBool p_revert) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->play_anim(p_name, p_speed, isLoop, p_revert);
}

void SpxSpriteMgr::play_backwards_anim(GdObj obj, GdString p_name) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->play_backwards_anim(p_name);
}

void SpxSpriteMgr::pause_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->pause_anim();
}

void SpxSpriteMgr::stop_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->stop_anim();
}

GdBool SpxSpriteMgr::is_playing_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_playing_anim();
}

void SpxSpriteMgr::set_anim(GdObj obj, GdString p_name) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim(p_name);
}

GdString SpxSpriteMgr::get_anim(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_anim();
}

void SpxSpriteMgr::set_anim_frame(GdObj obj, GdInt p_frame) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_frame(p_frame);
}

GdInt SpxSpriteMgr::get_anim_frame(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_anim_frame();
}

void SpxSpriteMgr::set_anim_speed_scale(GdObj obj, GdFloat p_speed_scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_speed_scale(p_speed_scale);
}

GdFloat SpxSpriteMgr::get_anim_speed_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(1.0)
	return sprite->get_anim_speed_scale();
}

GdFloat SpxSpriteMgr::get_anim_playing_speed(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(1.0)
	return sprite->get_anim_playing_speed();
}

void SpxSpriteMgr::set_anim_centered(GdObj obj, GdBool p_center) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_centered(p_center);
}

GdBool SpxSpriteMgr::is_anim_centered(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_anim_centered();
}

void SpxSpriteMgr::set_anim_offset(GdObj obj, GdVec2 p_offset) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_offset(p_offset);
}

GdVec2 SpxSpriteMgr::get_anim_offset(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_anim_offset();
}

void SpxSpriteMgr::set_anim_flip_h(GdObj obj, GdBool p_flip) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_flip_h(p_flip);
}

GdBool SpxSpriteMgr::is_anim_flipped_h(GdObj obj) {
	auto sprite = get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get property of a null sprite" + itos(obj));
		return false;
	}
	return sprite->is_anim_flipped_h();
}

void SpxSpriteMgr::set_anim_flip_v(GdObj obj, GdBool p_flip) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_anim_flip_v(p_flip);
}

GdBool SpxSpriteMgr::is_anim_flipped_v(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_anim_flipped_v();
}
GdString SpxSpriteMgr::get_current_anim_name(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdString())
	return sprite->get_current_anim_name();
}

void SpxSpriteMgr::set_velocity(GdObj obj, GdVec2 velocity) {
	SPX_REQUIRE_SPRITE_VOID()
	// flip y axis
	sprite->set_velocity(GdVec2(velocity.x, -velocity.y));
}

GdVec2 SpxSpriteMgr::get_velocity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto val = sprite->get_velocity();
	// flip y axis
	return GdVec2{ val.x, -val.y };
}

GdBool SpxSpriteMgr::is_on_floor(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_floor();
}

GdBool SpxSpriteMgr::is_on_floor_only(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_floor_only();
}

GdBool SpxSpriteMgr::is_on_wall(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_wall();
}

GdBool SpxSpriteMgr::is_on_wall_only(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_wall_only();
}

GdBool SpxSpriteMgr::is_on_ceiling(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_ceiling();
}

GdBool SpxSpriteMgr::is_on_ceiling_only(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_on_ceiling_only();
}

GdVec2 SpxSpriteMgr::get_last_motion(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_last_motion();
}

GdVec2 SpxSpriteMgr::get_position_delta(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_position_delta();
}

GdVec2 SpxSpriteMgr::get_floor_normal(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_floor_normal();
}

GdVec2 SpxSpriteMgr::get_wall_normal(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_wall_normal();
}

GdVec2 SpxSpriteMgr::get_real_velocity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	return sprite->get_real_velocity();
}

void SpxSpriteMgr::move_and_slide(GdObj obj) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->move_and_slide();
}

void SpxSpriteMgr::set_gravity(GdObj obj, GdFloat gravity) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_gravity(gravity);
}

GdFloat SpxSpriteMgr::get_gravity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_gravity();
}

void SpxSpriteMgr::set_mass(GdObj obj, GdFloat mass) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_mass(mass);
}

GdFloat SpxSpriteMgr::get_mass(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_mass();
}

void SpxSpriteMgr::add_force(GdObj obj, GdVec2 force) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->add_force(force);
}

void SpxSpriteMgr::add_impulse(GdObj obj, GdVec2 impulse) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->add_impulse(impulse);
}

void SpxSpriteMgr::set_physics_mode(GdObj obj, GdInt mode) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_physics_mode(mode);
}

GdInt SpxSpriteMgr::get_physics_mode(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_physics_mode();
}

void SpxSpriteMgr::set_use_gravity(GdObj obj, GdBool enabled) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_use_gravity(enabled);
}

GdBool SpxSpriteMgr::is_use_gravity(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_use_gravity();
}

void SpxSpriteMgr::set_gravity_scale(GdObj obj, GdFloat scale) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_gravity_scale(scale);
}

GdFloat SpxSpriteMgr::get_gravity_scale(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(1.0f)
	return sprite->get_gravity_scale();
}

void SpxSpriteMgr::set_drag(GdObj obj, GdFloat drag) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_drag(drag);
}

GdFloat SpxSpriteMgr::get_drag(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0.0f)
	return sprite->get_drag();
}

void SpxSpriteMgr::set_friction(GdObj obj, GdFloat friction) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_friction(friction);
}

GdFloat SpxSpriteMgr::get_friction(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0.0f)
	return sprite->get_friction();
}

void SpxSpriteMgr::set_collision_layer(GdObj obj, GdInt layer) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collision_layer((uint32_t)layer);
}

GdInt SpxSpriteMgr::get_collision_layer(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_collision_layer();
}

void SpxSpriteMgr::set_collision_mask(GdObj obj, GdInt mask) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collision_mask((uint32_t)mask);
}

GdInt SpxSpriteMgr::get_collision_mask(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_collision_mask();
}

void SpxSpriteMgr::set_trigger_layer(GdObj obj, GdInt layer) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_layer(layer);
}

GdInt SpxSpriteMgr::get_trigger_layer(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_trigger_layer();
}

void SpxSpriteMgr::set_trigger_mask(GdObj obj, GdInt mask) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_mask(mask);
}

GdInt SpxSpriteMgr::get_trigger_mask(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(0)
	return sprite->get_trigger_mask();
}

void SpxSpriteMgr::set_collider_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collider_rect(center, size);
}

void SpxSpriteMgr::set_collider_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collider_circle(center, radius);
}

void SpxSpriteMgr::set_collider_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collider_capsule(center, size);
}

void SpxSpriteMgr::set_collider_polygon(GdObj obj, GdVec2 center, GdArray points) {
	SPX_REQUIRE_SPRITE_VOID()
	center.y = -center.y;
	sprite->set_collider_polygon(center, points);
}

void SpxSpriteMgr::set_collision_enabled(GdObj obj, GdBool enabled) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_collision_enabled(enabled);
}

GdBool SpxSpriteMgr::is_collision_enabled(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_collision_enabled();
}

void SpxSpriteMgr::set_trigger_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_rect(center, size);
}

void SpxSpriteMgr::set_trigger_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_circle(center, radius);
}

void SpxSpriteMgr::set_trigger_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_capsule(center, size);
}

void SpxSpriteMgr::set_trigger_polygon(GdObj obj, GdVec2 center, GdArray points) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_polygon(center, points);
}

void SpxSpriteMgr::set_trigger_enabled(GdObj obj, GdBool trigger) {
	SPX_REQUIRE_SPRITE_VOID()
	sprite->set_trigger_enabled(trigger);
}
GdBool SpxSpriteMgr::is_trigger_enabled(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	return sprite->is_trigger_enabled();
}

Ref<Image> SpxSpriteMgr::_get_current_frame_image(AnimatedSprite2D *sprite) {
	Ref<Texture2D> texture = get_current_frame_texture(sprite);
	if (texture.is_null()) {
		return Ref<Image>();
	}

	Ref<Image> image = texture->get_image();
	if (image.is_null()) {
		return Ref<Image>();
	}
	return image;
}

Rect2 SpxSpriteMgr::_get_sprite_aabb(AnimatedSprite2D *anim2d) {
	if (!anim2d) {
		return Rect2();
	}

	Ref<Texture2D> texture = get_current_frame_texture(anim2d);
	if (texture.is_null()) {
		return Rect2();
	}

	return get_transformed_rect_aabb(anim2d->get_global_transform(), get_anim_local_rect(anim2d, texture->get_size()));
}

GdBool SpxSpriteMgr::check_collision_with_sprite(GdObj obj, GdObj obj_b, GdFloat alpha_threshold, GdBool use_pixel_perfect) {
	SPX_REQUIRE_SPRITE_RETURN(false)
	SPX_REQUIRE_TARGET_SPRITE_RETURN(obj_b, false)

	// If not using pixel-perfect collision, use simple collider2d collision detection
	if (!use_pixel_perfect) {
		return sprite->check_collision(sprite_target.get(), false, false);
	}

	return _check_pixel_collision_between(sprite.get(), sprite_target.get(), alpha_threshold);
}

bool SpxSpriteMgr::_check_pixel_collision_between(SpxSprite *sprite_a, SpxSprite *sprite_b, GdFloat alpha_threshold) {
	PixelCollisionQuery query_a;
	PixelCollisionQuery query_b;
	AnimatedSprite2D *anim_a = sprite_a->get_anim2d();
	AnimatedSprite2D *anim_b = sprite_b->get_anim2d();
	if (!anim_a || !anim_b) {
		return false;
	}
	if (!build_pixel_collision_query(anim_a, query_a) || !build_pixel_collision_query(anim_b, query_b)) {
		return false;
	}

	const Rect2i overlap_rect = get_pixel_overlap_rect(query_a.bounds, query_b.bounds);
	if (!overlap_rect.has_area()) {
		return false;
	}
	if (!ensure_query_image(query_a) || !ensure_query_image(query_b)) {
		return false;
	}
	const Vector2i overlap_end = overlap_rect.position + overlap_rect.size;

	for (int x = overlap_rect.position.x; x < overlap_end.x; x += pixel_collision_sampling_step) {
		for (int y = overlap_rect.position.y; y < overlap_end.y; y += pixel_collision_sampling_step) {
			const Vector2 sample_pos((real_t)x + 0.5f, (real_t)y + 0.5f);
			Color color_a;
			if (!read_query_pixel(query_a, sample_pos, color_a) || color_a.a <= alpha_threshold) {
				continue;
			}

			Color color_b;
			if (read_query_pixel(query_b, sample_pos, color_b) && color_b.a > alpha_threshold) {
				return true; // Early exit on first collision detected
			}
		}
	}
	return false;
}

GdBool SpxSpriteMgr::check_collision_by_color(GdObj obj, GdColor color, GdFloat color_threshold, GdFloat alpha_threshold) {
	const GdFloat threshold_sq = color_threshold * color_threshold;
	return _check_collision(obj, [=](GdColor a, GdColor b) -> bool {
		if (a.a <= alpha_threshold) {
			return false;
		}
		return color_distance_squared(color, b) < threshold_sq;
	});
}

GdBool SpxSpriteMgr::check_collision_by_colors(GdObj obj, GdColor sprite_color, GdColor target_color, GdFloat color_threshold, GdFloat alpha_threshold) {
	const GdFloat threshold_sq = color_threshold * color_threshold;
	return _check_collision(obj, [=](GdColor a, GdColor b) -> bool {
		if (a.a <= alpha_threshold || b.a <= alpha_threshold) {
			return false;
		}
		if (color_distance_squared(sprite_color, a) >= threshold_sq) {
			return false;
		}
		return color_distance_squared(target_color, b) < threshold_sq;
	});
}

GdBool SpxSpriteMgr::check_collision_by_alpha(GdObj obj, GdFloat alpha_threshold) {
	return _check_collision(obj, [alpha_threshold](GdColor a, GdColor b) -> bool {
		return a.a > alpha_threshold && b.a > alpha_threshold;
	});
}

GdBool SpxSpriteMgr::_check_collision(GdObj obj, ColorCheckFunc check_func) {
	SPX_REQUIRE_SPRITE_RETURN(false) // Ensure sprite exists

	AnimatedSprite2D *anim1 = sprite->get_anim2d();
	if (!anim1) {
		return false;
	}
	PixelCollisionQuery query1;
	if (!build_pixel_collision_query(anim1, query1)) {
		return false;
	}
	if (!ensure_query_image(query1)) {
		return false;
	}

	// Iterate through all objects
	for (const auto &item : id_objects) {
		SpxSprite *sp2 = item.value;
		if (sprite.get() == sp2) {
			continue; // Skip itself
		}

		AnimatedSprite2D *anim2 = sp2->get_anim2d();
		if (!anim2) {
			continue;
		}
		PixelCollisionQuery query2;
		if (!build_pixel_collision_query(anim2, query2)) {
			continue;
		}

		const Rect2i overlap_rect = get_pixel_overlap_rect(query1.bounds, query2.bounds);
		if (!overlap_rect.has_area()) {
			continue;
		}
		if (!ensure_query_image(query2)) {
			continue;
		}
		const Vector2i overlap_end = overlap_rect.position + overlap_rect.size;

		// Iterate through the overlapping area for pixel-perfect collision detection
		// Use sampling step for performance optimization
		for (int x = overlap_rect.position.x; x < overlap_end.x; x += pixel_collision_sampling_step) {
			for (int y = overlap_rect.position.y; y < overlap_end.y; y += pixel_collision_sampling_step) {
				const Vector2 sample_pos((real_t)x + 0.5f, (real_t)y + 0.5f);
				Color color1;
				if (!read_query_pixel(query1, sample_pos, color1)) {
					continue;
				}

				Color color2;
				if (!read_query_pixel(query2, sample_pos, color2)) {
					continue;
				}

				if (check_func(color1, color2)) {
					return true; // Early exit on collision detected
				}
			}
		}
	}
	return false;
}

void SpxSpriteMgr::on_trigger_enter(GdInt self_id, GdInt other_id) {
	if (physicsMgr->is_collision_by_pixel) {
		bounding_collision_pairs.insert(TriggerPair(self_id, other_id));
	} else {
		SPX_CALLBACK->func_on_trigger_enter(self_id, other_id);
	}
}
void SpxSpriteMgr::on_trigger_exit(GdInt self_id, GdInt other_id) {
	if (physicsMgr->is_collision_by_pixel) {
		const TriggerPair pair(self_id, other_id);
		// Trigger separation ends the broad-phase candidate pair, so pixel collision
		// tracking must stop immediately instead of waiting for another pixel check.
		bounding_collision_pairs.erase(pair);
		if (_erase_pixel_collision_pair(pair)) {
			_notify_pixel_collision_exit(pair);
		}
	} else {
		SPX_CALLBACK->func_on_trigger_exit(self_id, other_id);
	}
}

void SpxSpriteMgr::_notify_pixel_collision_enter(const TriggerPair &pair) {
	SPX_CALLBACK->func_on_trigger_enter(pair.id1, pair.id2);
	SPX_CALLBACK->func_on_trigger_enter(pair.id2, pair.id1);
}

void SpxSpriteMgr::_notify_pixel_collision_exit(const TriggerPair &pair, GdObj skip_id) {
	if (pair.id1 != skip_id) {
		SPX_CALLBACK->func_on_trigger_exit(pair.id1, pair.id2);
	}
	if (pair.id2 != skip_id) {
		SPX_CALLBACK->func_on_trigger_exit(pair.id2, pair.id1);
	}
}

bool SpxSpriteMgr::_erase_pixel_collision_pair(const TriggerPair &pair) {
	return pixel_collision_pairs.erase(pair) > 0;
}

void SpxSpriteMgr::_remove_collision_pairs_for_sprite(GdObj obj) {
	Vector<TriggerPair> exit_triggers;
	for (auto it = pixel_collision_pairs.begin(); it != pixel_collision_pairs.end();) {
		if (it->id1 == obj || it->id2 == obj) {
			exit_triggers.push_back(*it);
			it = pixel_collision_pairs.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = bounding_collision_pairs.begin(); it != bounding_collision_pairs.end();) {
		if (it->id1 == obj || it->id2 == obj) {
			it = bounding_collision_pairs.erase(it);
		} else {
			++it;
		}
	}

	for (const auto &pair : exit_triggers) {
		_notify_pixel_collision_exit(pair, obj);
	}
}

void SpxSpriteMgr::_check_pixel_collision_events() {
	if (!physicsMgr->is_collision_by_pixel || bounding_collision_pairs.empty()) {
		return;
	}

	Vector<TriggerPair> enter_triggers;
	Vector<TriggerPair> exit_triggers;

	for (auto it = bounding_collision_pairs.begin(); it != bounding_collision_pairs.end();) {
		const TriggerPair trigger = *it;
		SpxSprite *sprite1 = get_sprite(trigger.id1);
		SpxSprite *sprite2 = get_sprite(trigger.id2);
		if (sprite1 == nullptr || sprite2 == nullptr) {
			_erase_pixel_collision_pair(trigger);
			it = bounding_collision_pairs.erase(it);
			continue;
		}

		if (_check_pixel_collision_between(sprite1, sprite2, DEFAULT_COLLISION_ALPHA_THRESHOLD)) {
			if (pixel_collision_pairs.insert(trigger).second) {
				enter_triggers.push_back(trigger);
			}
		} else if (_erase_pixel_collision_pair(trigger)) {
			exit_triggers.push_back(trigger);
		}

		++it;
	}

	for (const auto &trigger : exit_triggers) {
		_notify_pixel_collision_exit(trigger);
	}
	for (const auto &trigger : enter_triggers) {
		_notify_pixel_collision_enter(trigger);
	}
}

void SpxSpriteMgr::set_pivot(GdObj obj, GdVec2 pivot) {
	SPX_REQUIRE_SPRITE_VOID()
	pivot.y = -pivot.y;
	sprite->set_pivot(pivot);
}
GdVec2 SpxSpriteMgr::get_pivot(GdObj obj) {
	SPX_REQUIRE_SPRITE_RETURN(GdVec2())
	auto pivot = sprite->get_pivot();
	return GdVec2(pivot.x, -pivot.y);
}

namespace {

uint32_t read_u32_lane(float value) {
	uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

GdObj read_gd_obj_lanes(const float *record) {
	uint64_t low = read_u32_lane(record[1]);
	uint64_t high = read_u32_lane(record[2]);
	uint64_t bits = (high << 32) | low;
	int64_t value = 0;
	std::memcpy(&value, &bits, sizeof(value));
	return static_cast<GdObj>(value);
}

void batch_update_transforms_impl(SpxSpriteMgr *mgr, const float *buffer_data, int len, const char *op_name) {
	// Buffer format with header: [updateCount, deleteCount, update_data..., delete_ids...]
	// - Header: [updateCount, deleteCount]
	// - Update section: [id, x, y, rotation, scaleX, scaleY, offsetX, offsetY, visible, ...] (9 fields per sprite)
	// - Delete section: [id1, id2, id3, ...] (1 field per sprite)
	const int FIELDS_PER_SPRITE = 9;
	const int HEADER_SIZE = 2;

	if (buffer_data == nullptr) {
		return;
	}

	if (len < HEADER_SIZE) {
		return;
	}

	// Read header using direct array access
	int update_count = static_cast<int>(buffer_data[0]);
	int delete_count = static_cast<int>(buffer_data[1]);
	if (update_count < 0 || delete_count < 0) {
		print_error(String(op_name) + ": buffer counts must be non-negative.");
		return;
	}

	// Validate buffer size
	int64_t expected_size = (int64_t)HEADER_SIZE + (int64_t)update_count * FIELDS_PER_SPRITE + delete_count;
	if (expected_size > INT_MAX) {
		print_error(String(op_name) + ": buffer count too large, would cause integer overflow.");
		return;
	}
	if (len != expected_size) {
		print_error(String(op_name) + ": buffer size " + itos(len) +
				" does not match expected size " + itos((int)expected_size) +
				" (updateCount=" + itos(update_count) + ", deleteCount=" + itos(delete_count) + ")");
		return;
	}

	int idx = HEADER_SIZE;

	// Process updates
	for (int i = 0; i < update_count; i++) {
		// Extract sprite ID and data using direct array access
		auto sprite_id = static_cast<GdObj>(buffer_data[idx]);
		auto x = buffer_data[idx + 1];
		auto y = buffer_data[idx + 2];
		auto rotation = buffer_data[idx + 3];
		auto scale_x = buffer_data[idx + 4];
		auto scale_y = buffer_data[idx + 5];
		auto offset_x = buffer_data[idx + 6];
		auto offset_y = buffer_data[idx + 7];
		auto visible = buffer_data[idx + 8] != 0.0;

		idx += FIELDS_PER_SPRITE;

		SpxSprite *sprite = mgr->get_sprite(sprite_id);
		if (sprite == nullptr) {
			continue;
		}

		// Apply transforms
		// Note: Y-axis is flipped in Godot coordinate system
		sprite->set_position(GdVec2(x, -y));
		sprite->set_rotation(rotation);
		sprite->set_scale(GdVec2(scale_x, scale_y));
		sprite->set_visible(visible);
		sprite->on_set_visible(visible);
		sprite->set_pivot(GdVec2(offset_x, -offset_y));
	}

	// Process deletes
	for (int i = 0; i < delete_count; i++) {
		auto sprite_id = static_cast<GdObj>(buffer_data[idx]);
		idx++;

		SpxSprite *sprite = mgr->get_sprite(sprite_id);
		if (sprite != nullptr) {
			sprite->set_block_signals(true);
			sprite->queue_free();
		}
	}
}

void batch_update_visuals_impl(SpxSpriteMgr *mgr, const float *buffer_data, int len, const char *op_name) {
	// Buffer format: [count, entry0..., entry1..., ...]
	// Each entry (9 floats): [spriteId, renderScaleX, renderScaleY, zIndex, flags, uvX, uvY, uvW, uvH]
	const int VISUAL_FIELDS_PER_SPRITE = 9;
	const int HEADER_SIZE = 1;
	const int FLAG_HAS_ZINDEX = 1;
	const int FLAG_HAS_UV_REMAP = 2;

	if (buffer_data == nullptr) {
		return;
	}

	if (len < HEADER_SIZE) {
		return;
	}

	int count = static_cast<int>(buffer_data[0]);
	if (count < 0) {
		print_error(String(op_name) + ": buffer count must be non-negative.");
		return;
	}

	int64_t expected_size = (int64_t)HEADER_SIZE + (int64_t)count * VISUAL_FIELDS_PER_SPRITE;
	if (expected_size > INT_MAX) {
		print_error(String(op_name) + ": buffer count too large, would cause integer overflow.");
		return;
	}
	if (len != expected_size) {
		print_error(String(op_name) + ": buffer size " + itos(len) +
				" does not match expected size " + itos((int)expected_size) +
				" (count=" + itos(count) + ")");
		return;
	}

	int idx = HEADER_SIZE;

	for (int i = 0; i < count; i++) {
		auto sprite_id = static_cast<GdObj>(buffer_data[idx]);
		auto render_scale_x = buffer_data[idx + 1];
		auto render_scale_y = buffer_data[idx + 2];
		auto z_index = static_cast<int>(buffer_data[idx + 3]);
		auto flags = static_cast<int>(buffer_data[idx + 4]);
		auto uv_x = buffer_data[idx + 5];
		auto uv_y = buffer_data[idx + 6];
		auto uv_w = buffer_data[idx + 7];
		auto uv_h = buffer_data[idx + 8];

		idx += VISUAL_FIELDS_PER_SPRITE;

		SpxSprite *sprite = mgr->get_sprite(sprite_id);
		if (sprite == nullptr) {
			continue;
		}

		// Apply render scale
		sprite->set_render_scale(GdVec2(render_scale_x, render_scale_y));

		// Apply z-index if flag is set
		if (flags & FLAG_HAS_ZINDEX) {
			sprite->set_z_index(z_index);
		}

		// Apply UV remap if flag is set
		if (flags & FLAG_HAS_UV_REMAP) {
			String uv_param = "uv_remap";
			sprite->set_material_params_vec4(SpxReturnStr(uv_param), GdVec4(uv_x, uv_y, uv_w, uv_h));
		}
	}
}

} // namespace

void SpxSpriteMgr::batch_update_transforms(const float *buffer_data, int len) {
	if (buffer_data == nullptr || len < 2) {
		return;
	}

	batch_update_transforms_impl(this, buffer_data, len, "batch_update_transforms");
}

void SpxSpriteMgr::batch_update_visuals(const float *buffer_data, int len) {
	if (buffer_data == nullptr || len < 1) {
		return;
	}

	batch_update_visuals_impl(this, buffer_data, len, "batch_update_visuals");
}

void SpxSpriteMgr::_batch_write_positions(const GdObj *ids, int count, float *out, int out_len) {
	if (count <= 0) {
		return;
	}
	if (count > INT_MAX / 2) {
		print_error("_batch_write_positions: count too large, would cause integer overflow.");
		return;
	}

	int need = count * 2;
	if (!ids || !out || out_len < need) {
		print_error("_batch_write_positions: invalid input or output buffer.");
		return;
	}

	int j = 0;
	for (int i = 0; i < count; i++) {
		GdObj id = ids[i];
		SpxSprite *sprite = get_sprite(id);
		if (sprite != nullptr) {
			auto pos = sprite->get_position();
			out[j++] = pos.x;
			out[j++] = -pos.y;
		} else {
			out[j++] = 0.0f;
			out[j++] = 0.0f;
		}
	}
}

void SpxSpriteMgr::batch_retrieve_positions(const GdObj *ids, int count, float *out, int out_len) {
	_batch_write_positions(ids, count, out, out_len);
}

void SpxSpriteMgr::batch_update_physics(const float *buffer_data, int len) {
	// Buffer format: [count, cmd, spriteIdLowBits, spriteIdHighBits, a, b, reserved0, ...].
	// Integer lanes are carried as raw float32 bits to preserve 32/64-bit ids and masks.
	if (buffer_data == nullptr || len < 1) {
		return;
	}

	int count = (int)buffer_data[0];
	if (count <= 0) {
		return;
	}
	int64_t required = 1 + (int64_t)count * SPX_PHYSICS_BATCH_FIELDS;
	if (required > INT_MAX) {
		print_error("batch_update_physics: count too large, would cause integer overflow.");
		return;
	}

	if (len != required) {
		print_error("batch_update_physics: buffer length is invalid.");
		return;
	}

	int idx = 1;
	for (int i = 0; i < count; i++) {
		int cmd = (int)buffer_data[idx];
		GdObj obj = read_gd_obj_lanes(&buffer_data[idx]);
		SpxSprite *sprite = get_sprite(obj);
		if (sprite != nullptr) {
			float a = buffer_data[idx + 3];
			float b = buffer_data[idx + 4];
			switch (cmd) {
				case SPX_PHYSICS_CMD_VELOCITY:
					sprite->set_velocity(GdVec2(a, -b));
					break;
				case SPX_PHYSICS_CMD_GRAVITY:
					sprite->set_gravity(a);
					break;
				case SPX_PHYSICS_CMD_MASS:
					sprite->set_mass(a);
					break;
				case SPX_PHYSICS_CMD_MODE:
					sprite->set_physics_mode((GdInt)(int32_t)read_u32_lane(a));
					break;
				case SPX_PHYSICS_CMD_USE_GRAVITY:
					sprite->set_use_gravity(a != 0);
					break;
				case SPX_PHYSICS_CMD_GRAVITY_SCALE:
					sprite->set_gravity_scale(a);
					break;
				case SPX_PHYSICS_CMD_DRAG:
					sprite->set_drag(a);
					break;
				case SPX_PHYSICS_CMD_FRICTION:
					sprite->set_friction(a);
					break;
				case SPX_PHYSICS_CMD_COLLISION_LAYER:
					sprite->set_collision_layer((GdInt)read_u32_lane(a));
					break;
				case SPX_PHYSICS_CMD_COLLISION_MASK:
					sprite->set_collision_mask((GdInt)read_u32_lane(a));
					break;
				case SPX_PHYSICS_CMD_TRIGGER_LAYER:
					sprite->set_trigger_layer((GdInt)read_u32_lane(a));
					break;
				case SPX_PHYSICS_CMD_TRIGGER_MASK:
					sprite->set_trigger_mask((GdInt)read_u32_lane(a));
					break;
				case SPX_PHYSICS_CMD_COLLISION_ENABLED:
					sprite->set_collision_enabled(a != 0);
					break;
				case SPX_PHYSICS_CMD_TRIGGER_ENABLED:
					sprite->set_trigger_enabled(a != 0);
					break;
				default:
					break;
			}
		}
		idx += SPX_PHYSICS_BATCH_FIELDS;
	}
}

void SpxSpriteMgr::set_pixel_collision_sampling_step(GdInt step) {
	// Clamp to valid range (minimum 1, as 0 or negative would cause infinite loop)
	if (step < 1) {
		pixel_collision_sampling_step = 1;
		print_error("pixel_collision_sampling_step must be at least 1. Setting to 1.");
	} else {
		pixel_collision_sampling_step = step;
	}
}

GdInt SpxSpriteMgr::get_pixel_collision_sampling_step() {
	return pixel_collision_sampling_step;
}
