/**************************************************************************/
/*  spx_audio_mgr.cpp                                                     */
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

#include "spx_audio_mgr.h"
#include "scene/2d/audio_stream_player_2d.h"

void SpxAudioMgr::on_start() {
	SpxBaseMgr::on_start();
	audio = memnew(AudioStreamPlayer2D);
	owner->add_child(audio);
	music = memnew(AudioStreamPlayer2D);
	owner->add_child(music);
}

void SpxAudioMgr::on_destroy() {
	if (audio) {
		audio->queue_free();
		audio = nullptr;
	}
	if (music) {
		music->queue_free();
		music = nullptr;
	}
	SpxBaseMgr::on_destroy();
}

void SpxAudioMgr::play_audio(GdString path) {
	Ref<Resource> res = ResourceLoader::load(SpxStr(path));
	ERR_FAIL_COND(res.is_null());
	ERR_FAIL_COND(!res->is_class("AudioStream"));

	Ref<AudioStream> stream = res;
	audio->set_stream(stream);
	audio->play();
}

void SpxAudioMgr::set_audio_volume(GdFloat volume) {
	audio->set_volume_db(volume);
}

GdFloat SpxAudioMgr::get_audio_volume() {
	return audio->get_volume_db();
}

GdBool SpxAudioMgr::is_music_playing() {
	return music->is_playing();
}

void SpxAudioMgr::play_music(GdString path) {
	Ref<Resource> res = ResourceLoader::load(SpxStr(path));
	ERR_FAIL_COND(res.is_null());
	ERR_FAIL_COND(!res->is_class("AudioStream"));
	Ref<AudioStream> stream = res;
	music->set_stream(stream);
	music->play();
}

void SpxAudioMgr::set_music_volume(GdFloat volume) {
	music->set_volume_db(volume);
}

GdFloat SpxAudioMgr::get_music_volume() {
	return music->get_volume_db();
}

void SpxAudioMgr::pause_music() {
	music->stop();
}

void SpxAudioMgr::resume_music() {
	music->play();
}

GdFloat SpxAudioMgr::get_music_timer() {
	return music->get_playback_position();
}

void SpxAudioMgr::set_music_timer(GdFloat time) {
	music->seek(time);
}
