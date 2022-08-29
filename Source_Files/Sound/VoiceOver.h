#ifndef __VOICEOVER_H
#define __VOICEOVER_H

/*

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

*/

// NOTE: mostly a ripoff of Music, just the bits needed to make it work
#include "Decoder.h"
#include "FileHandler.h"
#include "SoundManager.h"

struct TerminalVoiceInfo
{
	std::vector<std::string> groups;
};

struct LevelVoiceInfo
{
	std::vector<TerminalVoiceInfo> terminals;
};

struct ScenarioVoiceInfo
{
	std::vector<LevelVoiceInfo> levels;
};

struct VoiceOverIndex
{
	int16 level = 0;
	int16 terminal = 0;
	int16 group = 0;
};

class InfoTree;

class VoiceOver
{
public:
	static VoiceOver* instance() {
		static VoiceOver* m_instance = nullptr;
		if (!m_instance)
			m_instance = new VoiceOver();
		return m_instance;
	}

	void Open(const std::string& name);
	void Close();
	void Pause();
	void Play();
	bool Playing();
	void Rewind();
	void Restart();
	bool FillBuffer();
	
	void LoadVoiceOver(VoiceOverIndex index);
	void StopVoiceOver();

	void parse_mml_voiceover(const InfoTree& root);
private:
	VoiceOver();
	bool Load(const std::string& file);

	float GetVolumeLevel() { return SoundManager::instance()->parameters.music_db; } // FIXME: have separate volume control!

	static const int VO_BUFFER_SIZE = 1024;

	std::vector<uint8> vo_buffer;
	StreamDecoder* decoder;

	// info about the vo's format
	bool sixteen_bit;
	bool stereo;
	bool signed_8bit;
	int bytes_per_frame;
	_fixed rate;
	bool little_endian;

	std::string vo_file;

	bool vo_initialized;
	bool vo_play;

	ScenarioVoiceInfo voice_info;
};


#endif