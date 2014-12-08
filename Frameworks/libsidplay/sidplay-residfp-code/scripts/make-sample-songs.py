#!/usr/bin/python

import glob, os, sys, subprocess, time, threading, Queue

hvsc = "/home/alankila/Musiikki/sid"
dest = "alankila@bel.fi:public_html/c64-sw/patched-sound"

songs = (
("MUSICIANS/0-9/20CC/van_Santen_Edwin/Spijkerhoek.sid",     None, "r4ar"),
("MUSICIANS/0-9/4-Mat/Filter.sid",                          None, "r4ar"),
("../sid-nodistrib/AMJ/Asm93Compotune.sid",                 None, "r3"),
("MUSICIANS/A/AMJ/Blasphemy.sid",                           None, "r4ar"),
("MUSICIANS/A/AMJ/Needledrop.sid",                          None, "r4ar"),
("MUSICIANS/A/AMJ/SYS4096.sid",                             None, "r4ar"),
("MUSICIANS/B/Brennan_Neil/Fist_II-Legend_Continues.sid",      1, "r4ar"),
("GAMES/A-F/Alien.sid",                                     None, "r4ar"),
("MUSICIANS/C/Cane/SidRiders.sid",                          None, "r4ar"),
("MUSICIANS/D/Deenen_Charles/Mantalos.sid",                 None, "r4ar"),
("MUSICIANS/D/Dunn_David/Dark_Tower.sid",                   None, "r4ar"),
("MUSICIANS/D/Dunn_David/Elite.sid",                           2, "r4ar"),
("MUSICIANS/D/Dunn_David/Flight_Path_737.sid",              None, "r4ar"),
("MUSICIANS/D/Dunn_David/Jump_Jet.sid",                     None, "r4ar"),
("MUSICIANS/D/Dunn_David/P_C_Fuzz.sid",                     None, "r4ar"),
("MUSICIANS/D/Dunn_David/Theatre_Europe.sid",               None, "r4ar"),
("MUSICIANS/D/Dunn_David/Trapdoor.sid",                     None, "r4ar"),
("MUSICIANS/D/Dunn_David/Tropical_Fever.sid",               None, "r4ar"),
("MUSICIANS/E/Eeben_Aleksi/Spaceman_Salutes_Commodore.sid", None, "r4ar"),
("MUSICIANS/G/Galway_Martin/Miami_Vice.sid",                None, "r4ar"),
("MUSICIANS/G/Galway_Martin/Miami_Vice.sid",                None, "gw"),
("MUSICIANS/G/Galway_Martin/Terra_Cresta.sid",              None, "r4ar"),
("MUSICIANS/G/Galway_Martin/Terra_Cresta.sid",              None, "gw"),
("MUSICIANS/G/Galway_Martin/Wizball.sid",                   None, "r4ar"),
("MUSICIANS/G/Galway_Martin/Wizball.sid",                   None, "gw"),
("MUSICIANS/G/Gray_Matt/Vendetta.sid",                      None, "r4ar"),
("MUSICIANS/H/Huelsbeck_Chris/Shades_filter_corrected.sid", None, "r4ar"),
("MUSICIANS/J/Jeff/6581_Doped_Cows.sid",                    None, "r4ar"),
("MUSICIANS/J/Jeff/6581_Doped_Cows.sid",                    None, "ln"),
("MUSICIANS/J/Jeff/Anal_ogue.sid",                          None, "r4ar"),
("MUSICIANS/J/Jeff/Anal_ogue.sid",                          None, "ln"),
("MUSICIANS/J/Jeff/Arabian_Bias.sid",                       None, "r4ar"),
("MUSICIANS/J/Jeff/Arabian_Bias.sid",                       None, "ln"),
("MUSICIANS/J/Jeff/Commodore_64.sid",                       None, "r4ar"),
("MUSICIANS/J/Jeff/Commodore_64.sid",                       None, "ln"),
("MUSICIANS/J/Jeff/Hard_Track.sid",                         None, "r4ar"),
("MUSICIANS/J/Jeff/Hard_Track.sid",                         None, "ln"),
("MUSICIANS/J/Jeff/Ode_To_C64.sid",                         None, "r4ar"),
("MUSICIANS/J/Jeff/Ode_To_C64.sid",                         None, "ln"),
("MUSICIANS/J/JO/Pice_of_Mind.sid",                         None, "r4ar"),
("MUSICIANS/M/Mitch_and_Dane/Gloria.sid",                   None, "r4ar"),
("MUSICIANS/M/Mitch_and_Dane/In_Velvet.sid",                None, "r4ar"),
("MUSICIANS/M/Mueller_Markus/Mechanicus.sid",               None, "r4ar"),
("MUSICIANS/N/Noise/Insanes.sid",                           None, "r4ar"),
("MUSICIANS/T/TBB/Meanwhile_The_Planet.sid",                None, "r4ar"),
("MUSICIANS/Y/Yip/Scroll_Machine.sid",                         4, "r4ar"),
("MUSICIANS/W/Wacek/Snake_Disco.sid",                       None, "r4ar"),
)

def main():
    os.chdir(os.path.dirname(sys.argv[0]) + "/..")

    q = Queue.Queue()
    for _ in songs:
	q.put(_) 

    threads = [Worker(q) for _ in "ab"]
    for _ in threads:
	_.start()
    for _ in threads:
	_.join()

class Worker(threading.Thread):
    __slots__ = ['q']
    def __init__(self, q):
	self.q = q
	threading.Thread.__init__(self)

    def run(self):
	filter = {
            'r3': 'alankila6581r3_3984_1',
	    'r4ar': 'alankila6581r4ar_3789',
	    'gw': 'trurl6581r3_0486s',
	    'ln': 'lordnightmare6581r3_4485',
	}
	
	try:
	    while True:
		i, subsong, chip = self.q.get_nowait()
		wavname = chip + "_" + os.path.basename(i).replace("sid", "wav")

		subsong_arg = "-o0"
		if subsong is not None:
		    subsong_arg = "-o%d" % subsong

		subprocess.call(("sidplay2", subsong_arg, "-ns0", ("-nfini/%s.ini" % filter[chip]), ("-w%s" % wavname), "%s/%s" % (hvsc, i)))
		subprocess.call(("oggenc", "-q", "6", wavname))
		os.unlink(wavname) 
		oggname = wavname.replace("wav", "ogg")
		subprocess.call(("scp", oggname, dest))
		os.unlink(oggname) 

	except Queue.Empty:
	    return

if __name__ == '__main__':
    main()
