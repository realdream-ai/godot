#include "spx_navigation_mgr.h"
#include "core/input/input_event.h"
#include "core/math/color.h"
#include "gdextension_spx_ext.h"
#include "scene/2d/line_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/physics/static_body_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/resources/2d/capsule_shape_2d.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "spx.h"
#include "spx_engine.h"
#include "spx_pen.h"
#include "spx_res_mgr.h"
#include "spx_sprite.h"
#include "spx_draw_tiles.h"
#include "spx_layer_sorter.h"
#include "spx_physic_mgr.h"

#include <cmath>

void SpxNavigationMgr::on_reset() {
	if(path_finder.is_valid()){
		path_finder->reset();
		path_finder = nullptr;
	}
}
void SpxNavigationMgr::setup_path_finder_with_size(GdVec2 grid_size, GdVec2 cell_size, GdBool with_jump, GdBool with_debug) {
	if(path_finder.is_null() || !path_finder.is_valid()){
		path_finder.instantiate();
		path_finder->setup_spx(grid_size, cell_size, with_debug);
		path_finder->set_jumping_enabled(with_jump);
	}
}

void SpxNavigationMgr::setup_path_finder(GdBool with_jump) {
	setup_path_finder_with_size(default_grid_size, default_cell_size, with_jump, false);
}

void SpxNavigationMgr::set_obstacle(GdObj obj, GdBool enabled) {
	if(path_finder.is_valid()){
		path_finder->set_sprite_obstacle(obj, enabled);
	}
}

GdArray SpxNavigationMgr::find_path(GdVec2 p_from, GdVec2 p_to, GdBool with_jump) {
	if(path_finder.is_null() || !path_finder.is_valid()){
		setup_path_finder(with_jump);
	}

	return path_finder->find_path_spx(p_from, p_to);
}