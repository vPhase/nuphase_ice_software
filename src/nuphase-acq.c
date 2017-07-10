/* Main Acquisition program for vPhase 
 *
 * Cosmin Deaconu <cozzyd@kicp.uchicago.edu>
 *
 *
 * Design: 
 *
 * Despite running on a single(at least for the first iteration) processor, the
 * work is divided into multiple threads to reduce latency of critical tasks. 
 *
 * Right now the threads are: 
 *
 *  - The main thread: reads the config, sets up, tears down, and waits for
 *  signals
 *
 * - A per-device acquisition thread, which will wait for a data ready
 *   interrupt and then take data.
 * 
 * - A per-device monitoring thread, which monitors each device and does things
 *   like the pid loop . 
 * 
 * - A write thread, which writes to disk.
 *
 * The config is read on startup and reread on SIGUSR1  (although some things,
 * like the device names, require a restart of the process). 
 *
 **/ 



/************** Includes **********************************/

#include "nuphase-common.h"
#include "nuphase-cfg.h"
#include "nuphase-buf.h" 
#include "nuphasedaq.h"
#include <pthread.h> 
#include <stdlib.h>
#include <unistd.h>


/************** Structs /Typedefs ******************************/

/* Acquisition thread paramters*/ 
typedef struct acq_thread_param
{
  nuphase_dev_t * d; 
  nuphase_buf_t * b; 
  int index; 

} acq_thread_param_t;

/*Monitor thread parameters */ 
typedef struct monitor_thread_param
{
  nuphase_dev_t * d; 
  nuphase_buf_t * b; 
} monitor_thread_param_t; 

typedef struct write_thread_param 
{
  int start_time; 
}write_thread_param_t;



/**************Static vars *******************************/

/* The configuration state */ 
static nuphase_acq_cfg_t config; 

/* Mutex protecting the configuration */
static pthread_mutex_t config_lock; 

/* The devices */
static nuphase_dev_t* devices[NUM_NUPHASE_DEVICES]; 

/* Mutexes protecting the devices */ 
static pthread_mutex_t device_locks[NUM_NUPHASE_DEVICES]; 

static nuphase_buf_t* event_buffers[NUM_NUPHASE_DEVICES];  
static nuphase_buf_t* monitor_buffers[NUM_NUPHASE_DEVICES];  

static volatile int die; 


/*******Prototypes **********************/

static int setup(); 
static int teardown(); 
static int readConfig(); 


/* Acquisition thread */ 
void * acq_thread(void * p);

/* Monitor thread */ 
void * monitor_thread(void * p); 

void * write_thread(void * ); 



/* The main function... not too much here */ 
int main(int nargs, char ** args) 
{
  if(setup())
  {
    return 1; 
  }

  while(!die) 
  {
    usleep(1000); 
  }

  return teardown(); 
}




/*** Acquistion thread **/ 


void * acq_thread(void *v) 
{
  acq_thread_param_t * p = (acq_thread_param_t*) v; 
  while(!die) 
  {
    int wait = nuphase_wait(p->d); 


  }
}

