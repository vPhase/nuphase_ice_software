
/* Startup program.
 *
 * just runs at startup, reads temperatures, and exits when other things can start 
 *
 *
 **/ 


#include "nuphasehk.h" 
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <stdio.h> 


int main (int nargs, char ** args) 
{

  nuphase_hk_settings_t hkset;
  nuphase_hk_init(&hkset); 



  //read the config file
  nuphase_start_cfg_t cfg; 
  nuphase_start_config_init(&cfg); 

  char * config_file = 0; 
  if (!nuphase_get_cfg_file(&config_file, NUPHASE_STARTUP))
  {
    nuphase_start_config_read(config_file, &cfg); 
  }




  //now we can turn on the FPGA's via the ASPS so we have access to their temperature sensors
  nuphase_set_asps_power_state (ASPS_ALL, cfg.asps_method); //should we just do HTTP since it's safer? 

  nuphase_hk_t hk; 
  // now loop until the temperature is warm enough 
  while (1) 
  {
    nuphase_hk(&hk, cfg.asps_method); 
    int heater_current = nuphase_get_heater_current(cfg.asps_method); 
    printf("master: %d slave: %d, case: %d, heater current: %d ", hk.temp_master, hk.temp_slave, hk.temp_case, heater_current); 
    if (hk.temp_master > cfg.min_temperature && hk.temp_slave > cfg.min_temperature) 
    {
      //make sure we turn off heater. 
      nuphase_set_heater_current(0, cfg.asps_method); 
      break; 
    }

    if (heater_current != cfg.heater_current) 
    {
        nuphase_set_heater_current(cfg.heater_current, cfg.asps_method); 
    }
 
    sleep(cfg.poll_interval); 
  } 





  return 0; 
}
