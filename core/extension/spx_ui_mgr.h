/**************************************************************************/
/*  spx_ui_mgr.h                                                          */
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

#ifndef SPX_UI_MGR_H
#define SPX_UI_MGR_H

#include "gdextension_spx_ext.h"
#include "spx_engine.h"

class SpxUIMgr : SpxBaseMgr {
	SPXCLASS(SpxUI, SpxBaseMgr)
public:
	GdInt create_button(GdString path, GdRect2 rect, GdString text);
	GdInt create_label(GdString path, GdRect2 rect, GdString text);
	GdInt create_image(GdString path, GdRect2 rect, GdColor color);
	GdInt create_slider(GdString path, GdRect2 rect, GdFloat value);
	GdInt create_toggle(GdString path, GdRect2 rect, GdBool value);
	GdInt create_input(GdString path, GdRect2 rect, GdString text);
	GdInt get_type(GdObj obj);
	void set_interactable(GdObj obj, GdBool interactable);
	GdBool get_interactable(GdObj obj);
	void set_text(GdObj obj, GdString text);
	GdString get_text(GdObj obj);
	void set_rect(GdObj obj, GdRect2 rect);
	GdRect2 get_rect(GdObj obj);
	void set_color(GdObj obj, GdColor color);
	GdColor get_color(GdObj obj);
	void set_font_size(GdObj obj, GdFloat size);
	GdFloat get_font_size(GdObj obj);
	void set_visible(GdObj obj, GdBool visible);
	GdBool get_visible(GdObj obj);
};

#endif // SPX_UI_MGR_H
