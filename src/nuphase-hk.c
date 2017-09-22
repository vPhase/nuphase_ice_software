#include <stdio.h> 
#include <string.h>
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <fcntl.h> 
#include <sys/mman.h> 
#include <sys/stat.h>

/** Housekeeping Program. 
 * 
 *  - Polls housekeeping info and saves to file
 *  - also makes hk available to shared memory location 
 *  - with SIGUSR1, rereads config, also causing it to apply the power statuses. 
 */ 


static nuphase_hk_cfg_t * cfg = 0; 
char * config_file; 


static volatile int stop = 0; 
static nuphase_hk_t * the_hk = 0;  

static int shared_fd = 0; 


int open_shared_fd() 
{
  shared_fd = shm_open(cfg->shm_name, O_CREAT | O_RDWR, 0666); 
  if (shared_fd < 0) 
  {
    fprintf(stderr,"Could not open shared memory region %s\n", cfg->shm_name); 
    return 1; 
  }

  if (ftruncate(shared_fd, sizeof(nuphase_hk_t))) 
  {

    fprintf(stderr,"Could not resize shared memory region %s\n", cfg->shm_name); 
    close(shared_fd); 
    shared_fd = -1; 
    return 1; 
  }

   the_hk = mmap(0, sizeof(nuphase_hk_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0); 

   return shared_fd != 0; 
}

int readConfig() 
{
  char * current_shm = cfg ? strdupa(cfg->shm_name) : 0; 
  int ret =  nuphase_hk_config_read(config_file, cfg); 

  if (strcmp(current_shm, cfg->shm_name))
  {
    close(shared_fd); 
    return ret + open_shared_fd(); 
  }

  return ret; 
}


FILE * getOutputFile() 
{




  return 0; 
}



int main(int nargs, char ** args) 
{

  /** Initialize the configuration */ 
  nuphase_hk_config_init(cfg); 

  /* If we were launched with an argument, use it as the config file */ 
  if (nargs > 1)
  {
    config_file = args[1]; 
  }
  else 
  {
    nuphase_get_cfg_file(&config_file, NUPHASE_HK); 
  }


  if (readConfig())
  {
    fprintf(stderr, "Could not set up properly\n"); 
  }


  while (!stop) 
  {

    nuphase_hk(the_hk, cfg->asps_method) ; 





    sleep(cfg->interval); 
  }



  return 0 ; 
}
