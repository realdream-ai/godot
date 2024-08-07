/**************************************************************************/
/*  spx_sprite_mgr.h                                                      */
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

#ifndef SPX_SPRITE_MGR_H
#define SPX_SPRITE_MGR_H

#include "gdextension_spx_ext.h"
#include "spx_base_mgr.h"

class SpxSprite;
#define NULL_SPRITE_ID 0
class SpxSpriteMgr : SpxBaseMgr {
	SPXCLASS(SpxSprite, SpxBaseMgr)
private:
	RBMap<GdObj, SpxSprite *> id_sprites;

public:
	void on_start() override;
	void on_destroy() override;
	void on_update(float delta) override;

	SpxSprite *get_sprite(GdObj obj);
	void on_sprite_destroy(SpxSprite *sprite);

public:
	GdInt create_sprite(GdString path);
	GdInt clone_sprite(GdObj obj);
	GdBool destroy_sprite(GdObj obj);
	GdBool is_sprite_alive(GdObj obj);
	void set_position(GdObj obj, GdVec2 pos);
	void set_rotation(GdObj obj, float rot);
	void set_scale(GdObj obj, GdVec2 scale);
	GdVec2 get_position(GdObj obj);
	GdFloat get_rotation(GdObj obj);
	GdVec2 get_scale(GdObj obj);
	void set_color(GdObj obj, GdColor color);
	GdColor get_color(GdObj obj);
	void update_texture(GdObj obj, GdString path);
	GdString get_texture(GdObj obj);
	void set_visible(GdObj obj, GdBool visible);
	GdBool get_visible(GdObj obj);
	void update_z_index(GdObj obj, GdInt z);
};

#endif // SPX_SPRITE_MGR_H
