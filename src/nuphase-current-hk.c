#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <stdio.h> 

int main(int nargs, char ** args) 
{
  nuphase_hk_cfg_t cfg; 
  nuphase_hk_config_init(&cfg); 

  char * config_name = 0; 

  nuphase_get_cfg_file(&config_name, NUPHASE_HK); 
  nuphase_hk_config_read(config_name, &cfg); 


  char shbuf[512]; 
  sprintf(shbuf,"/dev/shm/%s", cfg.shm_name); 

  FILE * f = fopen(shbuf,"r"); 

  nuphase_hk_t hk; 
  fread(&hk, sizeof(hk),1, f); 

  fclose(f); 
  nuphase_hk_print(stdout, &hk); 


  return 0; 

}
