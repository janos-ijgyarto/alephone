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

#include "VoiceOver.h"
#include "Mixer.h"
#include "InfoTree.h"

#include <utility>

void VoiceOver::Open(const std::string& name)
{
	if (vo_initialized)
	{
		if (!name.empty() && (name == vo_file))
		{
			Rewind();
			return;
		}

		Close();
	}

	if (!name.empty())
	{
		vo_initialized = Load(name);
		vo_file = name;
	}
}

void VoiceOver::Close()
{
	if (vo_initialized)
	{
		vo_initialized = false;
		Pause();
		delete decoder;
		decoder = 0;
	}
}

void VoiceOver::Pause()
{
	if (!SoundManager::instance()->IsInitialized() || !SoundManager::instance()->IsActive()) return;
	Mixer::instance()->StopVOChannel();
}

void VoiceOver::Play()
{
	if (!vo_initialized || !SoundManager::instance()->IsInitialized() || !SoundManager::instance()->IsActive()) return;
	if (FillBuffer()) {
		// let the mixer handle it
		Mixer::instance()->StartVOChannel(sixteen_bit, stereo, signed_8bit, bytes_per_frame, rate, little_endian);
	}
}

void VoiceOver::Rewind()
{
	if (decoder)
		decoder->Rewind();
}

bool VoiceOver::FillBuffer()
{
	if (GetVolumeLevel() <= SoundManager::MINIMUM_VOLUME_DB) return false;

	if (!decoder) return false;
	int32 bytes_read = decoder->Decode(&vo_buffer.front(), VO_BUFFER_SIZE);
	if (bytes_read)
	{
		Mixer::instance()->UpdateVOChannel(&vo_buffer.front(), bytes_read);
		return true;
	}

	// Failed
	return false;
}

void VoiceOver::LoadVoiceOver(VoiceOverIndex index)
{
	if (index.level < voice_info.levels.size())
	{
		const LevelVoiceInfo& level_info = voice_info.levels[index.level];
		if (index.terminal < level_info.terminals.size())
		{
			const TerminalVoiceInfo& terminal_info = level_info.terminals[index.terminal];
			if (index.group < terminal_info.groups.size())
			{
				const std::string& vo_file_name = terminal_info.groups[index.group];

				if (!vo_file_name.empty())
				{
					Open(vo_file_name);
				}
			}
		}
	}
}

void VoiceOver::StopVoiceOver()
{
	vo_play = false;
	Close();
}

void VoiceOver::parse_mml_voiceover(const InfoTree& root)
{
	for (const InfoTree::value_type& v : root)
	{
		for (const InfoTree& voice : root.children_named("voice"))
		{
			int16 level_index, terminal_index, group_index;
			std::string sound;

			voice.read_attr("level", level_index);
			voice.read_attr("terminal", terminal_index);
			voice.read_attr("group", group_index);
			voice.read_attr("sound", sound);

			if (voice_info.levels.size() <= level_index)
			{
				voice_info.levels.resize(level_index + 1);
			}

			LevelVoiceInfo& level_info = voice_info.levels[level_index];

			if (level_info.terminals.size() <= terminal_index)
			{
				level_info.terminals.resize(terminal_index + 1);
			}

			TerminalVoiceInfo& terminal_info = level_info.terminals[terminal_index];

			if (terminal_info.groups.size() <= group_index)
			{
				terminal_info.groups.resize(group_index + 1);
			}

			terminal_info.groups[group_index] = sound;
		}
	}
}

VoiceOver::VoiceOver() :
	vo_initialized(false),
	vo_play(false),
	decoder(0)
{
	vo_buffer.resize(VO_BUFFER_SIZE);
}

bool VoiceOver::Load(const std::string& file)
{
	FileSpecifier vo_file(get_data_path(kPathLocalData) + "\\" + file);

	delete decoder;
	decoder = StreamDecoder::Get(vo_file);

	if (decoder)
	{
		sixteen_bit = decoder->IsSixteenBit();
		stereo = decoder->IsStereo();
		signed_8bit = decoder->IsSigned();
		bytes_per_frame = decoder->BytesPerFrame();
		rate = (_fixed)((decoder->Rate() / Mixer::instance()->obtained.freq) * (1 << FIXED_FRACTIONAL_BITS));
		little_endian = decoder->IsLittleEndian();

		return true;

	}
	else
	{
		return false;
	}
}