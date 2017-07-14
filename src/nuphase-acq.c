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
 * - A monitoring thread, reads in statuses and does the pid loops
 *
 * - A write thread, which writes to disk and screen.
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
#include <sys/stat.h>
#include <string.h>
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
  nuphase_dev_t * d[NUM_NUPHASE_DEVICES];  //device handles. 0 if not used. 
  nuphase_buf_t * b; 
  float hz; // attempted monitoring frequency 
} monitor_thread_param_t; 

typedef struct write_thread_param 
{
  int new_dir_time;                              //if more than this many seconds has elapsed, 
                                                 //start a new directory for the next file
  int events_per_file;                           //number of events to store per file. 
  int status_per_file;                           //number of status's to store per file
  const char * data_dir;                         //the output directory
  nuphase_buf_t * monitor_buf;                   //buffer for the monitor
  nuphase_buf_t * acq_buf[NUM_NUPHASE_DEVICES];  //buffer for the acquisitions.  
  int write_board_to_disk[NUM_NUPHASE_DEVICES];  //the devices for which we should write to disk
  int board_id [NUM_NUPHASE_DEVICES];            //board ids
  int write_interval;                            // will write out some stats to screen every write_interval (if greater than 0)

}write_thread_param_t;



/* This is what is stored within the acquisition buffer 
 *
 * It's a bit wasteful... we allocate space for all buffers even though
 * we often (usually? )won't have all of them. 
 *
 * Will see if this is a problem. 
 **/ 
typedef struct acq_buffer
{
  int nfilled; 
  nuphase_event_t events[NP_NUM_BUFFER]; 
  nuphase_header_t headers[NP_NUM_BUFFER]; 
} acq_buffer_t;


/* this is what is stored within the monitor buffer */ 
typedef struct monitor_buffer
{
  nuphase_status_t status[NUM_NUPHASE_DEVICES]; 
} monitor_buffer_t; 

/**************Static vars *******************************/

/* The configuration state */ 
static nuphase_acq_cfg_t config; 

/* Mutex protecting the configuration */
static pthread_mutex_t config_lock; 

/* The devices */
static nuphase_dev_t* devices[NUM_NUPHASE_DEVICES]; 

/* Acquisition thread handles */ 
static pthread_t acq_threads[NUM_NUPHASE_DEVICES] ; 

/*Buffers for acq thread */ 
static nuphase_buf_t* event_buffers[NUM_NUPHASE_DEVICES];  

/*Buffers for monitor thread */ 
static nuphase_buf_t* monitor_buffers[NUM_NUPHASE_DEVICES];  

/* Monitor thread handle */ 
static pthread_t mon_thread; 

/* Write thread handle */ 
static pthread_t wri_thread; 

/* used for exiting */ 
static volatile int die; 


/************** Prototypes *********************
 Brief documentation follows. More detailed documentation
 at implementation.
 */



static int setup(); 
static int teardown(); 
static int readConfig(); 
static void fatal(); // call this when we need to stop


/* Acquisition thread */ 
static void * acq_thread(void * p);

/* Monitor thread */ 
static void * monitor_thread(void * p); 

/* Write thread */ 
static void * write_thread(void * p ); 



/* The main function... not too much here */ 
int main(int nargs, char ** args) 
{
  if(setup())
  {
    return 1; 
  }

  while(!die) 
  {
    usleep(1000);  //1 ms 
    sched_yield();
  }

  return teardown(); 
}





/*** Acquistion thread 
 *
 * This will wait for data, then record and it and put it into its appropriate buffer. 
 *
 ***/ 
void * acq_thread(void *v) 
{
  acq_thread_param_t * p = (acq_thread_param_t*) v; 

  while(!die) 
  {
    /* grab a buffer to fill */ 
    acq_buffer_t * mem = (acq_buffer_t*) nuphase_buf_getmem(p->b); 
    mem->nfilled = 0; //nothing filled

    while (!mem->nfilled) 
    {
      mem->nfilled = nuphase_wait_for_and_read_multiple_events(p->d, &mem->headers, &mem->events); 
    }
    nuphase_buf_commit(p->b); // we filled it 
  }

  return 0; 
}


/** Monitor thread
 *
 * This will periodically grab the status and put it into its buffer
 *
 **/
void * monitor_thread(void *v) 
{
  monitor_thread_param_t * p = (monitor_thread_param_t*) v; 
  unsigned sleep_amt =  1e6 * 1./p->hz; 
  int i;

  while(!die) 
  {
    monitor_buffer_t * mem = (monitor_buffer_t*) nuphase_buf_getmem(p->b); 
    for (i = 0; i < NUM_NUPHASE_DEVICES; i++)
    {
      if (p->d[i]) 
      {
        nuphase_read_status(p->d[i], &mem->status[i]); 
        //TODO: adjust pid goal 
      }
    }
    nuphase_buf_commit(p->b);
    usleep(sleep_amt); 
  }

  return 0; 
}




/* makes a directory if it's noth already there. 
 * If 0 is returned, then you can probably trust that it's there */ 
static int mkdir_if_needed(const char * path)
{

  struct stat st; 
  if ( stat(path,&st) || ( (st.st_mode & S_IFMT) != S_IFDIR))//not there, try to make it
  {
     return mkdir(path,0666);
  }

  return 0; 

}

const char * subdirs[] = {"event","header","status"}; 
const int nsubdirs = sizeof(subdirs) / sizeof(*subdirs); 

//this makes the necessary directories for a time 
//returns 0 on success. 
static int make_dirs_for_time(const char * prefix, unsigned time) 
{ 
  char buf[strlen(prefix) + 512];  

  //check to see that prefix exists and is a directory
  if (mkdir_if_needed(prefix))
  {
    fprintf(stderr,"Couldn't find %s or it's not a directory. Bad things will happen!\n",prefix); 
    return 1; 
  }


  int i;

  for (i = 0; i < nsubdirs; i++)
  {
    snprintf(buf,sizeof(buf), "%s/%s",prefix,subdirs[i]); 
    if (mkdir_if_needed(buf))
    {
        fprintf(stderr,"Couldn't make %s. Bad things will happen!\n",buf); 
        return 1; 
    }

    snprintf(buf,sizeof(buf), "%s/%s/%u",prefix,subdirs[i],time); 
    if (mkdir_if_needed(buf))
    {
        fprintf(stderr,"Couldn't make %s. Bad things will happen!\n",buf); 
        return 1; 
    }
  }

  return 0; 
}



/** Will write output to disk and some status info to screen */ 
void * write_thread(void *v) 
{
  time_t start_time = time(0);
  time_t current_dir_time = 0; 
  time_t last_print_out =start_time ; 
  int i; 

  write_thread_param_t * p = (write_thread_param_t*) v; 

  char bigbuf[strlen(p->data_dir)+512];

  int    data_file_size[NUM_NUPHASE_DEVICES] = {0} ; 
  int    header_file_size[NUM_NUPHASE_DEVICES] = {0} ; 
  int    status_file_size[NUM_NUPHASE_DEVICES] = {0} ; 

  gzFile data_files[NUM_NUPHASE_DEVICES] = {0} ; 
  gzFile header_files[NUM_NUPHASE_DEVICES] = {0} ; 
  gzFile status_files[NUM_NUPHASE_DEVICES]  = {0} ; 

  acq_buffer_t *event_buffers[NUM_NUPHASE_DEVICES] = {0};
  monitor_buffer_t *mon_buffer = 0;

  int num_output_devices = 0; 
  for (i = 0; i < NUM_NUPHASE_DEVICES; i++)
  {
    num_output_devices += p->write_board_to_disk[i]; 
  }

  int num_events[NUM_NUPHASE_DEVICES] = {0}; 
 

  while(true) 
  {
    time_t now = time(0); 
    int have_data= 0; 
    int have_status = 0;
    
    for (i = 0; i < NUM_NUPHASE_DEVICES; i++)
    {
      if (p->acq_buf[i])
      {
        if (!nuphase_buf_occupancy(p->acq_buf[i])) continue; //nothign to see here
        event_buffers[i] = nuphase_buf_pop(p->acq_buf[i], event_buffers[i]); 
        num_events[i]  += event_buffers[i]->nfilled; 
        have_data=1;
      }
    }

    if (nuphase_buf_occupancy(p->monitor_buf)) 
    {
      mon_buffer = nuphase_buf_pop(p->monitor_buf, mon_buffer); 
      have_status=1; 
    }
    
    //print something to screen
    if (p->write_interval > 0 && now - last_print_out > p->write_interval)  
    {
      printf("---------%s----------\n",asctime(gmtime(&now)));  
      for (i = 0; i < NUM_NUPHASE_DEVICES; i++)
      {
        printf("BD %d approx_rate:  %g Hz\n", p->board_id[i], (num_events[i] == 0) ? 0. : num_events[i] / ((float) now - (float) last_print_out)); 
        num_events[i] = 0;
      }

    }


    if (!have_data && !have_status)
    {
      if (die) 
        break; 

      sched_yield(); 
      continue; 
    }
   
    //if we are writing stuff out, it's time to do that now . 
    if (num_output_devices)
    {
      //make sure we have the right dirs
      if ( now - current_dir_time > p->new_dir_time) 
      {
        if (make_dirs_for_time(p->data_dir, start_time))
        {
          fatal(); 
          break; 
        }
        current_dir_time = now; 
      }
     
      for (i = 0; i < NUM_NUPHASE_DEVICES; i++)
      {
        if (p->write_board_to_disk[i]) 
        {
          
          if (have_data)
          {
            int j; 

            for (j = 0; j < event_buffers[i]->nfilled; j++)
            {

              if (!data_files[i] || data_file_size[i] > p->events_per_file)
              {
                if (data_files[i]) gzclose(data_files[i]); 
                snprintf(bigbuf,sizeof(bigbuf),"%s/event/%u/bd%d_%u.event.gz", p->data_dir, (unsigned) current_dir_time, p->board_id[i], (unsigned) now); 
                data_files[i] = gzopen(bigbuf,"w");  //TODO add error check
                data_file_size[i] = 0; 
              }

              if (!header_files[i] || header_file_size[i] > p->events_per_file)
              {
                if (header_files[i]) gzclose(header_files[i]); 
                snprintf(bigbuf,sizeof(bigbuf),"%s/header/%u/bd%d_%u.header.gz", p->data_dir, (unsigned) current_dir_time, p->board_id[i],(unsigned)  now); 
                header_files[i] = gzopen(bigbuf,"w");  //TODO add error check
                header_file_size[i] = 0; 
              }
             
              nuphase_event_gzwrite(data_files[i], &event_buffers[i]->events[j]); 
              nuphase_header_gzwrite(header_files[i], &event_buffers[i]->headers[j]); 
              data_file_size[i]++; 
              header_file_size[i]++; 
            }
          }

          if (have_status)
          {
            if (!status_files[i] || status_file_size[i] > p->status_per_file)
            {
              if (status_files[i]) gzclose(status_files[i]); 
              snprintf(bigbuf,sizeof(bigbuf),"%s/status/%u/bd%d_%u.status.gz", p->data_dir, (unsigned) current_dir_time, p->board_id[i], (unsigned) now); 
              status_files[i] = gzopen(bigbuf,"w");  //TODO add error check
              status_file_size[i] = 0; 
            }

            nuphase_status_gzwrite(status_files[i], &mon_buffer->status[i]); 
            status_file_size[i]++; 
          }
        }
      }
    }
  }

  return 0; 

}


void fatal()
{
  die = 1; 
}

int setup()
{

  return 0;

}

int teardown() 
{
  return 0;
}

int readConfig()
{


  return 0;
}

