#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

/* simple test bed for initialization, shuffle, and follow */

int64_t cl;

void
printAr(int64_t *A,int64_t size)
{
	int64_t i;
	for(i=0;i<size;i++)
	{ 
		printf ("%02ld=%02ld ",i,A[i]);
	}
	printf("\n");
}

int
main()
{
	int64_t *A,i,tmp,iter,current,cnt,p;
	int64_t list_size=512;

	A = malloc (sizeof(int64_t)*list_size);
	srand48 ((long int) getpid ());

	cl=16;
	
	for (i=0;i<list_size;i=i+cl)
	{
		A[i]=i;
	}

//	printAr(A,list_size);

	iter = list_size;
	while (iter > cl) {
		iter -= cl;
		int64_t j = ((int64_t)lrand48() % (iter / cl)) * cl;
		int64_t tmp = A[iter];
		A[iter] = A[j];
		A[j] = tmp;
	}
//	printf ("done\n");
//	printAr(A,list_size);
	cnt=0;
	current=A[0];
	cnt++;
	while (current) { 
		current = A[current];
		cnt++;
	}
//	printAr(A,list_size);
	printf ("cnt=%ld a[0]=%ld\n",cnt,A[0]);
	return(0);
}

	
