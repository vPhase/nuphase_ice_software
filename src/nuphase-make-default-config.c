#include "nuphase-cfg.h" 
#include <stdio.h> 
#include <string.h> 

int usage() 
{
    fprintf(stderr,"usage: nuphase-make-default-config (acq|startup|hk|copy) out.cfg\n"); 
    exit(1); 

}


int main(int nargs, char ** args) 
{

  if (nargs < 3) 
  {
    usage(); 
  }


  if (!strcmp("acq", args[1]))
  {
    nuphase_acq_cfg_t cfg; 
    nuphase_acq_config_init(&cfg); 
    nuphase_acq_config_write(args[2],&cfg); 
  }
  else if (!strcmp("startup", args[1]))
  {
    nuphase_start_cfg_t cfg; 
    nuphase_start_config_init(&cfg); 
    nuphase_start_config_write(args[2],&cfg); 
  }

  else if (!strcmp("hk", args[1]))
  {
    nuphase_hk_cfg_t cfg; 
    nuphase_hk_config_init(&cfg); 
    nuphase_hk_config_write(args[2],&cfg); 
  }
  else if (!strcmp("copy", args[1]))
  {
    nuphase_copy_cfg_t cfg; 
    nuphase_copy_config_init(&cfg); 
    nuphase_copy_config_write(args[2],&cfg); 
  }

  else
  {
    usage(); 
  }



  return 0; 
}



