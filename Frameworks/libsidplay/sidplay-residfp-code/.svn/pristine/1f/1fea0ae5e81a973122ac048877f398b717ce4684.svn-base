#!/usr/bin/python

import os, sys, math, glob

def read_xy_file(filename, offset):
    f = file(filename)
    vals = []
    for line in f:
        x, y = line.split()
        x = float(x)
        y = float(y)
        vals.append([x, y + offset(x, y)])
    return vals

def main():
    fftdir = sys.argv[1]
    fnlp = "%s/lp_%%04d.wav.fft" % fftdir
    fnhp = "%s/hp_%%04d.wav.fft" % fftdir

    cutoffs = [];
    for x in range(0, 2047):
        if not os.path.exists(fnlp % x):
            continue
        cutoffs.append(x)

    result = []
    for cf in cutoffs:
        
        # no correction function, but we should also dump
        # the difference between lp and hp.
        lp = read_xy_file(fnlp % cf, lambda x, y: 0)
        hp = read_xy_file(fnhp % cf, lambda x, y: 0)

        noise_threshold = 3
        cf_est = 0
        cf_n = 0

        lens = range(len(lp))
        for i in lens:
            if lp[i][0] != hp[i][0]:
                raise RuntimeError, "invalid content in cf %d" % cf

            # zrx has a weird chip with freq ~ 140 Hz!
            if lp[i][0] < 120 or lp[i][0] > 20000:
                continue

            # disregard video signal spikes and some side spikes, unknown
            # origin... the 4900 Hz spike seems to multiply
            if lp[i][0] > 4500 and lp[i][0] < 4600:
                continue
            if lp[i][0] > 4800 and lp[i][0] < 4900:
                continue
            if lp[i][0] > 6650 and lp[i][0] < 6750:
                continue
            if lp[i][0] > 9740 and lp[i][0] < 9860:
                continue
            if lp[i][0] > 15600 and lp[i][0] < 15800:
                continue
            if lp[i][0] > 19500 and lp[i][0] < 19650:
                continue
            if lp[i][0] > 20150 and lp[i][0] < 20250:
                continue
            if lp[i][0] > 10050 and lp[i][0] < 10150:
                continue

            if abs(lp[i][1] - hp[i][1]) > noise_threshold:
                continue

            # this is ad-hoc weighing method. The values closer to crossing
            # point get more weight. The right model might be logarithmic, not ** 2.
            weight = noise_threshold - abs(lp[i][1] - hp[i][1]);
            weight = weight ** 2
            cf_est += weight * lp[i][0];
            cf_n += weight;

        # dump average
        if cf_n == 0:
            cf_n = 1
        source = "%d,%d" % (cf, cf_est / cf_n)
        result.append(source)

    for i in range(len(result)):
        print result[i].replace(",", " ")
    return

    print "[FilterMeasurement]"
    print "type=1"
    print "points=%d" % len(result)
    for i in range(len(result)):
        print "point%d=%s" % (i+1, result[i])

if __name__ == '__main__':
    main()



