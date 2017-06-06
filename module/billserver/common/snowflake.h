#include <stdio.h>  
#include <pthread.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <sched.h>  
#include <linux/unistd.h>  
#include <sys/syscall.h>  
#include <errno.h>  
#include<linux/types.h>  
#include<time.h>  
#include <stdint.h>  
#include <sys/time.h>  
  
struct  globle  
{  
    int global_int:12;  
    uint64_t last_stamp;  
    int workid;  
    int seqid;  
};  
  
void set_workid(int workid);  
pid_t gettid( void );  
uint64_t get_curr_ms();  
uint64_t wait_next_ms(uint64_t lastStamp);  
int atomic_incr(int id);  
uint64_t get_unique_id();  


