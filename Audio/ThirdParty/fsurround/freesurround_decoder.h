/*
Copyright (C) 2007-2010 Christian Kothe

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef FREESURROUND_DECODER_H
#define FREESURROUND_DECODER_H

/**
 * Identifiers for the supported output channels (from front to back, left to right).
 * The ordering here also determines the ordering of interleaved samples in the output signal.
 */
typedef enum channel_id {
	ci_none = 0,
	ci_front_left = 1 << 1,
	ci_front_center_left = 1 << 2,
	ci_front_center = 1 << 3,
	ci_front_center_right = 1 << 4,
	ci_front_right = 1 << 5,
	ci_side_front_left = 1 << 6,
	ci_side_front_right = 1 << 7,
	ci_side_center_left = 1 << 8,
	ci_side_center_right = 1 << 9,
	ci_side_back_left = 1 << 10,
	ci_side_back_right = 1 << 11,
	ci_back_left = 1 << 12,
	ci_back_center_left = 1 << 13,
	ci_back_center = 1 << 14,
	ci_back_center_right = 1 << 15,
	ci_back_right = 1 << 16,
	ci_lfe = 1 << 31
} channel_id;

/**
 * The supported output channel setups.
 * A channel setup is defined by the set of channels that are present. Here is a graphic
 * of the cs_5point1 setup: http://en.wikipedia.org/wiki/File:5_1_channels_(surround_sound)_label.svg
 */
typedef enum channel_setup {
	cs_stereo = ci_front_left | ci_front_right | ci_lfe,
	cs_3stereo = ci_front_left | ci_front_center | ci_front_right | ci_lfe,
	cs_5stereo = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right | ci_lfe,
	cs_4point1 = ci_front_left | ci_front_right | ci_back_left | ci_back_right | ci_lfe,
	cs_5point1 = ci_front_left | ci_front_center | ci_front_right | ci_back_left | ci_back_right | ci_lfe,
	cs_6point1 = ci_front_left | ci_front_center | ci_front_right | ci_side_center_left | ci_side_center_right | ci_back_center | ci_lfe,
	cs_7point1 = ci_front_left | ci_front_center | ci_front_right | ci_side_center_left | ci_side_center_right | ci_back_left | ci_back_right | ci_lfe,
	cs_7point1_panorama = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	                      ci_side_center_left | ci_side_center_right | ci_lfe,
	cs_7point1_tricenter = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	                       ci_back_left | ci_back_right | ci_lfe,
	cs_8point1 = ci_front_left | ci_front_center | ci_front_right | ci_side_center_left | ci_side_center_right |
	             ci_back_left | ci_back_center | ci_back_right | ci_lfe,
	cs_9point1_densepanorama = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	                           ci_side_front_left | ci_side_front_right | ci_side_center_left | ci_side_center_right | ci_lfe,
	cs_9point1_wrap = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	                  ci_side_center_left | ci_side_center_right | ci_back_left | ci_back_right | ci_lfe,
	cs_11point1_densewrap = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	                        ci_side_front_left | ci_side_front_right | ci_side_center_left | ci_side_center_right |
	                        ci_side_back_left | ci_side_back_right | ci_lfe,
	cs_13point1_totalwrap = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	                        ci_side_front_left | ci_side_front_right | ci_side_center_left | ci_side_center_right |
	                        ci_side_back_left | ci_side_back_right | ci_back_left | ci_back_right | ci_lfe,
	cs_16point1 = ci_front_left | ci_front_center_left | ci_front_center | ci_front_center_right | ci_front_right |
	              ci_side_front_left | ci_side_front_right | ci_side_center_left | ci_side_center_right | ci_side_back_left |
	              ci_side_back_right | ci_back_left | ci_back_center_left | ci_back_center | ci_back_center_right | ci_back_right | ci_lfe,
	cs_legacy = 0 // same channels as cs_5point1 but different upmixing transform; does not support the focus control
} channel_setup;

/**
 * The FreeSurround decoder.
 */
class freesurround_decoder {
	public:
	/**
	 * Create an instance of the decoder.
	 * @param setup The output channel setup -- determines the number of output channels
	 *			   and their place in the sound field.
	 * @param blocksize Granularity at which data is processed by the decode() function.
	 *				   Must be a power of two and should correspond to ca. 10ms worth of single-channel
	 *				   samples (default is 4096 for 44.1Khz data). Do not make it shorter or longer
	 *				   than 5ms to 20ms since the granularity at which locations are decoded
	 *				   changes with this.
	 */
	freesurround_decoder(channel_setup setup = cs_5point1, unsigned blocksize = 4096);
	~freesurround_decoder();

	/**
	 * Decode a chunk of stereo sound. The output is delayed by half of the blocksize.
	 * This function is the only one needed for straightforward decoding.
	 * @param input Contains exactly blocksize (multiplexed) stereo samples, i.e. 2*blocksize numbers.
	 * @return A pointer to an internal buffer of exactly blocksize (multiplexed) multichannel samples.
	 *		  The actual number of values depends on the number of output channels in the chosen
	 *		  channel setup.
	 */
	float *decode(const float *input);

	/**
	 * Flush the internal buffer.
	 */
	void flush();

	// --- soundfield transformations
	// These functions allow to set up geometric transformations of the sound field after it has been decoded.
	// The sound field is best pictured as a 2-dimensional square with the listener in its
	// center which can be shifted or stretched in various ways before it is sent to the
	// speakers. The order in which these transformations are applied is as listed below.

	/**
	 * Allows to wrap the soundfield around the listener in a circular manner.
	 * Determines the angle of the frontal sound stage relative to the listener, in degrees.
	 * A setting of 90� corresponds to standard surround decoding, 180� stretches the front stage from
	 * ear to ear, 270� wraps it around most of the head. The side and rear content of the sound
	 * field is compressed accordingly behind the listerer. (default: 90, range: [0�..360�])
	 */
	void circular_wrap(float v);

	/**
	 * Allows to shift the soundfield forward or backward.
	 * Value range: [-1.0..+1.0]. 0 is no offset, positive values move the sound
	 * forward, negative values move it backwards. (default: 0)
	 */
	void shift(float v);

	/**
	 * Allows to scale the soundfield backwards.
	 * Value range: [0.0..+5.0] -- 0 is all compressed to the front, 1 is no change, 5 is scaled 5x backwards (default: 1)
	 */
	void depth(float v);

	/**
	 * Allows to control the localization (i.e., focality) of sources.
	 * Value range: [-1.0..+1.0] -- 0 means unchanged, positive means more localized, negative means more ambient (default: 0)
	 */
	void focus(float v);

	// --- rendering parameters
	// These parameters control how the sound field is mapped onto speakers.

	/**
	 * Set the presence of the front center channel(s).
	 * Value range: [0.0..1.0] -- fully present at 1.0, fully replaced by left/right at 0.0 (default: 1).
	 * The default of 1.0 results in spec-conformant decoding ("movie mode") while a value of 0.7 is
	 * better suited for music reproduction (which is usually mixed without a center channel).
	 */
	void center_image(float v);

	/**
	 * Set the front stereo separation.
	 * Value range: [0.0..inf] -- 1.0 is default, 0.0 is mono.
	 */
	void front_separation(float v);

	/**
	 * Set the rear stereo separation.
	 * Value range: [0.0..inf] -- 1.0 is default, 0.0 is mono.
	 */
	void rear_separation(float v);

	// --- bass redirection (to LFE)

	/**
	 * Enable/disable LFE channel (default: false = disabled)
	 */
	void bass_redirection(bool v);

	/**
	 * Set the lower end of the transition band, in Hz/Nyquist (default: 40/22050).
	 */
	void low_cutoff(float v);

	/**
	 * Set the upper end of the transition band, in Hz/Nyquist (default: 90/22050).
	 */
	void high_cutoff(float v);

	// --- info

	/**
	 * Number of samples currently held in the buffer.
	 */
	unsigned buffered();

	/**
	 * Number of channels in the given setup.
	 */
	static unsigned num_channels(channel_setup s);

	/**
	 * Channel id of the i'th channel in the given setup.
	 */
	static channel_id channel_at(channel_setup s, unsigned i);

	private:
	class decoder_impl *impl; // private implementation
};

#endif
