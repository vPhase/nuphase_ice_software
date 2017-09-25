#include "nuphase-cfg.h" 
#include <stdio.h> 
#include <string.h> 

int usage() 
{
    fprintf(stderr,"usage: nuphase-check-config (acq|startup|hk|copy) check.cfg [output.cfg]\n"); 
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
    int notok = nuphase_acq_config_read(args[2],&cfg); 
    if (nargs >3) 
    {
      nuphase_acq_config_write(args[3],&cfg); 
    }
    if (notok) 
    {
      fprintf(stderr, "Trouble reading :(\n"); 
    }
    return notok; 
  }

  else if (!strcmp("startup", args[1]))
  {
    nuphase_start_cfg_t cfg; 
    nuphase_start_config_init(&cfg); 
    int notok = nuphase_start_config_read(args[2],&cfg); 
    if (nargs >3) 
    {
      nuphase_start_config_write(args[3],&cfg); 
    }
    if (notok) 
    {
      fprintf(stderr, "Trouble reading :(\n"); 
    }
    return notok; 
  }



  else if (!strcmp("hk", args[1]))
  {
    nuphase_hk_cfg_t cfg; 
    nuphase_hk_config_init(&cfg); 
    int notok = nuphase_hk_config_read(args[2],&cfg); 
    if (nargs >3) 
    {
      nuphase_hk_config_write(args[3],&cfg); 
    }
    if (notok) 
    {
      fprintf(stderr, "Trouble reading :(\n"); 
    }
    return notok; 
  }

  else if (!strcmp("copy", args[1]))
  {
    nuphase_copy_cfg_t cfg; 
    nuphase_copy_config_init(&cfg); 
    int notok = nuphase_copy_config_read(args[2],&cfg); 
    if (nargs >3) 
    {
      nuphase_copy_config_write(args[3],&cfg); 
    }
    if (notok) 
    {
      fprintf(stderr, "Trouble reading :(\n"); 
    }
    return notok; 
  }




  else
  {
    usage(); 
  }



  return 0; 
}



