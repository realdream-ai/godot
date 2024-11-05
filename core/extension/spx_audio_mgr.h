/**************************************************************************/
/*  spx_audio_mgr.h                                                       */
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

#ifndef SPX_AUDIO_MGR_H
#define SPX_AUDIO_MGR_H

#include "gdextension_spx_ext.h"
#include "spx_base_mgr.h"

class AudioStreamPlayer2D;

class SpxAudioMgr : SpxBaseMgr {
	SPXCLASS(SpxAudio, SpxBaseMgr)
private:
	List<AudioStreamPlayer2D*> audios;
	AudioStreamPlayer2D *music;

public:
	void on_awake() override;
	void on_destroy() override;
	void on_update(float delta) override;
private:
	void set_volume(int bus,GdFloat volume);
	GdFloat get_volume(int bus);
public:
	void stop_all();

	void play_sfx(GdString path);

	void play_music(GdString path);
	void pause_music();
	void resume_music();
	GdFloat get_music_timer();
	void set_music_timer(GdFloat time);
	GdBool is_music_playing();

	void set_sfx_volume(GdFloat volume);
	GdFloat get_sfx_volume();
	void set_music_volume(GdFloat volume);
	GdFloat get_music_volume();
	void set_master_volume(GdFloat volume);
	GdFloat get_master_volume();
};

#endif // SPX_AUDIO_MGR_H
