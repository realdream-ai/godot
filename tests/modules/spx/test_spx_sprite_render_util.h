/**************************************************************************/
/*  test_spx_sprite_render_util.h                                         */
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

#ifndef TEST_SPX_SPRITE_RENDER_UTIL_H
#define TEST_SPX_SPRITE_RENDER_UTIL_H

#include "modules/spx/spx_sprite_render_util.h"
#include "tests/test_macros.h"

namespace TestSpxSpriteRenderUtil {

TEST_CASE("[SPX] Render root preserves single-image render offset") {
	const Vector2 render_offset(12.0, -8.0);
	const Vector2 base_offset(1.0, 2.0);
	const Vector2 frame_offset(3.0, 4.0);
	const Vector2 render_scale(2.0, 0.5);

	const Vector2 anim_offset = spx_compute_anim_offset(true, base_offset, render_offset, frame_offset, render_scale);
	CHECK(render_offset + anim_offset == render_offset + base_offset + frame_offset * render_scale);
}

TEST_CASE("[SPX] Render root cancels costume offset for animation frames") {
	const Vector2 render_offset(12.0, -8.0);
	const Vector2 base_offset(1.0, 2.0);
	const Vector2 frame_offset(3.0, 4.0);
	const Vector2 render_scale(2.0, 0.5);

	const Vector2 anim_offset = spx_compute_anim_offset(false, base_offset, render_offset, frame_offset, render_scale);
	CHECK(render_offset + anim_offset == base_offset + frame_offset * render_scale);
}

} // namespace TestSpxSpriteRenderUtil

#endif // TEST_SPX_SPRITE_RENDER_UTIL_H
