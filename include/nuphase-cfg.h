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

  /* the name of the file holding the current run number */ 
  const char * run_file; 

  /* The output directory for files */ 
  const char * output_directory; 

  /* The size of the circular buffers */ 
  size_t buffer_capacity; 

  /* Monitor interval */ 
  size_t monitor_hz; 

  /* The length of a run in seconds */ 
  unsigned run_length; 

} nuphase_acq_cfg_t; 


typedef struct nuphase_copy_cfg
{
  const char * remote_hostname; 
  const char * copy_command; 
} nuphase_copy_cfg_t; 




#endif
