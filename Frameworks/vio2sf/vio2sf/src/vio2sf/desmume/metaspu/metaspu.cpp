/*  Copyright 2009-2015 DeSmuME team

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "metaspu.h"

#include <queue>
#include <vector>
#include <list>
#include <cstring>
#include <assert.h>

class NullSynchronizer : public ISynchronizingAudioBuffer
{
public:
  std::queue<uint32_t> buffer;
  NullSynchronizer() {}
  ~NullSynchronizer() {}

	virtual void enqueue_samples(s16* buf, int samples_provided) {
    for (int i = 0; i < samples_provided * 2; i += 2) {
      uint16_t left = buf[i];
      uint16_t right = buf[i + 1];
      buffer.push(left << 16 | right);
    }
  }

	virtual int output_samples(s16* buf, int samples_requested) {
    int samples = (samples_requested < buffer.size()) ? samples_requested : buffer.size();
    for (int offset = 0, i = 0; i < samples; i++) {
      uint32_t sample = buffer.front();
      buffer.pop();
      buf[offset++] = (sample >> 16) & 0xFFFF;
      buf[offset++] = sample & 0xFFFF;
    }
    return samples;
  }
};

ISynchronizingAudioBuffer* metaspu_construct(ESynchMethod method)
{
	return new NullSynchronizer();
}
