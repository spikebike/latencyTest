#!/bin/bash
for i in `seq 1 200`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done
for i in `seq 201 2 400`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done
for i in `seq 801 4 1600`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done
for i in `seq 1601 8 3200`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done
for i in `seq 3201 16 6400`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done
for i in `seq 6401 32 16384`; do 
	rm a.out; 
	export sz=`echo "$i * 4096"|bc`; 
	gcc -O3 -DRANDOM -DTLB p.c && sudo  ./a.out $sz 64|grep took; 
done


