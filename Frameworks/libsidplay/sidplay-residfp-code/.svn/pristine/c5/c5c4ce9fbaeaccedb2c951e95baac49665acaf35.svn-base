#!/usr/bin/python

import math, struct, sys, wave
from scipy.signal import get_window
from scipy.fftpack import fft, ifft
from scipy import array

class ChunkedReader(object):
    __slots__ = ['wav', 'format', 'formatlen', 'channels', 'channel', 'chunk', 'output_cache']
    def __init__(self, wavfile, chunk, channel=None, start=0):
        self.wav = wave.open(wavfile, 'r')
        self.channel = channel
        self.chunk = chunk

        self.channels = self.wav.getnchannels()
        if channel is not None and channel >= self.channels:
            raise RuntimeError, "not enough channels: %d vs. %d" % (self.channels, channel)
        format = "h"
        self.formatlen = 2
        if self.wav.getsampwidth() == 4:
            format = "f"
            self.formatlen = 4
        self.format = `self.channels * self.chunk` + format
        self.wav.setpos(start)
        self.output_cache = array([0] * self.chunk)

    def next(self):
        data = self.wav.readframes(self.chunk)
        # end of frames
        if len(data) < self.channels * self.chunk * self.formatlen:
            raise StopIteration

        data = struct.unpack(self.format, data)
        if self.formatlen == 4:
            data = map(lambda _: _ * 65536, data)
        # 1-channel case is easy
        if self.channels == 1:
            return data

        output = []
        if self.channel is not None:
            return array(data[self.channel : len(data) : self.channels])

        # mix all channels together
        output = array(self.output_cache)
        for c in range(self.channels):
            output += array(data[c : len(data) : self.channels])
        return output

def pow2db(x):
    if x == 0:
        return -999
    return math.log(x) / math.log(10) * 10

def get_one_spectrum(wav, analysis_len=None, FFT_SIZE=1024, channel=None):
    wav = ChunkedReader(wav, FFT_SIZE, channel)

    acc = array([0.0] * FFT_SIZE)
    window = get_window('hamming', FFT_SIZE)

    if analysis_len is None:
        count = 100000000000L
    else:
        count = analysis_len / FFT_SIZE

    num = 0
    try:
        for _ in xrange(count):
            acc += abs(fft(wav.next() * window)) ** 2
            num += 1
    except StopIteration:
        pass

    acc /= num
    return acc

def main():
    channel = None
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        raise RuntimeError, "usage: fft.py <wavfile> <fft size> [<channel>]"
    if len(sys.argv) == 4:
        channel = int(sys.argv[3])

    freq = wave.open(sys.argv[1]).getframerate()
    fft = get_one_spectrum(sys.argv[1], FFT_SIZE=int(sys.argv[2]), channel=channel)

    for i in range(len(fft) / 2 + 1):
        print "%f %f" % (float(i) / len(fft) * freq, pow2db(fft[i]))

if __name__ == '__main__':
    main()

