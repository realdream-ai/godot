/**************************************************************************/
/*  spx_physic_mgr.cpp                                                    */
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

#include "spx_physic_mgr.h"

#include "scene/resources/world_2d.h"
#include "servers/physics_server_2d.h"
#include "servers/physics_server_3d.h"
#include "spx_sprite.h"
#include "spx_sprite_mgr.h"

#include "scene/2d/camera_2d.h"
#include "scene/2d/collision_shape_2d.h"
#include "scene/main/window.h"
#include "scene/resources/rectangle_shape_2d.h"
#include "spx_engine.h"
#include "spx_sprite_mgr.h"

#define spriteMgr SpxEngine::get_singleton()->get_sprite()

GdObj SpxPhysicMgr::raycast(GdVec2 from, GdVec2 to, GdInt collision_mask) {
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

GdBool SpxPhysicMgr::check_collision(GdVec2 from, GdVec2 to, GdInt collision_mask, GdBool collide_with_areas, GdBool collide_with_bodies) {
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
const GdInt BOUND_CAM_ALL = BOUND_CAM_LEFT | BOUND_CAM_TOP | BOUND_CAM_TOP | BOUND_CAM_BOTTOM;

GdInt SpxPhysicMgr::check_touched_camera_boundaries(GdObj obj) {
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
	Vector2 half_size = (viewport_size * zoom) * 0.5;
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
GdBool SpxPhysicMgr::check_touched_camera_boundary(GdObj obj, GdInt board_type) {
	auto result = check_touched_camera_boundaries(obj);
	return (result & board_type) != 0;
}
