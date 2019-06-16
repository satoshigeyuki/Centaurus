#!/bin/sh

origfile=dblp.xml.orig

head -3 $origfile
for i in $(seq $(($1)))
do
    head -n -2 $origfile | tail +4
done
tail -2 $origfile
