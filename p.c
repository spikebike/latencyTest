#define _GNU_SOURCE
#define _MULTI_THREADED
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <float.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>

/* latency benchmark version 0.9 - written by Bill Broadley bill@broadley.org 

 Designed to explore single thread latency with while (p) { p=a[p] } to force
 dependent loads.  Shuffling is used to help prevent prefetching from hiding the
 real memory latency.

 To compile
    gcc -Wall -pedantic -O4 p.c -o p

 to run: 
    ./a.out 4096 128 # pages of 4k, cache lines of 128 bytes

*/

int pageSize;
int cacheLineSize;
int perCacheLine;
int cacheLinesPerPage;

double
second ()
{
	struct timeval tp;
	struct timezone tzp;
	gettimeofday (&tp, &tzp);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1.e-6);
}

void
swap (int64_t * a, int64_t x, int64_t y)
{
	int64_t t;
	t = a[x];
	a[x] = a[y];
	a[y] = t;
}

/* choose between l and h, inclusive of both. */
uint64_t
choose (uint64_t l, uint64_t h)
{
	uint64_t range, smallr, ret;

	range = h - l;
	assert (l <= h);
	smallr = range / perCacheLine;	
	ret = (l + (uint64_t) (drand48 () * smallr) * perCacheLine);
	assert (ret <= h);
	if (l < h)
		return ret;
	return h;
}

int64_t
initAr(int64_t *a,int64_t size)
{
	int64_t base,i;
	int64_t *b;
	int64_t cnt;

	base=0;
	cnt=0;
	printf ("cacheLinesPerPage=%d\n",cacheLinesPerPage);
   while (base<size)
	{
		b=&a[base]; 
		for (i = 0; i < pageSize/sizeof(int64_t); i = i + perCacheLine)
		{
			b[i] = i + perCacheLine;	/* assign each int the index of the next int */
			cnt++;
		}
		b[i-cacheLineSize/sizeof(int64_t)]=0;
		base=base+pageSize/sizeof(int64_t);
	}
	return(cnt);
}	

int
shuffleAr(int64_t *a, int64_t size, int64_t **visitOrder)
{
	int64_t base,c,x,y;
	int64_t *b;
	int64_t followPages;

	srand48 ((long int) getpid ());
	base=0;
   while (base<size)
   {
		b=&a[base];
		for (int64_t i = 0; i < pageSize/sizeof(int64_t); i = i + perCacheLine)
		{
			c = choose (i, pageSize/sizeof(int64_t) - perCacheLine);
			x = b[i];
			y = b[c];
			swap (b,i,c);
			swap (b,x,y);
		}
		base=base+pageSize/sizeof(int64_t);
   }
   followPages = size / (pageSize/sizeof(int64_t));
   *visitOrder = (int64_t*)malloc(sizeof(int64_t)*followPages); 

   //
	// To prevent hitting every cacheline of page N and having page N+1 prefetched do
   // a random sort of pages.  This allows TLB friendliness without prefetch.
   // 
	// if you visit 64 cachelines of 4k page N, randomly, with dependent loads, it looks
   // quite a bit like 64 sequential loads with stride of 4k to the hardware.  Triggering
	// prefetch of page N+1.  Thus latencies round about 10ns instread of 8 times higher.
   //
   // Ideas welcome to fixing this, while not thrashing the TLB.

   // Init the page order to squentially visit each page
   for (int64_t i=0; i<followPages; i++)
   {
      (*visitOrder)[i]=i;
   }
#ifdef RANDOM
   // Randomize it to prevent page N+1 prefection while accessing every cacheline of N
   for (int64_t  i = followPages - 1; i > 0; i--) {
        // Pick a random index from 0 to i
        int j = rand() % (i + 1);
        
        // Swap a[i] with a[j]
        int temp = (*visitOrder)[i];
        (*visitOrder)[i] = (*visitOrder)[j];
        (*visitOrder)[j] = temp;
   }  
#endif
	return(0);	
}

int64_t
followAr (int64_t *a, int64_t size, int64_t *visitOrder,int repeat)
{
	int64_t p;
	int64_t *b;
	int64_t base;
#ifdef CNT
	int64_t cnt=0;
#endif 
	int64_t followPages,pagesRead,i;
//	int64_t sum;

//	sum=0;
	followPages = size / (pageSize/sizeof(int64_t));
	for (i = 0; i < repeat; i++)
	{
		pagesRead=0;
		while (pagesRead<followPages) {
			base=visitOrder[pagesRead]*pageSize/sizeof(int64_t);
			b=&a[base];  // start pointer chasing at begin of page
			p=b[0];  // first access of a new page
#ifdef CNT
			cnt++;
#endif
//  uncomment to debug addresses
#ifdef PRINT
			printf("%p\n",(void *)&p[b]);
#endif
			while (p) // Visit cachelines until we get zero
			{
				p = b[p]; // Dependent load to visit next cacheline
//				sum += p;
#ifdef PRINT
				printf("%p\n",(void *)&p[b]);
#endif
#ifdef CNT
				cnt++;
#endif
			}
			pagesRead++; // Increment to next random page, NOT next sequential page
		}
	}
#ifdef CNT
	return (cnt);
#else
	return (0);
#endif
}

int
verifyAr(int64_t * a, int64_t size,int numPages)
{
	for (int64_t i=0;i<numPages*(pageSize/sizeof(int64_t)); i=i+perCacheLine) { 
      if (i%(pageSize/sizeof(int64_t)) ==0) { 
         printf("\nbase=%04ld ",i/perCacheLine);
      }
      printf("%ld ",a[i]); 
   }
	printf("\n");
	return(0);
}

int initialize_perf_event_attr(struct perf_event_attr *pe) {
	memset(pe, 0, sizeof(struct perf_event_attr));
	pe->type = PERF_TYPE_HW_CACHE;
   pe->size = sizeof(struct perf_event_attr);
   pe->config = PERF_COUNT_HW_CACHE_DTLB <<  0 |
               PERF_COUNT_HW_CACHE_OP_READ <<  8 |
               PERF_COUNT_HW_CACHE_RESULT_MISS << 16;
	pe->disabled = 1;
	pe->exclude_kernel = 1;
	pe->exclude_hv = 1;

	int fd = syscall(__NR_perf_event_open, pe, 0, -1, -1, 0);
	if (fd == -1) {
		fprintf(stderr, "Error opening perf event, might need to run as root or tweak capabilities\n");
		exit(EXIT_FAILURE);
	}
	return fd;
}

int
main (int argc, char *argv[])
{
	int64_t *a,*visitOrder=NULL;
	int64_t size,ret;
	int64_t maxmem=1073741824; // 1GB 
	double start,end;

#ifdef TLB
	int fd;
	struct perf_event_attr pe;
	fd=initialize_perf_event_attr(&pe);
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
#endif
	if (argc<3) {
		printf ("Try ./p 4096 128  # 4096 for the page size 128 for the cache line size\n\n");
		pageSize=4096;
		cacheLineSize=128;
	} else {
		pageSize= atoi(argv[1]);
		cacheLineSize= atoi(argv[2]);
	}

	cacheLinesPerPage = pageSize/cacheLineSize;
	perCacheLine = cacheLineSize/sizeof(int64_t);

	size = maxmem / sizeof (int64_t);  // number of int64s.

	if (posix_memalign((void **)&a, getpagesize(), sizeof(int64_t) * maxmem) != 0) {
		printf ("Memory allocation failed\n");
		exit(-1);
	} else { 
		printf ("Allocated %ld bytes or %ld INT64s succeeded.\n",maxmem,size);
	} 
	
	ret=initAr(a,size);
	printf("Initialized %ld cachelines\n",ret);
#ifdef VERIFY 
	printf ("in order page traversal\n");
	ret=verifyAr(a,size,4);
#endif
	ret=shuffleAr(a,size,&visitOrder);
	if (!ret) { printf ("Shuffle succeded\n"); } else { printf ("shuffle failed\n"); }
#ifdef VERIFY 
	printf ("shuffled page traversal\n");
	ret=verifyAr(a,size,4);
#endif
#ifdef TLB
   ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
	start = second ();
	ret=followAr (a, size, visitOrder,1);
	end = second();
#ifndef CNT                  // calculate cachelines if CNT is not defined
	ret=maxmem/cacheLineSize; // actually count each cacheline acces if CNT is defined
#endif

#ifdef TLB
   ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	printf("visited %ld cachelines\n",ret);
	 long long count;
    if (read(fd, &count, sizeof(long long)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
#endif
	printf ("took %f seconds, %f ns per cacheline, pageSize=%d, cacheLine=%d, pages=%ld ",end-start,((end-start)/ret)*1000000000,pageSize,cacheLineSize,maxmem/pageSize);
#ifdef TLB
	printf ("tlb misses=%lld\n",count);
#else
	printf ("\n");
#endif
}
