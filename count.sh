#!/bin/bash
words=0
lines=0
count=0
for f in ./src/*.c; 
do
  length=`wc -l<$f`
  lines=`expr $lines + $length`
  length=`wc -w<$f`
  words=`expr $words + $length`
  count=`expr $count + 1`
done
for f in ./Headers/*.h; 
do
  length=`wc -l<$f`
  lines=`expr $lines + $length`
  length=`wc -w<$f`
  words=`expr $words + $length`
  count=`expr $count + 1`
done
echo "Files: $count"
echo "Lines: $lines"
echo "Words: $words"
