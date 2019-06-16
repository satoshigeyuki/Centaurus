#!/bin/sh

origfile=citylots.json.orig

head -3 $origfile
head -n -3 $origfile | tail +4
for i in $(seq $(($1 - 1)))
do
    echo ','
    head -n -3 $origfile | tail +4
done
tail -3 $origfile
