#!/bin/sh

WAV="../noisetest/noise-nf.wav"

./simulate-filter.py Vhp 0 > Vhp-0.txt
./simulate-filter.py Vbp 0 > Vbp-0.txt
./simulate-filter.py Vlp 0 > Vlp-0.txt
./simulate-filter.py sum 0 > sum-0.txt
./simulate-filter.py sum_lo 0 > sum_lo-0.txt
./simulate-filter.py sum_hi 0 > sum_hi-0.txt
./simulate-filter.py sum_notch 0 > sum_notch-0.txt

./simulate-filter.py Vhp 15 > Vhp-f.txt
./simulate-filter.py Vbp 15 > Vbp-f.txt
./simulate-filter.py Vlp 15 > Vlp-f.txt
./simulate-filter.py sum 15 > sum-f.txt
./simulate-filter.py sum_lo 15 > sum_lo-f.txt
./simulate-filter.py sum_hi 15 > sum_hi-f.txt
./simulate-filter.py sum_notch 15 > sum_notch-f.txt

./combine-two-files.py Vhp-0.txt Vhp-f.txt > Vhp.txt
./combine-two-files.py Vbp-0.txt Vbp-f.txt > Vbp.txt
./combine-two-files.py Vlp-0.txt Vlp-f.txt > Vlp.txt
./combine-two-files.py sum-0.txt sum-f.txt > sum.txt
./combine-two-files.py sum_lo-0.txt sum_lo-f.txt > sum_lo.txt
./combine-two-files.py sum_hi-0.txt sum_hi-f.txt > sum_hi.txt
./combine-two-files.py sum_notch-0.txt sum_notch-f.txt > sum_notch.txt

