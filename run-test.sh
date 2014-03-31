#!/bin/bash
count=0
for i in 1 2 4 8 16 32
do
	echo "Starting thread count $i"
	for j in 1 100 1000 10000 100000 500000
	do
		count=$[$count +1]
		echo "NUM_LOOPS=$j"
		./server $[8600+ $count] $i $j &
		for k in {2..$i}
		do
			./client localhost $[8600+ $count] &
		done
		./client localhost $[8600+ $count]
	done
done
