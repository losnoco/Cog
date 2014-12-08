#!/usr/bin/python

import os, sys

def main():
    fn1, fn2 = sys.argv[1:]
    f1 = file(fn1)
    f2 = file(fn2)
    
    for l1 in f1:
        l1 = l1.strip()
        l1 = l1.split()
        k1 = l1[0]
        v1 = l1[1]
        l2 = f2.readline().strip()
        l2 = l2.split()
        k2 = l2[0]
        v2 = l2[1]
        if k1 != k2:
            raise FileFormatError, "key1 and key2 differ on line: '%s' vs '%s'" % (l1, l2)
        print "%s %s %s" % (k1, v1, v2)

if __name__ == '__main__':
    main()
