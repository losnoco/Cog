#!/usr/bin/python

import os, sys, subprocess, time, glob

os.chdir(os.path.dirname(sys.argv[0]) + "/../sidplay-libs-2.1.1")
subprocess.check_call(("rm", "-rf", "sidplay-libs-2.1.1"));
subprocess.check_call(("make", "distdir"))
subprocess.check_call(("tar", "cjf", "../sidplay-libs-2.1.1+alan.tar.bz2", "sidplay-libs-2.1.1"));
os.chdir("../sidplay-2.0.9")
subprocess.check_call(("make", "distdir"))
subprocess.check_call(("tar", "cjf", "../sidplay-2.0.9+alan.tar.bz2", "sidplay-2.0.9"));
os.chdir("..");

dest = "alankila@bel.fi:public_html/c64-sw/"
subprocess.call(("scp", "-r", "sidplay-2.0.9+alan.tar.bz2", "sidplay-libs-2.1.1+alan.tar.bz2", "ini/sidplay2.ini", "ini", "fc-curves", "combined-waveforms", dest))

for file in glob.glob("*.deb"):
    subprocess.call(("scp", file, dest + "deb/"));
