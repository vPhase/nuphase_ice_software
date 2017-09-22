#ifndef _NUPHASE_CFG_H
#define _NUPHASE_CFG_H

#include "nuphase-common.h" 
#include "nuphase.h" 
#include "nuphasehk.h" 
#include <stdlib.h>

/** 
 * \file nuphase-cfg.h 
 *
 * This contains the configuration structs
 * and the methods to read / write them. 
 *
 * Usually, each program will use a config file to read/write these. 
 *
 * See the config files in cfg/ 
 */ 



/** Configuration options for nuphase-acq */ 
typedef struct nuphase_acq_cfg
{

  /* the names of the spi devices
   * 0 should be master, 1 should be slave/ 
   * */ 
  const char * spi_devices[NP_MAX_BOARDS]; 

  /* the name of the file holding the desired run number*/ 
  const char * run_file; 

  /* The output directory for files */ 
  const char * output_directory; 

  //scaler goals, in Hz 
  float scaler_goal[NP_NUM_BEAMS]; 

  // trigger mask
  uint16_t trigger_mask; 

  // channel mask
  uint8_t channel_mask; 

  //channel read_mask
  uint8_t channel_read_mask[2]; 

  // pid goal constats;
  double k_p,k_i, k_d; 

  /* The size of the circular buffers */ 
  size_t buffer_capacity; 

  /* Monitor interval  (in seconds) */ 
  float monitor_interval; 

  /* SW trigger interval (in seconds) */ 
  float sw_trigger_interval; 

  // print to screen interval 
  int print_interval; 

  /* The maximum length of a run in seconds */ 
  unsigned run_length; 

  /* The SPI clock speed, in MHz */ 
  unsigned spi_clock; 
  
  // number of samples to save 
  unsigned waveform_length;

  /** 1 to enable the phased trigger, 0 otherwise */ 
  int enable_phased_trigger; 

  /** cal pulser state , 0 for off, 3 for on (or 2 for nothing) */ 
  int calpulser_state; 

  uint8_t attenuation [NP_MAX_BOARDS][NP_NUM_CHAN]; 

  int pretrigger; 

  double slow_scaler_weight; 

  double fast_scaler_weight; 

  int secs_before_phased_trigger; 

  int events_per_file; 

  int status_per_file; 

  int n_fast_scaler_avg; 

} nuphase_acq_cfg_t; 


/** Initialize a config with defaults. Usually a good idea to do this before reading a file in case the config file doesn't have all the keys */
void nuphase_acq_config_init(nuphase_acq_cfg_t *); 

/** Replace any values in the config with the values from the file */ 
int nuphase_acq_config_read(const char * file, nuphase_acq_cfg_t * ); 

/** Write out the current configuration to the file */ 
int nuphase_acq_config_write(const char * file, const nuphase_acq_cfg_t * ); 


typedef struct nuphase_copy_cfg
{
  //TODO . Have to figure out if we'll do it this way or the other way around
  const char * remote_hostname; 
} nuphase_copy_cfg_t; 


void nuphase_copy_config_init(nuphase_copy_cfg_t *); 
int nuphase_copy_config_read(const char * file, nuphase_copy_cfg_t * ); 
int nuphase_copy_config_write(const char * file, const nuphase_copy_cfg_t * ); 

typedef struct nuphase_start_cfg
{
  int min_temperature; 
  nuphase_asps_method_t asps_method; 
  int heater_current; 
  int poll_interval; 
}nuphase_start_cfg_t; 


void nuphase_start_config_init(nuphase_start_cfg_t *); 
int nuphase_start_config_read(const char * file, nuphase_start_cfg_t * ); 
int nuphase_start_config_write(const char * file, const nuphase_start_cfg_t * ); 

/* configuration options for nuphase-hkd */ 
typedef struct nuphase_hkd_cfg
{
  int interval; //polling interval for HK data . Default 5 seconds 
  const char * out_dir; //output directory for hk data 
  int max_secs_per_file; // maximum number of seconds per file. Default 600
  const char * shm_name; //shared memory name
  nuphase_asps_method_t asps_method;  //asps output method 

} nuphase_hk_cfg_t;



void nuphase_hk_config_init(nuphase_hk_cfg_t *); 
int nuphase_hk_config_read(const char * file, nuphase_hk_cfg_t * ); 
int nuphase_hk_config_write(const char * file, const nuphase_hk_cfg_t * ); 




#endif
