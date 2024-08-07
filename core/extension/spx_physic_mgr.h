/**************************************************************************/
/*  spx_physic_mgr.h                                                      */
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

#ifndef SPX_PHYSIC_MGR_H
#define SPX_PHYSIC_MGR_H

#include "gdextension_spx_ext.h"
#include "spx_base_mgr.h"

class SpxPhysicMgr : SpxBaseMgr {
	SPXCLASS(SpxPhysic, SpxBaseMgr)
public:
	void set_gravity(GdFloat gravity);
	GdFloat get_gravity();
	void set_velocity(GdObj obj, GdVec2 velocity);
	GdVec2 get_velocity(GdObj obj);
	void set_mass(GdObj obj, GdFloat mass);
	GdFloat get_mass(GdObj obj);
	void add_force(GdObj obj, GdVec2 force);
	void add_impulse(GdObj obj, GdVec2 impulse);
	void set_collision_layer(GdObj obj, GdInt layer);
	GdInt get_collision_layer(GdObj obj);
	void set_collision_mask(GdObj obj, GdInt mask);
	GdInt get_collision_mask(GdObj obj);
	GdInt get_collider_type(GdObj obj);
	void add_collider_rect(GdObj obj, GdVec2 center, GdVec2 size);
	void add_collider_circle(GdObj obj, GdVec2 center, GdFloat radius);
	void add_collider_capsule(GdObj obj, GdVec2 center, GdVec2 size);
	void set_trigger(GdObj obj, GdBool trigger);
	GdBool is_trigger(GdObj obj);
	void set_collision_enabled(GdObj obj, GdBool enabled);
	GdBool is_collision_enabled(GdObj obj);
};

#endif // SPX_PHYSIC_MGR_H
