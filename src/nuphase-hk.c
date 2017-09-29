#include <stdio.h> 
#include <string.h>
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <fcntl.h> 
#include <sys/mman.h> 
#include <sys/stat.h>
#include <signal.h>

/** Housekeeping Program. 
 * 
 *  - Polls housekeeping info and saves to file
 *  - also makes hk available to shared memory location 
 *  - with SIGUSR1, rereads config, also causing it to apply the power statuses. 
 *
 */ 


static nuphase_hk_cfg_t cfg; 
static char * config_file; 


static volatile int stop = 0; 
static nuphase_hk_t * the_hk = 0;  

static int shared_fd = 0; 


static int open_shared_fd() 
{
  shared_fd = shm_open(cfg.shm_name, O_CREAT | O_RDWR, 0666); 
  if (shared_fd < 0) 
  {
    fprintf(stderr,"Could not open shared memory region %s\n", cfg.shm_name); 
    return 1; 
  }

  if (ftruncate(shared_fd, sizeof(nuphase_hk_t))) 
  {

    fprintf(stderr,"Could not resize shared memory region %s\n", cfg.shm_name); 
    close(shared_fd); 
    shared_fd = -1; 
    return 1; 
  }

   the_hk = mmap(0, sizeof(nuphase_hk_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0); 

   return shared_fd == 0; 
}

static int read_config() 
{

  char * current_shm = strdupa(cfg.shm_name); 

  int ret =  nuphase_hk_config_read(config_file, &cfg); 

  if (!shared_fd || strcmp(current_shm, cfg.shm_name))
  {
    if (shared_fd) close(shared_fd); 
    return ret + open_shared_fd(); 
  }

  return ret; 
}

static void signal_handler(int signal, siginfo_t * sig, void * v) 
{
  switch (signal)
  {
    case SIGUSR1: 
      read_config(); 
      break; 
    case SIGTERM: 
    case SIGUSR2: 
    case SIGINT: 
    default: 
      fprintf(stderr,"Caught deadly signal %d\n", signal); 
      stop =1; 
  }
}

static const char * mk_name(time_t t)
{
  struct tm * tim = gmtime(&t); 
  static char buf[1024]; 
  int ok = 0; 
  ok += mkdir_if_needed(cfg.out_dir); 

  sprintf(buf,"%s/hk", cfg.out_dir); 
  ok += mkdir_if_needed(buf); 

  sprintf(buf,"%s/hk/%04d/", cfg.out_dir, 1900 + tim->tm_year); 
  ok += mkdir_if_needed(buf); 

  sprintf(buf,"%s/hk/%04d/%02d/", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1); 
  ok += mkdir_if_needed(buf); 
  
  sprintf(buf,"%s/hk/%04d/%02d/%02d", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1, tim->tm_mday); 
  ok += mkdir_if_needed(buf); 

  if (!ok) 
  {
    sprintf(buf,"%s/%04d/%02d/%02d/%02d%02d%02d.hk.gz", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec); 
    return buf; 
  }

  return 0; 
}





int main(int nargs, char ** args) 
{

  //set up signal handlers 
  
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


  /** Initialize the configuration */ 
  nuphase_hk_config_init(&cfg); 

  /* If we were launched with an argument, use it as the config file */ 
  if (nargs > 1)
  {
    config_file = args[1]; 
  }
  else 
  {
    nuphase_get_cfg_file(&config_file, NUPHASE_HK); 
  }



  if (read_config())
  {
    fprintf(stderr, "Could not set up properly\n"); 
  }


  time_t last = 0;



  gzFile outf = 0; 
  

  while (!stop) 
  {
    nuphase_hk(the_hk, cfg.asps_method) ; 

    time_t now = time(0); 

    if (now - last > cfg.max_secs_per_file) 
    {
      if (outf) gzclose(outf); 
      const char * fname = mk_name(now); 
      outf = gzopen(fname,"w"); 
      last = now; 
    }

    nuphase_hk_gzwrite(outf, the_hk);
    if (cfg.print_to_screen)
      nuphase_hk_print(stdout, the_hk); 
    sleep(cfg.interval); 
  }


  close(shared_fd); 
  gzclose(outf); 

  return 0 ; 
}
