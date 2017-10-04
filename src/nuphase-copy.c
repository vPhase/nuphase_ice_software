
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <signal.h> 
#include <stdio.h> 
#include <sys/statvfs.h>


/* 
 * 
 * This program is used to copy things to a host and delete old files. 
 *
 *   - It uses rsync to copy to host (which requires that you have the keys set up properly on the remote host(ssh-copy-id is your friend)) 
 *   - If rsync is successful AND there is less disk space than the threshold, files older than X days are deleted (using find). 
 *
 */ 


static nuphase_copy_cfg_t cfg; 
static volatile int stop = 0; 

static char * copy_command = 0; 
static char * delete_command = 0; 

static void construct_commands() 
{
  if (copy_command) free(copy_command); 
  asprintf(&copy_command, "rsync -q -a %s/ %s@%s:%s", cfg.local_path, cfg.remote_user, cfg.remote_hostname, cfg.remote_path); 

  if (delete_command) free(delete_command) ; 
  asprintf(&delete_command,"find %s -mtime +%d %s", cfg.local_path, cfg.delete_files_older_than, cfg.dummy_mode ? "-print" : "-delete"); 
}

static int read_config()
{
  char * cfgfile; 
  if (!nuphase_get_cfg_file(&cfgfile, NUPHASE_COPY))
  {
    printf("Reading %s\n", cfgfile); 
    nuphase_copy_config_read(cfgfile, &cfg); 
    free(cfgfile);
  }

  construct_commands(); 
  return 0; 
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



int main(int nargs, char ** arsg) 
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

  //set up config
  nuphase_copy_config_init(&cfg); 
  read_config(); 

  // main loop 
  while(!stop)
  {
    int copy_ret = system(copy_command);
    if (!copy_ret)
    {
      //only try to delete if copy succeeded 
      //and disk space below some threshold 
      struct statvfs vfs; 
      statvfs(cfg.local_path, &vfs); 
      int free_mb = ((vfs.f_bavail >> 10)  * vfs.f_bsize) >> 10; 
      printf("free MB: %d\n", free_mb); 
      if (free_mb < cfg.free_space_delete_threshold) 
      {
        system(delete_command); 
      }
    }
    else
    {
      fprintf(stderr,"rsync returned error code %d\n", copy_ret ); 
    }

    sleep(cfg.wakeup_interval); 
  }


  return 0; 

}
