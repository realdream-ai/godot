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

#include "servers/physics_server_2d.h"
#include "servers/physics_server_3d.h"

// physic
void SpxPhysicMgr::set_gravity(GdFloat gravity) {
	print_line(vformat("Setting gravity  %f", gravity));
	// TODO @jiepengtan implement this
}

GdFloat SpxPhysicMgr::get_gravity() {
	// TODO @jiepengtan implement this
	return 9.8;
}

void SpxPhysicMgr::set_velocity(GdObj obj, GdVec2 velocity) {
	print_line(vformat("Setting velocity of %d to %f, %f", obj, velocity.x, velocity.y));
}

GdVec2 SpxPhysicMgr::get_velocity(GdObj obj) {
	return GdVec2();
}

void SpxPhysicMgr::set_mass(GdObj obj, GdFloat mass) {
	print_line(vformat("Setting mass of %d to %f", obj, mass));
}

GdFloat SpxPhysicMgr::get_mass(GdObj obj) {
	return 0;
}

void SpxPhysicMgr::add_force(GdObj obj, GdVec2 force) {
	print_line(vformat("Adding force to %d %f, %f", obj, force.x, force.y));
}

void SpxPhysicMgr::add_impulse(GdObj obj, GdVec2 impulse) {
	print_line(vformat("Adding impulse to %d %f, %f", obj, impulse.x, impulse.y));
}

void SpxPhysicMgr::set_collision_layer(GdObj obj, GdInt layer) {
	print_line("");
}

GdInt SpxPhysicMgr::get_collision_layer(GdObj obj) {
	return 0;
}

void SpxPhysicMgr::set_collision_mask(GdObj obj, GdInt mask) {
	print_line(vformat("Setting collision mask of %d to %d", obj, mask));
}

GdInt SpxPhysicMgr::get_collision_mask(GdObj obj) {
	return 0;
}

GdInt SpxPhysicMgr::get_collider_type(GdObj obj) {
	return 0;
}

void SpxPhysicMgr::add_collider_rect(GdObj obj, GdVec2 center, GdVec2 size) {
	print_line("");
}

void SpxPhysicMgr::add_collider_circle(GdObj obj, GdVec2 center, GdFloat radius) {
	print_line("");
}

void SpxPhysicMgr::add_collider_capsule(GdObj obj, GdVec2 center, GdVec2 size) {
	print_line("");
}

void SpxPhysicMgr::set_trigger(GdObj obj, GdBool trigger) {
	print_line(vformat("Setting trigger of %d to %d", obj, trigger));
}

GdBool SpxPhysicMgr::is_trigger(GdObj obj) {
	return false;
}

void SpxPhysicMgr::set_collision_enabled(GdObj obj, GdBool enabled) {
	print_line("");
}

GdBool SpxPhysicMgr::is_collision_enabled(GdObj obj) {
	return false;
}
