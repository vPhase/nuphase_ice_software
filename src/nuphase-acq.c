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
 *  signals. 
 *
 * - Acquisition thread - takes the data and reads it
 * 
 * - A monitoring thread, reads in statuses, send software_triggers, and does the pid loops 
 *   (not sure if this should be merged into the acquisition thread...) 
 *
 * - A write thread, which writes to disk and screen.
 *
 * The config is read on startup. Right now, the configuration cannot be reloaded
 * without a restart.
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
#include <signal.h>


/************** Structs /Typedefs ******************************/


/** PID state */ 
typedef struct pid_state
{
  double k_p; 
  double k_i; 
  double k_d; 
  int nsum; 
  double sum_error[NP_NUM_BEAMS];
  double last_measured[NP_NUM_BEAMS];
} pid_state_t; 

static void pid_state_init (pid_state_t * c, double p, double i, double d)
{
  c->k_p = p; 
  c->k_i = i; 
  c->k_d = d; 

  c->nsum =0;
  memset(c->sum_error,0, sizeof(c->sum_error)); 
  memset(c->last_measured,0, sizeof(c->last_measured)); 
}

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
  nuphase_status_t status; //status before
  uint32_t thresholds[NP_NUM_BEAMS]; //thresholds when written 
  pid_state_t control; //the pid state 
} monitor_buffer_t; 

/**************Static vars *******************************/

/* The configuration state */ 
static nuphase_acq_cfg_t config; 

/* Mutex protecting the configuration */
static pthread_mutex_t config_lock; 

/* The device */
static nuphase_dev_t* device;

/* Acquisition thread handles */ 
static pthread_t the_acq_thread; 

/*Buffer for acq thread */ 
static nuphase_buf_t* acq_buffer = 0; 

/*Buffer for monitor thread */ 
static nuphase_buf_t* mon_buffer = 0; 

/* Monitor thread handle */ 
static pthread_t the_mon_thread; 

/* Write thread handle */ 
static pthread_t the_wri_thread; 

static pid_state_t control; 

/* used for exiting */ 
static volatile int die; 


/************** Prototypes *********************
 Brief documentation follows. More detailed documentation
 at implementation.
 */


static int run_number; 

// this sets everything up (opens device, starts threads, signal handlers, etc. ) 
static int setup(); 
// this cleans up 
static int teardown(); 

//this reads in the config
static int read_config(int first_time); 


// call this when we need to stop
static void fatal(); 

static void signal_handler(int, siginfo_t *, void *); 

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
    usleep(100000);  //100 ms 
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


  while(!die) 
  {
   /* grab a buffer to fill */ 
    acq_buffer_t * mem = (acq_buffer_t*) nuphase_buf_getmem(acq_buffer); 
    mem->nfilled = 0; //nothing filled

    while (!mem->nfilled) 
    {
      mem->nfilled = nuphase_wait_for_and_read_multiple_events(device, &mem->headers, &mem->events); 
    }
    nuphase_buf_commit(acq_buffer); // we filled it 
  }

  return 0; 
}





/***********************************************************************
 * Monitor thread
 *
 * This will periodically grab the status / adjust thresholds / send software triggers
 *
 *  The PID loops lies within here. 
 ***************************************************************************************/
void * monitor_thread(void *v) 
{

  struct timespec start; 
  clock_gettime(CLOCK_MONOTONIC, &start); 

  struct timespec last_sw_trig = { .tv_sec = 0, .tv_nsec = 0};
  struct timespec last_mon = { .tv_sec = 0, .tv_nsec = 0};
  int phased_trigger_status = -1; 

  while(!die) 
  {
    //figure out the current time
    struct timespec now; 
    clock_gettime(CLOCK_MONOTONIC, &now); 

    /////////////////////////////////////////////////////
    //turn on and off phased trigger as necessary
    /////////////////////////////////////////////////////
    if (config.enable_phased_trigger && phased_trigger_status != 1)
    {
      if (config.secs_before_phased_trigger)
      {
        struct timespec now; 
        clock_gettime(CLOCK_MONOTONIC, &now); 
        if (timespec_difference_float(&now,&start) > config.secs_before_phased_trigger)
        {
          nuphase_phased_trigger_readout(device, 1); 
          phased_trigger_status = 1; 
        }
      }
      else
      {
          nuphase_phased_trigger_readout(device, 1); 
          phased_trigger_status = 1; 
      }
    }
    else if (!config.enable_phased_trigger && phased_trigger_status == 1)
    {
      nuphase_phased_trigger_readout(device, 0); 
      phased_trigger_status = 0; 
    }

 
    float diff_mon = timespec_difference_float(&now, &last_mon); 
    float diff_swtrig = timespec_difference_float(&now, &last_sw_trig); 

    //////////////////////////////////////////////////////
    //read the status, and react to it 
    //////////////////////////////////////////////////////
    if (config.monitor_interval &&  diff_mon > config.monitor_interval)
    {
      monitor_buffer_t mb; 
      nuphase_status_t *st = &mb.status;
      nuphase_read_status(device, st, MASTER); 
      int ibeam; 

      for (ibeam = 0; ibeam < NP_NUM_BEAMS; ibeam++)
      {

       
        ///// REVISIT THIS 
        ///// We need to figure out how to use both the fast and slow scalers
        ///// For now, take a weighted average of the fast and slow scalers to determine the rate. 
        double measured_slow = ((double) st->beam_scalers[SCALER_SLOW][ibeam]) / NP_SCALER_TIME(SCALER_SLOW); 
        double measured_fast = ((double) st->beam_scalers[SCALER_FAST][ibeam]) / NP_SCALER_TIME(SCALER_FAST); 
        double measured =  (config.slow_scaler_weight * measured_slow + config.fast_scaler_weight * measured_fast) / (config.slow_scaler_weight + config.fast_scaler_weight); 
        double e =  config.scaler_goal[ibeam] - measured;
        double de = 0;

        // only compute difference after first iteration
        if (control.nsum > 0) 
        {
          de = (measured - control.last_measured[ibeam]) / diff_mon; 
        }

        control.sum_error[ibeam] += e; 
        control.nsum++; 
        double ie = control.sum_error[ibeam] / control.nsum; 

        control.last_measured[ibeam] = measured; 

        // modify threshold 
        mb.thresholds[ibeam] =  control.k_p * e + control.k_i * ie * control.k_d * de; 
      }

      //apply the thresholds 
      nuphase_set_thresholds(device, mb.thresholds,0); 
      
      //copy over the current control status 
      memcpy(&mb.control, &control, sizeof(control)); 

      nuphase_buf_push(mon_buffer, &mb);
      memcpy(&last_mon,&now, sizeof(now)); 
      diff_mon = 0; 
    }

    if (config.sw_trigger_interval && diff_swtrig > config.sw_trigger_interval)
    {
      nuphase_sw_trigger(device); 
      memcpy(&last_sw_trig,&now, sizeof(now)); 
      diff_swtrig = 0;
    }

    //now figure out how long to sleep 
    //
    float how_long_to_sleep = 0.1; //don't sleep longer than 100 ms 

    if (config.monitor_interval && config.monitor_interval - diff_mon  < how_long_to_sleep) how_long_to_sleep = config.monitor_interval - diff_mon; 
    if (config.sw_trigger_interval && config.sw_trigger_interval - diff_swtrig  < how_long_to_sleep) how_long_to_sleep = config.sw_trigger_interval - diff_swtrig; 

    usleep(how_long_to_sleep * 1e6); 
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

  char bigbuf[strlen(config.output_directory)+512];

  int    data_file_size = 0;
  int    header_file_size = 0;
  int    status_file_size =0;

  gzFile data_file = 0 ; 
  gzFile header_file = 0 ; 
  gzFile status_file  = 0 ; 

  acq_buffer_t *events= 0; 
  monitor_buffer_t *mon= 0;

  
  int num_events = 0; 
 

  while(1) 
  {
    time_t now = time(0); 
    int have_data= 0; 
    int have_status = 0;
    
    if (nuphase_buf_occupancy(acq_buffer))
    {
        events = nuphase_buf_pop(acq_buffer, events); 
        num_events += events->nfilled; 
        have_data=1;
    }

    if (nuphase_buf_occupancy(mon_buffer)) 
    {
      mon = nuphase_buf_pop(mon_buffer, mon); 
      have_status=1; 
    }
    
    //print something to screen
    if (config.print_interval > 0 && now - last_print_out > config.print_interval)  
    {
      printf("---------%s----------\n",asctime(gmtime(&now)));  
      printf("  approx_rate:  %g Hz\n", (num_events == 0) ? 0. : num_events / ((float) now - (float) last_print_out)); 
      num_events = 0;
    }


    if (!have_data && !have_status)
    {
      if (die) 
        break; 

      sched_yield(); 
      continue; 
    }
   
    //make sure we have the right dirs
    if ( now - current_dir_time > config.run_length) 
    {
      if (make_dirs_for_time(config.output_directory, start_time))
      {
        fatal(); 
        break; 
      }
      current_dir_time = now; 
    }
   
        
    if (have_data)
    {
      int j; 

      for (j = 0; j < events->nfilled; j++)
      {

        if (!data_file || data_file_size > config.events_per_file)
        {
          if (data_file) gzclose(data_file); 
          snprintf(bigbuf,sizeof(bigbuf),"%s/event/%u/%u.event.gz", config.output_directory, (unsigned) current_dir_time, (unsigned) now); 
          data_file = gzopen(bigbuf,"w");  //TODO add error check
          data_file_size = 0; 
        }

        if (!header_file || header_file_size > config.events_per_file)
        {
          if (header_file) gzclose(header_file); 
          snprintf(bigbuf,sizeof(bigbuf),"%s/header/%u/%u.header.gz", config.output_directory, (unsigned) current_dir_time,(unsigned)  now); 
          header_file = gzopen(bigbuf,"w");  //TODO add error check
          header_file_size = 0; 
        }
       
        nuphase_event_gzwrite(data_file, &events->events[j]); 
        nuphase_header_gzwrite(header_file, &events->headers[j]); 
        data_file_size++; 
        header_file_size++; 
      }
    }

    if (have_status)
    {
      if (!status_file || status_file_size > config.status_per_file)
      {
        if (status_file) gzclose(status_file); 
        snprintf(bigbuf,sizeof(bigbuf),"%s/status/%u/%u.status.gz", config.output_directory, (unsigned) current_dir_time, (unsigned) now); 
        status_file = gzopen(bigbuf,"w");  //TODO add error check
        status_file_size = 0; 
      }

      nuphase_status_gzwrite(status_file, &mon->status); 
      status_file_size++; 
    }
    
  }

  return 0; 

}


void fatal()
{
  die = 1; 

  //cancel any waits 
  if (device) 
    nuphase_cancel_wait(device); 

}

void signal_handler(int signal, siginfo_t * sig, void * v) 
{
  switch (signal)
  {
    case SIGUSR1: 
      read_config(0); 
      break; 
    case SIGTERM: 
    case SIGUSR2: 
    case SIGINT: 
    default: 
      fprintf(stderr,"Caught deadly signal %d\n", signal); 
      fatal(); 
  }
}

const char * tmp_run_file = "/tmp/.runfile"; 

int setup()
{
  //let's do signal handlers. 
  //My understanding is that the main thread gets all the signals it doesn't block first, so we'll just handle it that way. 

  sigset_t empty;
  sigemptyset(&empty); 
  struct sigaction sa; 
  sa.sa_mask = empty; 
  sa.sa_flags = 0; 
  sa.sa_sigaction = signal_handler; 

  sigaction(SIGINT,  &sa,0); 
  sigaction(SIGTERM, &sa,0); 
  sigaction(SIGUSR1, &sa,0); 
  sigaction(SIGUSR2, &sa,0); 


  //Read configuration 
  read_config(1); 

  /* open up the run number file, read the run, then increment it and save it
   * This is a bit fragile, potentially, so maybe revisit. 
   * */ 
  FILE * run_file = fopen(config.run_file, "r"); 
  fscanf(run_file, "%d\n", &run_number); 
  fclose(run_file); 

  run_file = fopen(tmp_run_file,"w"); 
  fprintf(run_file,"%d\n", run_number+1); 
  fclose(run_file); 
  rename(tmp_run_file, config.run_file); 


  //open the devices and configure properly
  // the gpio state should already have been set 
  device = nuphase_open(config.spi_devices[0], config.spi_devices[1], 0, 1); 

  if (!device)
  {
    //that would not be really good 
    fprintf(stderr,"Couldn't open device. Aborting! \n"); 
    exit(1); 
  }

  uint64_t run64 = run_number; 

  nuphase_set_spi_clock(device, config.spi_clock); 
  nuphase_set_buffer_length(device, config.waveform_length); 
  nuphase_set_readout_number_offset(device, run64 * 1000000000); 

  // set up the buffers
  acq_buffer = nuphase_buf_init( config.buffer_capacity, sizeof(acq_buffer_t)); 
  mon_buffer = nuphase_buf_init( config.buffer_capacity, sizeof(acq_buffer_t)); 


  // set up the threads 
  pthread_create(&the_mon_thread, 0, monitor_thread, 0); 
  pthread_create(&the_acq_thread, 0, acq_thread, 0); 
  pthread_create(&the_wri_thread, 0, write_thread, 0); 
  
  return 0;
}

int teardown() 
{
  pthread_join(the_acq_thread,0); 
  pthread_join(the_mon_thread,0); 
  pthread_join(the_wri_thread,0); 
  nuphase_close(device); 
  return 0;
}


int read_config(int first_time)
{

  char * cfgpath = 0;  
  
  asprintf(&cfgpath, "%s/%s", getenv(CONFIG_DIR_ENV), CONFIG_DIR_ACQ_NAME); 

  pthread_mutex_lock(&config_lock); 

  nuphase_acq_config_read( cfgpath, &config); 
  pthread_mutex_unlock(&config_lock); 

  pid_state_init(&control, config.k_p, config.k_i, config.k_d); 

  if (!first_time) 
  {
    // what do we need to change? 
   
    nuphase_set_buffer_length(device, config.waveform_length); 
    nuphase_set_spi_clock(device, config.spi_clock); 

    //rewrite run number in case we are using a different file 
    FILE * run_file = fopen(tmp_run_file,"w"); 
    fprintf(run_file,"%d\n", run_number+1); 
    fclose(run_file); 
    rename(tmp_run_file, config.run_file); 
  }

  free(cfgpath); 

  return 0;
}

