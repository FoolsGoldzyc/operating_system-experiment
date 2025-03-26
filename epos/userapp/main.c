/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "graphics.h"
struct timespec req = {
    .tv_sec = 0,
    .tv_nsec = 100000000 
};
struct draw_data
{
    int *arr;
    int region;
    int num;
    COLORREF color;
};

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */

 void __main()
{
    size_t heap_size = 32*1024*1024;
    void  *heap_base = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	
}

     /**
 * 第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{
   printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here
    int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
     time_t   t1, t2;
     t1 = time(&t2);
     

     printf("t1= %ld\n",t1);
     printf("t2= %ld\n",t2);
     if (t1 == t2){
    printf("program runs successfully\n");  
     }
   
    
     
     
     
 
     while(1);
   
    
    task_exit(0);
    
}

