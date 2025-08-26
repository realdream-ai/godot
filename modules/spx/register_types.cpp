#include "register_types.h"
#include "spx_mass_spring.h"
#include "spx.h"

void initialize_spx_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<MassSpring2D>();
}

void uninitialize_spx_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
