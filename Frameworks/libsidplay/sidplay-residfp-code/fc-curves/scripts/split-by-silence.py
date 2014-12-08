#!/usr/bin/python

import struct, sys, wave

class UnsupportedWaveformat(Exception):
    pass

class InvalidArgs(Exception):
    pass

class AnalysisError(Exception):
    pass

def split_wave(input, pattern, multiplier=8):
    wv = wave.open(input, 'r')

    if wv.getnchannels() != 1:
        raise UnsupportedWaveformat, "mono audio only"
    if wv.getsampwidth() != 2:
        raise UnsupportedWaveformat, "16-bit pcm only"

    readlen = 128

    outhandle = None
    seqnum = -1
    frames = None
    sum = 0

    # highpass filter
    levelcomp_coeff = (50 * 2 * 3.14159) / wv.getframerate()
    levelcomp = 0
    pos = 0
    while True:
        # read in chunks, classify as silence or as signal
        datachar = wv.readframes(readlen)
        pos += readlen

        if frames is not None:
            frames += readlen

        # wave finish?
        if len(datachar) < readlen * 2:
            if outhandle is not None:
                outhandle.writeframes(datachar)
                outhandle.close();
                cliplen = float(frames) / outhandle.getframerate()
                print "End writing wave %d, %.1f s clip" % (seqnum, cliplen)
                outhandle = None
            break

        # get wave values
        datatuple = struct.unpack("%sh" % readlen, datachar)

        # analyse signal levels.
        _sum = 0
        _min = 0
        _max = 0
        for _ in datatuple:
            _ /= 32768.
            if _ < _min:
                _min = _
            if _ > _max:
                _max = _
            levelcomp = _ * levelcomp_coeff + (1 - levelcomp_coeff) * levelcomp
            _sum += (_ - levelcomp) ** 2
        _sum /= readlen
        sum = (sum * 15 + _sum) / 16;

        start = _max > 0.2# or sum > 5e-7
        end = _min < -0.2# or sum < 1e-7

        # trigger silence
        if end:
            if outhandle is not None:
                outhandle.close()
                cliplen = float(frames) / outhandle.getframerate()
                print "End writing wave %d, %.1f seconds" % (seqnum, cliplen)
                if cliplen < 4:
                    raise AnalysisError, "sound prematurely clipped at %f" % (pos/outhandle.getframerate())
                if cliplen > 6:
                    raise AnalysisError, "sound clipped too late at %f" % (pos/outhandle.getframerate())
                outhandle = None
                frames = 0

        # trigger signal
        if start:
            if outhandle is None:
                seqnum += 1

                outhandle = wave.open(pattern % (multiplier * seqnum), "w")
                outhandle.setnchannels(1)
                outhandle.setframerate(wv.getframerate())
                outhandle.setsampwidth(2);

                if frames is not None:
                    cliplen = float(frames) / outhandle.getframerate()
                    print "Begun writing wave %d after %.1f s silence" % (seqnum, cliplen)
                    if cliplen < 0.05:
                        raise AnalysisError, "silence too short";
                    if cliplen > 1.5:
                        raise AnalysisError, "silence too long";

                frames = 0
                continue

        # write if we got handle
        if outhandle is not None:
            outhandle.writeframes(datachar)

    if seqnum != 2048 / multiplier - 1:
        raise AnalysisError, "Wrong count of waves written. Results are probably invalid."

def main(args):
    if len(args) != 2:
        raise InvalidArgs, "Usage: program <inputfile> { lp | hp }"
    return split_wave(args[0], args[1] + "_%04d.wav")
    return 0

if __name__ == '__main__':
    main(sys.argv[1:])
