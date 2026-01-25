/**************************************************************************/
/*  spx_physics_mgr.cpp                                                   */
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

#include "spx_physics_mgr.h"


#include "core/templates/hash_set.h"
#include "core/variant/typed_array.h"
#include "gdextension_spx_ext.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_server_2d.h"
#include "servers/physics_server_3d.h"


#include "scene/2d/camera_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/main/window.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "spx_sprite.h"
#include "spx_engine.h"
#include "spx_camera_mgr.h"
#include "spx_sprite_mgr.h"

GdArray SpxRaycastInfo::ToArray(){
	GdArray result_array = SpxBaseMgr::create_array(GD_ARRAY_TYPE_INT64, 6);
	SpxBaseMgr::set_array(result_array,0,(GdInt)collide);
	SpxBaseMgr::set_array(result_array,1,(GdInt)sprite_gid);
	SpxBaseMgr::set_array(result_array,2,spx_float_to_int(position.x));
	SpxBaseMgr::set_array(result_array,3,spx_float_to_int(position.y));
	SpxBaseMgr::set_array(result_array,4,spx_float_to_int(normal.x));
	SpxBaseMgr::set_array(result_array,5,spx_float_to_int(normal.y));
	return result_array;
}

void SpxPhysicsDefine::set_global_gravity(GdFloat gravity) {
	SpxPhysicsDefine::global_gravity = gravity;
}

GdFloat SpxPhysicsDefine::get_global_gravity() {
	return SpxPhysicsDefine::global_gravity;
}

void SpxPhysicsDefine::set_global_friction(GdFloat friction) {
	SpxPhysicsDefine::global_friction = friction;
}

GdFloat SpxPhysicsDefine::get_global_friction() {
	return SpxPhysicsDefine::global_friction;
}

void SpxPhysicsDefine::set_global_air_drag(GdFloat air_drag) {
	SpxPhysicsDefine::global_air_drag = air_drag;
}

GdFloat SpxPhysicsDefine::get_global_air_drag() {
	return SpxPhysicsDefine::global_air_drag	;
}




void SpxPhysicsMgr::on_awake() {
	SpxBaseMgr::on_awake();
	is_collision_by_pixel = true;
}

void SpxPhysicsMgr::on_reset(int reset_code) {
}

SpxRaycastInfo SpxPhysicsMgr::_raycast(GdVec2 from, GdVec2 to, GdArray ignore_sprites, GdInt collision_mask, GdBool collide_with_areas, GdBool collide_with_bodies) {
	SpxRaycastInfo info;
	info.collide = false;
	info.position = GdVec2{0, 0};
	info.normal = GdVec2{0, 0};
	info.sprite_gid = 0;

	// invert y
	GdVec2 current_from = GdVec2{from.x, -from.y};
	GdVec2 target_to = GdVec2{to.x, -to.y};

	HashSet<RID> ignore_set;
	if(ignore_sprites && ignore_sprites->size > 0){
		GdObj* sprite_data = (SpxBaseMgr::get_array<GdObj>(ignore_sprites, 0));
		for (int i = 0; i < ignore_sprites->size; i++) {
			auto obj = sprite_data[i];
			auto sprite = spriteMgr->get_sprite(obj);
			if(sprite != nullptr){
				ignore_set.insert(sprite->get_rid());
				auto trigger = sprite->get_area2d();
				if(trigger != nullptr){
					ignore_set.insert(trigger->get_rid());
				}
			}
		}
	}
	PhysicsDirectSpaceState2D::RayResult result;
	PhysicsDirectSpaceState2D::RayParameters params;
	params.from = current_from;
	params.to = target_to;
	params.collision_mask = (uint32_t)collision_mask;
	params.collide_with_areas = collide_with_areas;
	params.collide_with_bodies = collide_with_bodies;
	params.exclude = ignore_set;

	auto node = (Node2D *)get_root();
	PhysicsDirectSpaceState2D *space_state = node->get_world_2d()->get_direct_space_state();
	if (!space_state) {
		return info;
	}

	bool hit = space_state->intersect_ray(params, result);
	if (!hit) {
		return info;
	}
	SpxSprite *collider = dynamic_cast<SpxSprite *>(result.collider);
	GdObj current_gid = collider ? collider->get_gid() : 0;
	info.collide = true;
	info.position = GdVec2{result.position.x, -result.position.y};
	info.normal = GdVec2{result.normal.x, -result.normal.y};
	info.sprite_gid = current_gid;
	return info;
}



GdArray SpxPhysicsMgr::raycast_with_details(GdVec2 from, GdVec2 to, GdArray ignore_sprites, GdInt collision_mask, GdBool collide_with_areas, GdBool collide_with_bodies){
	SpxRaycastInfo info = _raycast(from, to, ignore_sprites, collision_mask, collide_with_areas, collide_with_bodies);
	return info.ToArray();
}

GdObj SpxPhysicsMgr::raycast(GdVec2 from, GdVec2 to, GdInt collision_mask) {
	auto node = (Node2D *)get_root();
	PhysicsDirectSpaceState2D *space_state = node->get_world_2d()->get_direct_space_state();

	PhysicsDirectSpaceState2D::RayResult result;
	PhysicsDirectSpaceState2D::RayParameters params;
	// flip y axis
	from = GdVec2{ from.x, -from.y };
	to = GdVec2{ to.x, -to.y };
	params.from = from;
	params.to = to;
	params.collision_mask = (uint32_t)collision_mask;
	bool hit = space_state->intersect_ray(params, result);
	if (hit) {
		SpxSprite *collider = dynamic_cast<SpxSprite *>(result.collider);
		if (collider != nullptr) {
			return collider->get_gid();
		}
	}
	return 0;
}

GdBool SpxPhysicsMgr::check_collision(GdVec2 from, GdVec2 to, GdInt collision_mask, GdBool collide_with_areas, GdBool collide_with_bodies) {
	auto node = (Node2D *)get_root();
	PhysicsDirectSpaceState2D *space_state = node->get_world_2d()->get_direct_space_state();
	PhysicsDirectSpaceState2D::RayResult result;
	PhysicsDirectSpaceState2D::RayParameters params;

	// flip y axis
	from = GdVec2{ from.x, -from.y };
	to = GdVec2{ to.x, -to.y };
	params.from = from;
	params.to = to;
	params.collision_mask = (uint32_t)collision_mask;
	params.collide_with_areas = collide_with_areas;
	params.collide_with_bodies = collide_with_bodies;
	bool hit = space_state->intersect_ray(params, result);
	return hit;
}

const GdInt BOUND_CAM_LEFT = 1 << 0;
const GdInt BOUND_CAM_TOP = 1 << 1;
const GdInt BOUND_CAM_RIGHT = 1 << 2;
const GdInt BOUND_CAM_BOTTOM = 1 << 3;

GdInt SpxPhysicsMgr::check_touched_camera_boundaries(GdObj obj) {
	auto sprite = spriteMgr->get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get property of a null sprite gid=" + itos(obj));
		return false;
	}
	Transform2D sprite_transform = sprite->get_global_transform();

	CollisionShape2D *collision_shape = sprite->get_trigger();
	if (!collision_shape) {
		return false;
	}
	Ref<Shape2D> sprite_shape = collision_shape->get_shape();
	if (sprite_shape.is_null()) {
		return false;
	}

	Camera2D *camera = get_tree()->get_root()->get_camera_2d();
	if (!camera) {
		return false;
	}
	Transform2D camera_transform = camera->get_global_transform();

	Vector2 viewport_size = camera->get_viewport_rect().size;
	Vector2 zoom = camera->get_zoom();
	Vector2 half_size = (viewport_size / zoom) * 0.5;
	Vector2 camera_position = camera_transform.get_origin();

	Ref<RectangleShape2D> vertical_edge_shape;
	vertical_edge_shape.instantiate();
	vertical_edge_shape->set_size(Vector2(2, half_size.y*50));// mutil by 50 is to check the case of some collider is out of boundary

	Ref<RectangleShape2D> horizontal_edge_shape;
	horizontal_edge_shape.instantiate();
	horizontal_edge_shape->set_size(Vector2(half_size.x*50, 2));

	Transform2D left_edge_transform(0, camera_position + Vector2(-half_size.x, 0));
	Transform2D right_edge_transform(0, camera_position + Vector2(half_size.x, 0));
	Transform2D top_edge_transform(0, camera_position + Vector2(0, -half_size.y));
	Transform2D bottom_edge_transform(0, camera_position + Vector2(0, half_size.y));

	bool is_colliding_left = sprite_shape->collide(sprite_transform, vertical_edge_shape, left_edge_transform);
	bool is_colliding_right = sprite_shape->collide(sprite_transform, vertical_edge_shape, right_edge_transform);
	bool is_colliding_top = sprite_shape->collide(sprite_transform, horizontal_edge_shape, top_edge_transform);
	bool is_colliding_bottom = sprite_shape->collide(sprite_transform, horizontal_edge_shape, bottom_edge_transform);
	GdInt result = 0;
	result += is_colliding_top ? BOUND_CAM_TOP : 0;
	result += is_colliding_right ?  BOUND_CAM_RIGHT : 0;
	result += is_colliding_bottom ? BOUND_CAM_BOTTOM : 0;
	result += is_colliding_left ? BOUND_CAM_LEFT : 0;
	return result;
}

GdBool SpxPhysicsMgr::check_touched_camera_boundary(GdObj obj, GdInt board_type) {
	auto result = check_touched_camera_boundaries(obj);
	return (result & board_type) != 0;
}

GdInt SpxPhysicsMgr::check_nearest_touched_camera_boundary(GdObj obj) {
	auto sprite = spriteMgr->get_sprite(obj);
	if (sprite == nullptr) {
		print_error("try to get property of a null sprite gid=" + itos(obj));
		return 0;
	}

	// Get sprite's bounding box
	CollisionShape2D *collision_shape = sprite->get_trigger();
	if (!collision_shape) {
		return 0;
	}
	
	Ref<Shape2D> sprite_shape = collision_shape->get_shape();
	if (sprite_shape.is_null()) {
		return 0;
	}

	Transform2D sprite_transform = sprite->get_global_transform();
	Rect2 sprite_rect = sprite_shape->get_rect();
	
	// Transform the rect to world coordinates
	Vector2 sprite_pos = sprite_transform.get_origin();
	Vector2 sprite_scale = sprite_transform.get_scale();
	
	// Get the actual bounding box in world space
	real_t left = sprite_pos.x + sprite_rect.position.x * sprite_scale.x;
	real_t right = sprite_pos.x + (sprite_rect.position.x + sprite_rect.size.x) * sprite_scale.x;
	real_t top = sprite_pos.y + sprite_rect.position.y * sprite_scale.y;
	real_t bottom = sprite_pos.y + (sprite_rect.position.y + sprite_rect.size.y) * sprite_scale.y;

	// Get camera boundaries
	Rect2 camera_rect = cameraMgr->get_global_camera_rect();
	real_t stage_left = camera_rect.position.x;
	real_t stage_top = camera_rect.position.y;
	real_t stage_right = camera_rect.position.x + camera_rect.size.x;
	real_t stage_bottom = camera_rect.position.y + camera_rect.size.y;

	// Calculate distances to edges (positive when far away, clamped to 0 when beyond)
	real_t dist_left = MAX(0.0, left - stage_left);
	real_t dist_top = MAX(0.0, top - stage_top);
	real_t dist_right = MAX(0.0, stage_right - right);
	real_t dist_bottom = MAX(0.0, stage_bottom - bottom);

	// Find nearest edge
	real_t min_dist = INFINITY;
	GdInt nearest_edge = 0;

	if (dist_left < min_dist) {
		min_dist = dist_left;
		nearest_edge = BOUND_CAM_LEFT;
	}
	if (dist_top < min_dist) {
		min_dist = dist_top;
		nearest_edge = BOUND_CAM_TOP;
	}
	if (dist_right < min_dist) {
		min_dist = dist_right;
		nearest_edge = BOUND_CAM_RIGHT;
	}
	if (dist_bottom < min_dist) {
		min_dist = dist_bottom;
		nearest_edge = BOUND_CAM_BOTTOM;
	}

	if(min_dist > 0){
		return 0;
	}

	return nearest_edge;
}

//
void SpxPhysicsMgr::set_collision_system_type(GdBool is_collision_by_alpha) {
	this->is_collision_by_pixel = is_collision_by_alpha;
}

void SpxPhysicsMgr::set_global_gravity(GdFloat gravity) {
	SpxPhysicsDefine::set_global_gravity(gravity);
}

GdFloat SpxPhysicsMgr::get_global_gravity() {
	return SpxPhysicsDefine::get_global_gravity();
}

void SpxPhysicsMgr::set_global_friction(GdFloat friction) {
	SpxPhysicsDefine::set_global_friction(friction);
}

GdFloat SpxPhysicsMgr::get_global_friction() {
	return SpxPhysicsDefine::get_global_friction();
}

void SpxPhysicsMgr::set_global_air_drag(GdFloat air_drag) {
	SpxPhysicsDefine::set_global_air_drag(air_drag);
}

GdFloat SpxPhysicsMgr::get_global_air_drag() {
	return SpxPhysicsDefine::get_global_air_drag();
}


GdArray SpxPhysicsMgr::_check_collision(RID shape, GdVec2 pos, GdInt collision_mask){
	auto node = (Node2D *)get_root();
	PhysicsDirectSpaceState2D *space_state = node->get_world_2d()->get_direct_space_state();
	if (!space_state) {
		return create_array(GD_ARRAY_TYPE_GDOBJ, 0);
	}

	GdVec2 flipped_pos = GdVec2{ pos.x, -pos.y };
	Transform2D query_transform(0, flipped_pos);

	PhysicsDirectSpaceState2D::ShapeParameters params;
	params.shape_rid = shape;
	params.transform = query_transform;
	params.collision_mask = (uint32_t)collision_mask;
	params.collide_with_areas = true;
	params.collide_with_bodies = true;
	params.motion = Vector2(0, 0);
	params.margin = 0.0;

	PhysicsDirectSpaceState2D::ShapeResult results[32];
	int result_count = space_state->intersect_shape(params, results, 32);

	Array resultIds;	
	for (int i = 0; i < result_count; i++) {
		Object *collider_obj = results[i].collider;
		if (collider_obj) {
			SpxSprite *sprite = dynamic_cast<SpxSprite *>(collider_obj);
			if (sprite) {
				resultIds.push_back(sprite->get_gid()); 
			}
		}
	}
	int valid_count = resultIds.size();
	GdArray result_array = create_array(GD_ARRAY_TYPE_GDOBJ, valid_count);
	for(int i=0; i < valid_count; i++){
		auto val=(GdObj)resultIds.get(i);
		set_array(result_array,i,val);
	}
	return result_array;
}
GdArray SpxPhysicsMgr::check_collision_rect(GdVec2 pos, GdVec2 size, GdInt collision_mask) {
	Ref<RectangleShape2D> rect_shape;
	rect_shape.instantiate();
	rect_shape->set_size(size);
	return _check_collision(rect_shape->get_rid(), pos, collision_mask);
}

GdArray SpxPhysicsMgr::check_collision_circle(GdVec2 pos, GdFloat radius, GdInt collision_mask) {
	Ref<CircleShape2D> circle_shape;
	circle_shape.instantiate();
	circle_shape->set_radius(radius); 
	return _check_collision(circle_shape->get_rid(), pos, collision_mask);
}
