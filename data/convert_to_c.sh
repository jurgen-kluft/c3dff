#!/bin/bash

FILES="*.ply"
for f in $FILES
do
  echo "Processing file $f ..."
  xxd -i $f > "../source/test/cpp/data/input_${f/.ply/.cpp}"
done
