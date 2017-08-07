#ifndef _NUPHASE_CFG_H
#define _NUPHASE_CFG_H
#include "nuphase-common.h" 
#include <stdlib.h>



typedef struct nuphase_acq_cfg
{

  /* the names of the spi devices */ 
  const char * spi_devices[NUM_NUPHASE_DEVICES]; 

  /* the names of the gpio devices for interrupts (or none)*/ 
  const char * gpio_devices[NUM_NUPHASE_DEVICES]; 

  /* the name of the file holding the desired run number*/ 
  const char * run_file; 

  /* The output directory for files */ 
  const char * output_directory; 

  /* The size of the circular buffers */ 
  size_t buffer_capacity; 

  /* Monitor interval */ 
  size_t monitor_hz; 

  /* The length of a run in seconds */ 
  unsigned run_length; 

  /* The SPI clock speed, in MHz */ 
  unsigned spi_clock; 
  
  unsigned bufer_length;


} nuphase_acq_cfg_t; 


void nuphase_acq_config_init(nuphase_acq_cfg *); 
void nuphase_acq_config_read(const char * file, nuphase_acq_cfg * ); 
void nuphase_acq_config_write(const char * file, cons tnuphase_acq_cfg * ); 



typedef struct nuphase_copy_cfg
{
  const char * remote_hostname; 
  const char * copy_command; 
  int delete_after; 
} nuphase_copy_cfg_t; 



void nuphase_copy_config_init(nuphase_acq_cfg *); 
void nuphase_copy_config_read(const char * file, nuphase_acq_cfg * ); 
void nuphase_copy_config_write(const char * file, cons tnuphase_acq_cfg * ); 


typedef struct nuphase_startup_cfg
{
  int  min_temperature; 
  int  temperature_ain[NUM_NUPHASE_DEVICES]; 
  int board_gpios[NUM_NUPHASE_DEVICES]; 
} nuphase_stratup_cfg_t; 


typedef struct nuphase_hk_cfg
{
  int interval; 
} nuphase_hk_cfg_t;






#endif
