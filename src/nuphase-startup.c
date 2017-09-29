/* 
 * \file nuphase-startup.c
 *
 * Startup program. This makes sure the FPGA's dont' turn on
 * until it's warm enough and that the heater is running properly. 
 *
 * In the future, it may do an attenuation scan as well. 
 *
 * Right now this doesn't save any output (although I suppose things will be logged)
 *
 **/ 


#include "nuphasehk.h" 
#include "nuphase-cfg.h" 
#include "nuphase-common.h" 
#include <stdio.h> 
#include <string.h> 



int main (int nargs, char ** args) 
{

  //read the config file
  nuphase_start_cfg_t cfg; 
  nuphase_start_config_init(&cfg); 

  char * config_file = 0; 

  if (!nuphase_get_cfg_file(&config_file, NUPHASE_STARTUP))
  {
    printf("Using config file: %s\n", config_file); 
    nuphase_start_config_read(config_file, &cfg); 
  }
  


  //turn off all fpga gpio's 
  nuphase_set_gpio_power_state(0, GPIO_FPGA_ALL); 

  //now we can turn on the FPGA's via the ASPS so we have access to their temperature sensors
  nuphase_set_asps_power_state ( ASPS_MINIMAL|ASPS_FPGAS, cfg.asps_method); 

  //make the output directory
  gzFile out = 0; 

  time_t start_t; 
  time(&start_t); 

  if(!mkdir_if_needed(cfg.out_dir)) 
  {
    char buf[1024]; 
    struct tm * tim = gmtime(&start_t); 
    sprintf(buf,"%s/%04d_%02d_%02d_%02d%02d%02d.hk.gz", cfg.out_dir, 1900 + tim->tm_year, tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec); 
    out = gzopen(buf,"w"); 
  }



  //wait a little bit
  sleep(1); 



  nuphase_hk_t hk; 
  int master_ok = 0; 
  int slave_ok = 0; 
  // now loop until the temperature is warm enough 
  while (!master_ok || !slave_ok) 
  {

    //get the hk info 
    nuphase_hk(&hk, cfg.asps_method); 
    nuphase_hk_print(stdout,&hk); 
    if (out) 
    {
      nuphase_hk_gzwrite(out, &hk); 
    }


    if (hk.temp_master > cfg.min_temperature)
    {
      printf("Master OK\n"); 
      master_ok =1; 
    }

    if (hk.temp_slave > cfg.min_temperature)
    {
      printf("Slave OK, turning on\n"); 
      slave_ok =1; 
      nuphase_set_gpio_power_state( NP_FPGA_POWER_SLAVE, NP_FPGA_POWER_SLAVE); 
      sleep(1); 
    }

    if (master_ok && slave_ok) break; 


    //make sure that the asps heater is on 
    if (hk.asps_heater_current != cfg.heater_current) 
    {
        nuphase_set_asps_heater_current(cfg.heater_current, cfg.asps_method); 
    }

    //make sure that the aux heater is on
    nuphase_set_gpio_power_state(NP_AUX_HEATER, NP_AUX_HEATER); 
 
    sleep(cfg.poll_interval); 
  } 



  //turn off heaters
  printf("Turning off heaters\n"); 
  nuphase_set_asps_heater_current(0, cfg.asps_method); 
  nuphase_set_gpio_power_state(0, NP_AUX_HEATER); 

  //wait a bit 
  usleep(100000); 

  printf("Turning everything on\n"); 
  //turn on the frontends? 
  nuphase_set_asps_power_state(ASPS_ALL, cfg.asps_method); 

  //turn on the master 
  nuphase_set_gpio_power_state( NP_FPGA_POWER_MASTER, NP_FPGA_POWER_MASTER); 

  //turn on the downhole??? 
  // (not yet) 


  usleep(100000); 
  printf("Final state: \n"); 

  nuphase_hk(&hk, cfg.asps_method); 
  nuphase_hk_print(stdout,&hk); 

  if (out) 
  {
    nuphase_hk_gzwrite(out, &hk); 
    gzclose(out); 
  }


  if (strlen(cfg.set_attenuation_cmd))
  {
    char cmd[1024]; 
    sprintf(cmd,"%s %g %g", cfg.set_attenuation_cmd, cfg.desired_rms_master, cfg.desired_rms_slave); 
    printf("Running: %s\n", cmd); 
    system(cmd); 
  }



  return 0; 
}
