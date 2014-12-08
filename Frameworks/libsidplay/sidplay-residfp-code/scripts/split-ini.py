#!/usr/bin/python

import os, sys, re

targetfile = None
filtersection = re.compile(r"\[Filter(.*)\]")

os.chdir(os.path.dirname(sys.argv[0]) + "/..")
for line in file("ini/sidplay2.ini"):
    line = line.strip();

    row = filtersection.match(line)
    if row is None:
	if targetfile:
	    print >> targetfile, line
	continue

    name = row.group(1).lower()
    targetfile = file(("ini/%s.ini" % name), "w")
    print >> targetfile, "[Filter]"
