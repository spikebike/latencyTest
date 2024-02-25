#!/bin/bash
for i in `seq 465 16384`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done


