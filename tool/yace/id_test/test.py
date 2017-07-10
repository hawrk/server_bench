#!/usr/bin/env python

data = {}
count = 0
for i in range(15):
    hd = open(str(i), 'r')
    for key in hd:
        key = key[:-1]
        count += 1
        if key in data:
            data[key] += 1
            print key, ":", data[key]
        else:
            data[key] = 1
    hd.close()
print 'check data lines:', count
