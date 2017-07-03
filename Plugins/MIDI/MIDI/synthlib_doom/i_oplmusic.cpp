//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2014-2015 Alexey Khokholov
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//   System interface for music.
//

#include "i_oplmusic.h"

void DoomOPL::OPL_WriteRegister(unsigned int reg, unsigned char data) {
	opl->fm_writereg(reg, data);
}

void DoomOPL::OPL_InitRegisters(bool opl_new)
{
	unsigned int r;

	// Initialize level registers

	for (r = OPL_REGS_LEVEL; r <= OPL_REGS_LEVEL + OPL_NUM_OPERATORS; ++r)
	{
		OPL_WriteRegister(r, 0x3f);
	}

	// Initialize other registers
	// These two loops write to registers that actually don't exist,
	// but this is what Doom does ...
	// Similarly, the <= is also intenational.

	for (r = OPL_REGS_ATTACK; r <= OPL_REGS_WAVEFORM + OPL_NUM_OPERATORS; ++r)
	{
		OPL_WriteRegister(r, 0x00);
	}

	// More registers ...

	for (r = 1; r < OPL_REGS_LEVEL; ++r)
	{
		OPL_WriteRegister(r, 0x00);
	}

	if (opl_new)
	{
		OPL_WriteRegister(OPL_REG_NEW_MODE, 0x01);
		// Initialize level registers
		for (r = OPL_REGS_LEVEL; r <= OPL_REGS_LEVEL + OPL_NUM_OPERATORS; ++r)
		{
			OPL_WriteRegister(r | 0x100, 0x3f);
		}

		// Initialize other registers
		// These two loops write to registers that actually don't exist,
		// but this is what Doom does ...
		// Similarly, the <= is also intenational.

		for (r = OPL_REGS_ATTACK; r <= OPL_REGS_WAVEFORM + OPL_NUM_OPERATORS; ++r)
		{
			OPL_WriteRegister(r | 0x100, 0x00);
		}

		// More registers ...

		for (r = 1; r < OPL_REGS_LEVEL; ++r)
		{
			OPL_WriteRegister(r | 0x100, 0x00);
		}
	}

	// Re-initialize the low registers:

	// Reset both timers and enable interrupts:
	OPL_WriteRegister(OPL_REG_TIMER_CTRL, 0x60);
	OPL_WriteRegister(OPL_REG_TIMER_CTRL, 0x80);

	// "Allow FM chips to control the waveform of each operator":
	OPL_WriteRegister(OPL_REG_WAVEFORM_ENABLE, 0x20);

	// Keyboard split point on (?)
	OPL_WriteRegister(OPL_REG_FM_MODE, 0x40);
	if (opl_new)
	{
		OPL_WriteRegister(OPL_REG_NEW_MODE, 0x01);
	}

    if (opl_extp)
    {
        OPL_WriteRegister(0x106, 0x17);
    }
}

// Load instrument table from GENMIDI lump:

namespace DoomOPL_inst
{
#include "inst/dmx_dmx.h"
#include "inst/dmx_doom1.h"
#include "inst/dmx_doom2.h"
#include "inst/dmx_raptor.h"
#include "inst/dmx_strife.h"
#include "inst/dmxopl.h"
}

bool DoomOPL::LoadInstrumentTable(unsigned int bank)
{
	const byte *lump;
    
    switch (bank)
    {
        default:
        case 0:
            lump = DoomOPL_inst::dmx_dmx;
            break;
            
        case 1:
            lump = DoomOPL_inst::dmx_doom1;
            break;
            
        case 2:
            lump = DoomOPL_inst::dmx_doom2;
            break;
            
        case 3:
            lump = DoomOPL_inst::dmx_raptor;
            break;
            
        case 4:
            lump = DoomOPL_inst::dmx_strife;
            break;

        case 5:
            lump = DoomOPL_inst::dmxopl;
            break;
    }

    main_instrs = (const genmidi_instr_t *) (lump + strlen(GENMIDI_HEADER));
    percussion_instrs = main_instrs + GENMIDI_NUM_INSTRS;

    return true;
}

// Release a voice back to the freelist.

void DoomOPL::ReleaseVoice(unsigned int id)
{
	opl_voice_t *voice;
    unsigned int i;
	bool doublev;

    // Doom 2 1.666 OPL crash emulation.
    if (id >= voice_alloced_num)
    {
        voice_alloced_num = 0;
        voice_free_num = 0;
        return;
    }
    voice = voice_alloced_list[id];

    VoiceKeyOff(voice);

    voice->channel = NULL;
	voice->note = 0;

	doublev = voice->current_instr_voice != 0;

    // Remove from alloced list.

    voice_alloced_num--;

    for (i = id; i < voice_alloced_num; i++)
    {
        voice_alloced_list[i] = voice_alloced_list[i + 1];
    }

    // Search to the end of the freelist (This is how Doom behaves!)

    voice_free_list[voice_free_num++] = voice;

	if (doublev && opl_drv_ver < opl_doom_1_9)
	{
		ReleaseVoice(id);
	}
}

// Load data to the specified operator

void DoomOPL::LoadOperatorData(int slot, const genmidi_op_t *data,
                             bool max_level, unsigned int *volume)
{
    int level;

    // The scale and level fields must be combined for the level register.
    // For the carrier wave we always set the maximum level.

    level = (data->scale & 0xc0) | (data->level & 0x3f);

    if (max_level)
    {
        level |= 0x3f;
    }
    else
    {
        level |= data->level;
    }

    *volume = level;

    OPL_WriteRegister(OPL_REGS_LEVEL + slot, level);
	OPL_WriteRegister(OPL_REGS_TREMOLO + slot, data->tremolo);
	OPL_WriteRegister(OPL_REGS_ATTACK + slot, data->attack);
	OPL_WriteRegister(OPL_REGS_SUSTAIN + slot, data->sustain);
	OPL_WriteRegister(OPL_REGS_WAVEFORM + slot, data->waveform);
}

// Set the instrument for a particular voice.

void DoomOPL::SetVoiceInstrument(opl_voice_t *voice,
                               const genmidi_instr_t *instr,
                               unsigned int instr_voice)
{
    const genmidi_voice_t *data;
    unsigned int modulating;

    // Instrument already set for this channel?

    if (voice->current_instr == instr
     && voice->current_instr_voice == instr_voice)
    {
        return;
    }

    voice->current_instr = instr;
    voice->current_instr_voice = instr_voice;

    data = &instr->voices[instr_voice];

    // Are we usind modulated feedback mode?

    modulating = (data->feedback & 0x01) == 0;

    // Doom loads the second operator first, then the first.
    // The carrier is set to minimum volume until the voice volume
    // is set in SetVoiceVolume (below).  If we are not using
    // modulating mode, we must set both to minimum volume.

    LoadOperatorData(voice->op2 | voice->array, &data->carrier, true, &voice->car_volume);
	LoadOperatorData(voice->op1 | voice->array, &data->modulator, !modulating, &voice->mod_volume);

    // Set feedback register that control the connection between the
    // two operators.  Turn on bits in the upper nybble; I think this
    // is for OPL3, where it turns on channel A/B.

	OPL_WriteRegister((OPL_REGS_FEEDBACK + voice->index) | voice->array,
                      data->feedback | voice->reg_pan);
	
	// Calculate voice priority.

	voice->priority = 0x0f - (data->carrier.attack >> 4)
					+ 0x0f - (data->carrier.sustain & 0x0f);
}

void DoomOPL::SetVoiceVolume(opl_voice_t *voice, unsigned int volume)
{
    const genmidi_voice_t *opl_voice;
    unsigned int midi_volume;
    unsigned int full_volume;
    unsigned int car_volume;
    unsigned int mod_volume;

    voice->note_volume = volume;

    opl_voice = &voice->current_instr->voices[voice->current_instr_voice];

    // Multiply note volume and channel volume to get the actual volume.

    midi_volume = 2 * (volume_mapping_table[voice->channel->volume] + 1);

    full_volume = (volume_mapping_table[voice->note_volume] * midi_volume) >> 9;

    // The volume value to use in the register:
    car_volume = 0x3f - full_volume;

    // Update the volume register(s) if necessary.

    if (car_volume != (voice->car_volume & 0x3f))
    {
        voice->car_volume = car_volume | (voice->car_volume & 0xc0);

        OPL_WriteRegister((OPL_REGS_LEVEL + voice->op2) | voice->array, voice->car_volume);

        // If we are using non-modulated feedback mode, we must set the
        // volume for both voices.

        if ((opl_voice->feedback & 0x01) != 0 && opl_voice->modulator.level != 0x3f)
        {
            mod_volume = opl_voice->modulator.level;
            if (mod_volume < car_volume)
            {
                mod_volume = car_volume;
            }

            mod_volume |= voice->mod_volume & 0xc0;

            if (mod_volume != voice->mod_volume)
            {
                voice->mod_volume = mod_volume;
                OPL_WriteRegister((OPL_REGS_LEVEL + voice->op1) | voice->array,
                                  voice->mod_volume);
            }
        }
    }
}

void DoomOPL::SetVoicePan(opl_voice_t *voice, unsigned int pan)
{
    const genmidi_voice_t *opl_voice;
    
    voice->reg_pan = pan;
    opl_voice = &voice->current_instr->voices[voice->current_instr_voice];

    OPL_WriteRegister((OPL_REGS_FEEDBACK + voice->index) | voice->array,
                      opl_voice->feedback | pan);
}

void DoomOPL::SetVoicePanEx(opl_voice_t *voice, unsigned int pan)
{
    const genmidi_voice_t *opl_voice;
    
    opl_voice = &voice->current_instr->voices[voice->current_instr_voice];
    
    OPL_WriteRegister(0x107, voice->index + (voice->array * 9 / 256));
    OPL_WriteRegister(0x108, pan * 2);
}

// Initialize the voice table and freelist

void DoomOPL::InitVoices(void)
{
    unsigned int i;
    
    voice_free_num = opl_voices;
    voice_alloced_num = 0;

    // Initialize each voice.

	for (i = 0; i < opl_voices; ++i)
    {
        voices[i].index = i % OPL_NUM_VOICES;
		voices[i].op1 = voice_operators[0][i % OPL_NUM_VOICES];
		voices[i].op2 = voice_operators[1][i % OPL_NUM_VOICES];
		voices[i].array = (i / OPL_NUM_VOICES) << 8;
        voices[i].current_instr = NULL;

        voice_free_list[i] = &voices[i];
    }
}

void DoomOPL::VoiceKeyOff(opl_voice_t *voice)
{
	OPL_WriteRegister((OPL_REGS_FREQ_2 + voice->index) | voice->array, voice->freq >> 8);
}

opl_channel_data_t *DoomOPL::TrackChannelForEvent(unsigned char channel_num)
{
    channel_num = channel_map_table[channel_num];

    return &channels[channel_num];
}

// Get the frequency that we should be using for a voice.

void DoomOPL::KeyOffEvent(unsigned char channel_num, unsigned char key)
{
	opl_channel_data_t *channel;
    unsigned int i;

/*
    printf("note off: channel %i, %i, %i\n",
           event->data.channel.channel,
           event->data.channel.param1,
           event->data.channel.param2);
*/

    channel = TrackChannelForEvent(channel_num);

    // Turn off voices being used to play this key.
    // If it is a double voice instrument there will be two.

    for (i = 0; i < voice_alloced_num;)
	{
		if (voice_alloced_list[i]->channel == channel && voice_alloced_list[i]->key == key)
		{
			// Finished with this voice now.

			ReleaseVoice(i);

            continue;
		}
        i++;
	}
}

// When all voices are in use, we must discard an existing voice to
// play a new note.  Find and free an existing voice.  The channel
// passed to the function is the channel for the new note to be
// played.

void DoomOPL::ReplaceExistingVoice()
{
    unsigned int i;
    unsigned int result;

    // Check the allocated voices, if we find an instrument that is
    // of a lower priority to the new instrument, discard it.
    // If a voice is being used to play the second voice of an instrument,
    // use that, as second voices are non-essential.
    // Lower numbered MIDI channels implicitly have a higher priority
    // than higher-numbered channels, eg. MIDI channel 1 is never
    // discarded for MIDI channel 2.

    result = 0;

    for (i = 0; i < voice_alloced_num; i++)
    {
        if (voice_alloced_list[i]->current_instr_voice != 0
         || voice_alloced_list[i]->channel >= voice_alloced_list[result]->channel)
        {
            result = i;
        }
    }

    ReleaseVoice(result);
}

// Alternate versions of ReplaceExistingVoice() used when emulating old
// versions of the DMX library used in Doom 1.666, Heretic and Hexen.

void DoomOPL::ReplaceExistingVoiceDoom1(void)
{
    int i;
    int result;

    result = 0;

    for (i = 0; i < voice_alloced_num; i++)
    {
        if (voice_alloced_list[i]->channel
          > voice_alloced_list[result]->channel)
        {
            result = i;
        }
    }

    ReleaseVoice(result);
}

void DoomOPL::ReplaceExistingVoiceDoom2(opl_channel_data_t *channel)
{
    unsigned int i;
    unsigned int result;
	unsigned int priority;

	result = 0;

	priority = 0x8000;

    for (i = 0; i < voice_alloced_num - 3; i++)
    {
		if (voice_alloced_list[i]->priority < priority
			&& voice_alloced_list[i]->channel >= channel)
		{
			priority = voice_alloced_list[i]->priority;
			result = i;
		}
	}

	ReleaseVoice(result);
}


unsigned int DoomOPL::FrequencyForVoice(opl_voice_t *voice)
{
    const genmidi_voice_t *gm_voice;
    signed int freq_index;
    unsigned int octave;
    unsigned int sub_index;
    signed int note;

    note = voice->note;

    // Apply note offset.
    // Don't apply offset if the instrument is a fixed note instrument.

    gm_voice = &voice->current_instr->voices[voice->current_instr_voice];

    if ((voice->current_instr->flags & GENMIDI_FLAG_FIXED) == 0)
    {
        note += (signed short) gm_voice->base_note_offset;
    }

    // Avoid possible overflow due to base note offset:

    while (note < 0)
    {
        note += 12;
    }

    while (note > 95)
    {
        note -= 12;
    }

    freq_index = 64 + 32 * note + voice->channel->bend;

    // If this is the second voice of a double voice instrument, the
    // frequency index can be adjusted by the fine tuning field.

    if (voice->current_instr_voice != 0)
    {
        freq_index += (voice->current_instr->fine_tuning / 2) - 64;
    }

    if (freq_index < 0)
    {
        freq_index = 0;
    }

    // The first 7 notes use the start of the table, while
    // consecutive notes loop around the latter part.

    if (freq_index < 284)
    {
        return frequency_curve[freq_index];
    }

    sub_index = (freq_index - 284) % (12 * 32);
    octave = (freq_index - 284) / (12 * 32);

    // Once the seventh octave is reached, things break down.
    // We can only go up to octave 7 as a maximum anyway (the OPL
    // register only has three bits for octave number), but for the
    // notes in octave 7, the first five bits have octave=7, the
    // following notes have octave=6.  This 7/6 pattern repeats in
    // following octaves (which are technically impossible to
    // represent anyway).

    if (octave >= 7)
    {
        octave = 7;
    }

    // Calculate the resulting register value to use for the frequency.

    return frequency_curve[sub_index + 284] | (octave << 10);
}

// Update the frequency that a voice is programmed to use.

void DoomOPL::UpdateVoiceFrequency(opl_voice_t *voice)
{
    unsigned int freq;

    // Calculate the frequency to use for this voice and update it
    // if neccessary.

    freq = FrequencyForVoice(voice);

    if (voice->freq != freq)
    {
		OPL_WriteRegister((OPL_REGS_FREQ_1 + voice->index) | voice->array, freq & 0xff);
		OPL_WriteRegister((OPL_REGS_FREQ_2 + voice->index) | voice->array, (freq >> 8) | 0x20);

        voice->freq = freq;
    }
}

// Program a single voice for an instrument.  For a double voice
// instrument (GENMIDI_FLAG_2VOICE), this is called twice for each
// key on event.

void DoomOPL::VoiceKeyOn(opl_channel_data_t *channel,
                       const genmidi_instr_t *instrument,
                       unsigned int instrument_voice,
                       unsigned int note,
                       unsigned int key,
                       unsigned int volume)
{
    opl_voice_t *voice;
    unsigned int i = 0;

    // Find a voice to use for this new note.

    if (voice_free_num == 0)
    {
        return;
    }

    voice = voice_free_list[0];

    voice_free_num--;

    for (i = 0; i < voice_free_num; ++i)
    {
        voice_free_list[i] = voice_free_list[i + 1];
    }

    voice_alloced_list[voice_alloced_num++] = voice;

    if (!opl_new && opl_drv_ver == opl_doom1_1_666)
    {
        instrument_voice = 0;
    }

    voice->channel = channel;
    voice->key = key;

    // Work out the note to use.  This is normally the same as
    // the key, unless it is a fixed pitch instrument.

    if ((instrument->flags & GENMIDI_FLAG_FIXED) != 0)
    {
        voice->note = instrument->fixed_note;
    }
    else
    {
        voice->note = note;
    }

	voice->reg_pan = channel->pan;

    // Program the voice with the instrument data:

    SetVoiceInstrument(voice, instrument, instrument_voice);

    // Set the volume level.

    SetVoiceVolume(voice, volume);
    
    // Set the extended panning, if necessary
    
    if (opl_extp)
        SetVoicePanEx(voice, channel->panex);

    // Write the frequency value to turn the note on.

    voice->freq = 0;
    UpdateVoiceFrequency(voice);
}

void DoomOPL::KeyOnEvent(unsigned char channel_num, unsigned char key, unsigned char volume)
{
    const genmidi_instr_t *instrument;
    opl_channel_data_t *channel;
	unsigned int note, voicenum;
	bool doublev;

/*
    printf("note on: channel %i, %i, %i\n",
           event->data.channel.channel,
           event->data.channel.param1,
           event->data.channel.param2);
*/

    note = key;

    // A volume of zero means key off. Some MIDI tracks, eg. the ones
    // in AV.wad, use a second key on with a volume of zero to mean
    // key off.
    if (volume <= 0)
    {
        KeyOffEvent(channel_num, key);
        return;
    }

    // The channel.
    channel = TrackChannelForEvent(channel_num);

    // Percussion channel is treated differently.
	if (channel_num == 9)
    {
        if (key < 35 || key > 81)
        {
            return;
        }

        instrument = &percussion_instrs[key - 35];
        note = 60;
    }
    else
    {
        instrument = channel->instrument;
    }

	doublev = ((short)(instrument->flags) & GENMIDI_FLAG_2VOICE) != 0;

    switch (opl_drv_ver)
    {
        case opl_doom1_1_666:
            voicenum = doublev + 1;
            if (!opl_new)
            {
                voicenum = 1;
            }
            while (voice_alloced_num > opl_voices - voicenum)
            {
                ReplaceExistingVoiceDoom1();
            }

            // Find and program a voice for this instrument.  If this
            // is a double voice instrument, we must do this twice.

            if (doublev)
            {
                VoiceKeyOn(channel, instrument, 1, note, key, volume);
            }

            VoiceKeyOn(channel, instrument, 0, note, key, volume);
            break;
        case opl_doom2_1_666:
            if (voice_alloced_num == opl_voices)
            {
                ReplaceExistingVoiceDoom2(channel);
            }
            if (voice_alloced_num == opl_voices - 1 && doublev)
            {
                ReplaceExistingVoiceDoom2(channel);
            }

            // Find and program a voice for this instrument.  If this
            // is a double voice instrument, we must do this twice.

            if (doublev)
            {
                VoiceKeyOn(channel, instrument, 1, note, key, volume);
            }

            VoiceKeyOn(channel, instrument, 0, note, key, volume);
            break;
        default:
        case opl_doom_1_9:
            if (voice_free_num == 0)
            {
                ReplaceExistingVoice();
            }

            // Find and program a voice for this instrument.  If this
            // is a double voice instrument, we must do this twice.

            VoiceKeyOn(channel, instrument, 0, note, key, volume);

            if (doublev)
            {
                VoiceKeyOn(channel, instrument, 1, note, key, volume);
            }
            break;
    }
}

void DoomOPL::ProgramChangeEvent(unsigned char channel_num, unsigned char instrument)
{
    opl_channel_data_t *channel;

    // Set the instrument used on this channel.

    channel = TrackChannelForEvent(channel_num);
    channel->instrument = &main_instrs[instrument];

    // TODO: Look through existing voices that are turned on on this
    // channel, and change the instrument.
}

void DoomOPL::SetChannelVolume(opl_channel_data_t *channel, unsigned int volume)
{
    unsigned int i;

    channel->volume = volume;

    // Update all voices that this channel is using.

    for (i = 0; i < voice_alloced_num; ++i)
    {
        if (voice_alloced_list[i]->channel == channel)
        {
            SetVoiceVolume(voice_alloced_list[i], voice_alloced_list[i]->note_volume);
        }
    }
}

void DoomOPL::SetChannelPan(opl_channel_data_t *channel, unsigned int pan)
{
    unsigned int reg_pan;
    unsigned int i;

    if (opl_new)
    {
        if (opl_extp)
        {
            if (channel->panex != pan)
            {
                channel->panex = pan;
                for (i = 0; i < voice_alloced_num; i++)
                {
                    if (voice_alloced_list[i]->channel == channel)
                    {
                        SetVoicePanEx(voice_alloced_list[i], pan);
                    }
                }
            }
        }
        else
        {
            if (pan >= 96)
            {
                reg_pan = 0x10;
            }
            else if (pan <= 48)
            {
                reg_pan = 0x20;
            }
            else
            {
                reg_pan = 0x30;
            }
            if (channel->pan != reg_pan)
            {
                channel->pan = reg_pan;
                for (i = 0; i < voice_alloced_num; i++)
                {
                    if (voice_alloced_list[i]->channel == channel)
                    {
                        SetVoicePan(voice_alloced_list[i], reg_pan);
                    }
                }
            }
        }
    }
}

// Handler for the MIDI_CONTROLLER_ALL_NOTES_OFF channel event.
void DoomOPL::AllNotesOff(opl_channel_data_t *channel, unsigned int param)
{
    unsigned int i;

    for (i = 0; i < voice_alloced_num;)
    {
        if (voice_alloced_list[i]->channel == channel)
        {
            // Finished with this voice now.

            ReleaseVoice(i);
            continue;
        }
        i++;
    }
}

void DoomOPL::ControllerEvent(unsigned char channel_num, unsigned char controller, unsigned char param)
{
    opl_channel_data_t *channel;

/*
    printf("change controller: channel %i, %i, %i\n",
           event->data.channel.channel,
           event->data.channel.param1,
           event->data.channel.param2);
*/

    channel = TrackChannelForEvent(channel_num);

    switch (controller)
    {
        case MIDI_CONTROLLER_MAIN_VOLUME:
            SetChannelVolume(channel, param);
            break;

        case MIDI_CONTROLLER_PAN:
            SetChannelPan(channel, param);
            break;

        case MIDI_CONTROLLER_ALL_NOTES_OFF:
            AllNotesOff(channel, param);
            break;

        default:
            break;
    }
}

// Process a pitch bend event.

void DoomOPL::PitchBendEvent(unsigned char channel_num, unsigned char bend)
{
    opl_channel_data_t *channel;
    unsigned int i;
    opl_voice_t *voice_updated_list[OPL_NUM_VOICES * 2];
    unsigned int voice_updated_num = 0;
    opl_voice_t *voice_not_updated_list[OPL_NUM_VOICES * 2];
    unsigned int voice_not_updated_num = 0;

    // Update the channel bend value.  Only the MSB of the pitch bend
    // value is considered: this is what Doom does.

	channel = TrackChannelForEvent(channel_num);
    channel->bend = bend - 64;

    // Update all voices for this channel.

	for (i = 0; i < voice_alloced_num; ++i)
    {
        if (voice_alloced_list[i]->channel == channel)
        {
            UpdateVoiceFrequency(voice_alloced_list[i]);
            voice_updated_list[voice_updated_num++] = voice_alloced_list[i];
        }
        else
        {
            voice_not_updated_list[voice_not_updated_num++] = voice_alloced_list[i];
        }
    }

    for (i = 0; i < voice_not_updated_num; i++)
    {
        voice_alloced_list[i] = voice_not_updated_list[i];
    }

    for (i = 0; i < voice_updated_num; i++)
    {
        voice_alloced_list[i + voice_not_updated_num] = voice_updated_list[i];
    }
}

// Process a MIDI event from a track.

void DoomOPL::midi_write(unsigned int data)
{
	unsigned char event_type = data & 0xf0;
	unsigned char channel_num = data & 0x0f;
	unsigned char key = (data >> 8) & 0xff;
	unsigned char volume = (data >> 16) & 0xff;
	if (key > 0x7f) {
		key = 0x7f;
	}
	if (volume > 0x7f) {
		volume = 0x7f;
	}
    switch (event_type)
    {
        case MIDI_EVENT_NOTE_OFF:
            KeyOffEvent(channel_num, key);
            break;

        case MIDI_EVENT_NOTE_ON:
			KeyOnEvent(channel_num, key, volume);
            break;

        case MIDI_EVENT_CONTROLLER:
			ControllerEvent(channel_num, key, volume);
            break;

        case MIDI_EVENT_PROGRAM_CHANGE:
			ProgramChangeEvent(channel_num, key);
            break;

        case MIDI_EVENT_PITCH_BEND:
			PitchBendEvent(channel_num, volume);
            break;

        default:
            break;
    }
}

// Initialize a channel.

void DoomOPL::InitChannel(opl_channel_data_t *channel)
{
    // TODO: Work out sensible defaults?

    channel->instrument = &main_instrs[0];
	channel->volume = 127;
	channel->pan = 0x30;
    channel->panex = 64;
    channel->bend = 0;
}

int DoomOPL::midi_init(unsigned int rate, unsigned int bank, unsigned int extp)
{
	/*char *env;*/
	unsigned int i;
	
	opl = getchip();
	if (!opl || !opl->fm_init(rate))
	{
		return 0;
	}
    
    opl_extp = !!extp;
    
	memset(channels, 0, sizeof(channels));
	main_instrs = NULL;
	percussion_instrs = NULL;
	memset(voices, 0, sizeof(voices));
	voice_alloced_num = 0;
    voice_free_num = 0;
	opl_new = 0;
	opl_voices = OPL_NUM_VOICES;
	opl_drv_ver = opl_doom_1_9;

	/*env = getenv("DMXOPTION");
	if (env)
	{
		if (strstr(env, "-opl3"))*/
		{
			opl_new = 1;
			opl_voices = OPL_NUM_VOICES * 2;
		}/*
		if (strstr(env, "-doom1"))
		{
			opl_drv_ver = opl_doom1_1_666;
		}
	if (strstr(env, "-doom2"))
	{
		opl_drv_ver = opl_doom2_1_666;
	}
	}*/

	OPL_InitRegisters(opl_new);

    // Load instruments from GENMIDI lump:

    if (!LoadInstrumentTable(bank))
    {
        return 0;
    }

	for (i = 0; i < MIDI_CHANNELS_PER_TRACK; i++) {
		InitChannel(&channels[i]);
	}

    InitVoices();

    return 1;
}

void DoomOPL::midi_generate(signed short *buffer, unsigned int length) {
	opl->fm_generate(buffer, length);
}

const char * DoomOPL::midi_synth_name(void)
{
    return "DoomOPL";
}

unsigned int DoomOPL::midi_bank_count(void)
{
    return 6;
}

const char * DoomOPL::midi_bank_name(unsigned int bank)
{
    switch (bank)
    {
        default:
        case 0:
            return "DMX Default";
            
        case 1:
            return "DMX Doom 1";
            
        case 2:
            return "DMX Doom 2";
            
        case 3:
            return "DMX Raptor";
            
        case 4:
            return "DMX Strife";

        case 5:
            return "DMXOPL";
    }
}

midisynth *getsynth_doom() {
	DoomOPL *synth = new DoomOPL;
	return synth;
}
