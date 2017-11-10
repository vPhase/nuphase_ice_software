#include <stdio.h> 
#include <stdlib.h> 
#include "nuphase.h" 
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 

/** This program overwrites 
 *  the saved status file for Acqd, allowing you to "seed" 
 *  Acqd with desired thresholds */ 

int main (int nargs, char ** args) 
{
  if (nargs != 2 && nargs != NP_NUM_BEAMS +1) 
  {
    fprintf(stderr,"Usage:\tnuphase-set-saved-thresolds thresh\n\t\tnuphase-set-saved-thresholds thresh0 thresh1 thresh2 thresh3 ... thresh14\n"); 
    return 1; 
  }

  nuphase_acq_cfg_t cfg; 
  nuphase_acq_config_init(&cfg); 
  char * cfgpath; 
  if (!nuphase_get_cfg_file(&cfgpath, NUPHASE_ACQ))
  {
    nuphase_acq_config_read(cfgpath, &cfg);
  }

  nuphase_status_t save; 
  
  int thresh = atoi(args[1]); 
  int ibeam ;
  for (ibeam = 0; ibeam < NP_NUM_BEAMS; ibeam++)
  {
    if (nargs > 2 && ibeam > 0) 
    {
      thresh = atoi(args[1+ibeam]); 
    }
    save.trigger_thresholds[ibeam] = thresh; 
  }

  FILE * f = fopen(cfg.status_save_file,"w"); 
  fwrite(&save, sizeof(save),1,f); 
  fclose(f); 

  return 0; 
}
  
