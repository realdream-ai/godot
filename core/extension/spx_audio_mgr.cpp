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

#include "modules/minimp3/audio_stream_mp3.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "spx_engine.h"
#include "spx_res_mgr.h"

const int BUS_MASTER = 0;
const int BUS_SFX = 1;
const int BUS_MUSIC = 2;
const StringName STR_BUS_MASTER = "Master";
const StringName STR_BUS_SFX = "Master";
const StringName STR_BUS_MUSIC = "Master";

void SpxAudioMgr::on_awake() {
	SpxBaseMgr::on_awake();
	// add default bus : music and sfx
	AudioServer::get_singleton()->set_bus_count(3);
	AudioServer::get_singleton()->set_bus_name(BUS_SFX, STR_BUS_SFX);
	AudioServer::get_singleton()->set_bus_name(BUS_MUSIC, STR_BUS_MUSIC);

	AudioServer::get_singleton()->set_bus_send(BUS_SFX, STR_BUS_MASTER);
	AudioServer::get_singleton()->set_bus_send(BUS_MUSIC, STR_BUS_MASTER);

	music = memnew(AudioStreamPlayer2D);
	music->set_bus(STR_BUS_MASTER);
	owner->add_child(music);
}

void SpxAudioMgr::stop_all() {
	for (List<AudioStreamPlayer2D *>::Element *item = audios.front(); item;) {
		item->get()->queue_free();
		item = item->next();
	}
	audios.clear();
	if (music) {
		pause_music();
	}
}

void SpxAudioMgr::on_destroy() {
	stop_all();
	if (music) {
		music->queue_free();
		music = nullptr;
	}
	SpxBaseMgr::on_destroy();
}
void SpxAudioMgr::on_update(float delta) {
	SpxBaseMgr::on_update(delta);

	// check the audio is done
	for (auto item = audios.front(); item;) {
		const auto audio = item->get();
		auto *next = item->next();
		if (!audio->is_playing()) {
			audio->queue_free();
			audios.erase(item);
		}
		item = next;
	}
}

void SpxAudioMgr::play_sfx(GdString path) {
	auto path_str = SpxStr(path);
	Ref<AudioStream> stream = resMgr->load_audio(path_str);

	auto audio = memnew(AudioStreamPlayer2D);
	audio->set_bus(STR_BUS_SFX);
	owner->add_child(audio);
	audio->set_stream(stream);
	audio->play();
	audio->set_name(SpxStr(path));
	audios.push_back(audio);
}

GdBool SpxAudioMgr::is_music_playing() {
	return music->is_playing();
}

void SpxAudioMgr::play_music(GdString path) {
	auto path_str = SpxStr(path);
	Ref<AudioStream> stream = resMgr->load_audio(path_str);

	// set loop
	Ref<AudioStreamMP3> mp3_stream = stream;
	if (mp3_stream.is_valid()) {
		mp3_stream->set_loop(true);
	}

	music->set_stream(stream);
	music->play();
}

void SpxAudioMgr::pause_music() {
	music->set_stream_paused(true);
}

void SpxAudioMgr::resume_music() {
	music->set_stream_paused(false);
}

GdFloat SpxAudioMgr::get_music_timer() {
	return music->get_playback_position();
}

void SpxAudioMgr::set_music_timer(GdFloat time) {
	music->seek(time);
}

void SpxAudioMgr::set_volume(int bus, GdFloat volume) {
	auto dbval = Math::linear_to_db(volume);
	AudioServer::get_singleton()->set_bus_volume_db(bus, dbval);
}
GdFloat SpxAudioMgr::get_volume(int bus) {
	auto dbval = AudioServer::get_singleton()->get_bus_volume_db(bus);
	return Math::linear_to_db(dbval);
}

void SpxAudioMgr::set_sfx_volume(GdFloat volume) {
	set_volume(BUS_SFX, volume);
}

GdFloat SpxAudioMgr::get_sfx_volume() {
	return get_volume(BUS_SFX);
}

void SpxAudioMgr::set_music_volume(GdFloat volume) {
	set_volume(BUS_MUSIC, volume);
}

GdFloat SpxAudioMgr::get_music_volume() {
	return get_volume(BUS_MUSIC);
}

void SpxAudioMgr::set_master_volume(GdFloat volume) {
	set_volume(BUS_MASTER, volume);
}
GdFloat SpxAudioMgr::get_master_volume() {
	return get_volume(BUS_MASTER);
}
