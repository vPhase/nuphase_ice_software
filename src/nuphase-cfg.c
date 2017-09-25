#include <libconfig.h> 
#include <string.h> 

#include "nuphase-cfg.h" 
#include "nuphase.h" 

/** Config file parsing uses libconfig. Not sure if it's the most efficient
 * option, but writing our own parser is a big hassle.
 *
 * Because comments aren't supported in writing out libconfig things, I just manually 
 * write out everything to file, including the "default" comments. 
 **/ 



//////////////////////////////////////////////////////
//start config 
/////////////////////////////////////////////////////

void nuphase_start_config_init(nuphase_start_cfg_t * c) 
{
  c->min_temperature = 0; 
  c->asps_method = NP_ASPS_SERIAL; 
  c->heater_current = 500; 
  c->poll_interval = 5; 
}

static void lookup_asps_method(const config_t * cfg, nuphase_asps_method_t * method, const char * key)
{
  const char * str; 
  if (config_lookup_string(cfg, key,&str)) 
  {
    *method = strcasestr(str, "http") ? NP_ASPS_HTTP : NP_ASPS_SERIAL; 
  }
}


int nuphase_start_config_read(const char * file, nuphase_start_cfg_t * c) 
{
  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

  if (!config_read_file(&cfg, file))
  {
     fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
     config_error_line(&cfg), config_error_text(&cfg));

     config_destroy(&cfg); 
     return 1; 
  }
  config_lookup_int(&cfg,"min_temperature", &c->min_temperature);
  config_lookup_int(&cfg,"heater_current", &c->heater_current);
  config_lookup_int(&cfg,"poll_interval", &c->poll_interval);

  lookup_asps_method(&cfg, &c->asps_method, "asps_method"); 

  config_destroy(&cfg); 
  return 0; 
}

int nuphase_start_config_write(const char * file, const nuphase_start_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 

  if (!f) return 1; 

  fprintf(f,"//Configuration file for nuphase-start\n");  
  fprintf(f, "//Minimum temperature, in C, to turn on FPGA's\n"); 
  fprintf(f, "min_temperature=%d;\n\n", c->min_temperature); 
  fprintf(f,"// Current in mA to run the heater at\n"); 
  fprintf(f,"heater_current = %d;\n", c->heater_current);  
  fprintf(f,"// The polling interval (to check temperature and heater state) in seconds\n"); 
  fprintf(f,"poll_interval = %d;\n", c->poll_interval); 

  fclose(f); 

  return 0; 
}



//////////////////////////////////////////////////////
//hk config 
/////////////////////////////////////////////////////
void nuphase_hk_config_init(nuphase_hk_cfg_t * c) 
{
  c->interval = 5; 
  c->asps_method = NP_ASPS_SERIAL; 
  c->out_dir = "/data/hk/"; 
  c->max_secs_per_file = 600; 
  c->shm_name = "/dev/shm/hk.bin"; 
}

int nuphase_hk_config_read(const char * file, nuphase_hk_cfg_t * c) 
{
  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

  if (!config_read_file(&cfg, file))
  {
     fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
     config_error_line(&cfg), config_error_text(&cfg));

     config_destroy(&cfg); 
     return 1; 
  }
  config_lookup_int(&cfg,"interval", &c->interval);
  config_lookup_int(&cfg,"max_secs_per_file", &c->max_secs_per_file);
  lookup_asps_method(&cfg, &c->asps_method, "asps_method"); 


  const char * outdir_str; 
  if (config_lookup_string(&cfg,"out_dir", &outdir_str))
  {
    c->out_dir = strdup(outdir_str); //memory leak, but not easy to do anything else here. 
  }
  const char * shm_str; 
  if (config_lookup_string(&cfg,"out_dir", &shm_str))
  {
    c->shm_name = strdup(shm_str); //memory leak, but not easy to do anything else here. 
  }


  config_destroy(&cfg); 
  return 0; 
}

int nuphase_hk_config_write(const char * file, const nuphase_hk_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 

  if (!f) return 1; 

  fprintf(f,"//Configuration file for nuphase-hk\n");  
  fprintf(f, "//Polling interval, in seconds. Treated as integer.  \n"); 
  fprintf(f, "interval=%d;\n\n", c->interval); 
  fprintf(f, "//Asps communication method. Valid values are \"serial\" and \"http\"\n"); 
  fprintf(f, "asps_method=%s;\n\n", c->asps_method == NP_ASPS_HTTP  ? "http" : "serial" ); 
  fprintf(f, "//max seconds per file, in seconds. Treated as integer.  \n"); 
  fprintf(f, "max_secs_per_file=%d;\n\n", c->max_secs_per_file); 
  fprintf(f, "//output directory \n"); 
  fprintf(f, "out_dir=%s;\n\n", c->out_dir); 
  fprintf(f, "//shared binary data name \n"); 
  fprintf(f, "shm_name=%s;\n\n", c->shm_name); 
  fclose(f); 

  return 0; 
}


/////////////////////////////////////////////////////
// copy config 
/////////////////////////////////////////////////////

void nuphase_copy_config_init(nuphase_copy_cfg_t * c) 
{
  c->remote_hostname = "radproc"; 
}


int nuphase_copy_config_read(const char * file, nuphase_copy_cfg_t * c) 
{

  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

 
  const char * remote_hostname_str; 
  if (config_lookup_string(&cfg,"remote_hostname", &remote_hostname_str))
  {
    c->remote_hostname = strdup(remote_hostname_str); //memory leak, but not easy to do anything else here. 
  } 

  config_destroy(&cfg); 
  return 0; 
}

int nuphase_copy_config_write(const char * file, const nuphase_copy_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 
  if (!f) return 1; 
  fprintf(f,"//Cofniguration file for nuphase-copy\n"); 
  fprintf(f,"remote_hostname=%s\n", c->remote_hostname); 
  fclose(f); 

  return 0; 
}



/////////////////////////////////////////////////////
// acq config 
/////////////////////////////////////////////////////



void nuphase_acq_config_init ( nuphase_acq_cfg_t * c) 
{
  c->spi_devices[0] = "/dev/spidev2.0"; 
  c->spi_devices[1] = "/dev/spidev1.0"; 
  c->run_file = "/data/runfile" ; 
  c->status_save_file = "/data/last.st.bin"; 
  c->output_directory = "/data/" ; 
  c->alignment_command = "/usr/bin/env python /home/nuphase/nuphase-python/align_adcs.py" ; 

  c->load_thresholds_from_status_file = 1; 

  int i; 
  for ( i = 0; i < NP_NUM_BEAMS; i++) c->scaler_goal[i] = 1; 


  //TODO tune this 
  c->k_p = 10; 
  c->k_i = 10; 
  c->k_d = 0; 

  c->trigger_mask = 0xe; 
  c->channel_mask = 0xf; 
  c->channel_read_mask[0] = 0xf;
  c->channel_read_mask[1] = 0x7;

  c->buffer_capacity = 100; 
  c->monitor_interval = 1.0; 
  c->sw_trigger_interval = 1; 
  c->print_interval = 5; 

  c->run_length = 7200; 
  c->spi_clock = 20; 
  c->waveform_length = 384; 
  c->enable_phased_trigger = 1; 
  c->calpulser_state = 0; 


  c->apply_attenuations = 1; 
  c->enable_trigout=1; 

  //provisional reasonable values 
  c->attenuation[0][0] = 9; 
  c->attenuation[0][1] = 3; 
  c->attenuation[0][2] = 11; 
  c->attenuation[0][3] = 8; 
  c->attenuation[0][4] = 7; 
  c->attenuation[0][5] = 19; 
  c->attenuation[0][6] = 11; 
  c->attenuation[0][7] = 3; 
  c->attenuation[1][0] = 23; 
  c->attenuation[1][1] = 10; 
  c->attenuation[1][2] = 27; 
  c->attenuation[1][3] = 27; 
  c->attenuation[1][4] = 0; 
  c->attenuation[1][5] = 0; 
  c->attenuation[1][6] = 0; 
  c->attenuation[1][7] = 0; 


  c->pretrigger = 4; 
  c->slow_scaler_weight = 0.3; 
  c->fast_scaler_weight = 0.7; 
  c->secs_before_phased_trigger = 20; 
  c->events_per_file = 1000; 
  c->status_per_file = 200; 
  c->n_fast_scaler_avg = 20; 
}

int nuphase_acq_config_read(const char * fi, nuphase_acq_cfg_t * c) 
{
  fprintf(stderr,"Not implemented yet"); 
  return 1; 
}

int nuphase_acq_config_write(const char * fi, const nuphase_acq_cfg_t * c) 
{

  FILE * f = fopen(fi,"w");  
  int i = 0; 
  if (!f) return -1; 
  fprintf(f,"// config file for nuphase-acq\n"); 
  fprintf(f,"// not all options are changeable by restart\n\n"); 

  fprintf(f,"// settings related to threshold  / trigger control\n"); 
  fprintf(f,"// These all can be set without restarting\n"); 
  fprintf(f,"control:\n"); 
  fprintf(f,"{\n"); 
  fprintf(f,"   // scaler goals for each beam, desired rate ( in Hz)\n"); 
  fprintf(f,"   scaler_goals = {\n"); 
  for (i = 0; i < NP_NUM_BEAMS; i++)
  {
    fprintf(f, "     %d : %g%c\n", i, c->scaler_goal[i], i < NP_NUM_BEAMS - 1 ? ',' : ' '); 
  }
  fprintf(f,"    }\n\n"); 

  fprintf(f,"   //the beams allowed to participate in the trigger\n"); 
  fprintf(f,"   trigger_mask = 0x%x;\n\n", c->trigger_mask);  

  fprintf(f,"   // the channels on the master allowed to participate in the trigger\n"); 
  fprintf(f,"   channel_mask = 0x%x;\n\n", c->channel_mask); 

  fprintf(f,"   // pid loop proportional term "); 
  fprintf(f,"   k_p = %g;\n\n", c->k_p); 

  fprintf(f,"   // pid loop integral term\n"); 
  fprintf(f,"   k_i = %g;\n\n", c->k_i);

  fprintf(f,"   // pid loop differential term\n"); 
  fprintf(f,"   k_d = %g;\n\n", c->k_d);

  fprintf(f,"   //monitoring interval, for PID loop (in seconds)\n"); 
  fprintf(f,"   monitor_interval = %g;\n\n",c->monitor_interval); 

  fprintf(f,"   // software trigger interval (in seconds)\n"); 
  fprintf(f,"   sw_trigger_interval = %g;\n\n", c->sw_trigger_interval); 

  fprintf(f,"   //enable the phased trigger readout\n"); 
  fprintf(f,"   enable_phased_trigger = %d;\n\n",c->enable_phased_trigger); 

  fprintf(f,"   //delay for phased trigger to start\n"); 
  fprintf(f,"   secs_before_phased_trigger = %d;\n\n", c->secs_before_phased_trigger); 

  fprintf(f,"   //weight of fast scaler in pid loop\n"); 
  fprintf(f,"   fast_scaler_weight = %g;\n\n", c->fast_scaler_weight); 

  fprintf(f,"   //weight of slow scaler in pid loop\n"); 
  fprintf(f,"   slow_scaler_weight = %g;\n\n", c->slow_scaler_weight); 

  fprintf(f,"  //number of fast scalers to average\n"); 
  fprintf(f,"  n_fast_scaler_avg = %d\n\n", c->n_fast_scaler_avg); 

  fprintf(f,"  //File to persist the status info (primarily for saving thresholds between restarts)\n") ;
  fprintf(f,"  status_save_file = %s\n\n", c->status_save_file); 

  fprintf(f,"  // load thresholds from status file on start.\n");  
  fprintf(f,"  load_thresholds_from_status_file=%d\n\n", c->load_thresholds_from_status_file); 


  fprintf(f,"};\n\n"); 

  fprintf(f,"// settings related to the acquisition\n"); 
  fprintf(f,"// Not all of these can be set without restarting\n"); 
  fprintf(f,"device: \n");
  fprintf(f,"{\n"); 
  fprintf(f,"  //spi devices, master first, requires restart to change\n"); 
  fprintf(f,"  spi_devices = (\"%s\", \"%s\"); \n\n", c->spi_devices[0], c->spi_devices[1]); 

  
  fprintf(f,"  // circular buffer capacity. In-memory storage in between acquisition and writing. Requires restart.\n"); 
  fprintf(f,"  buffer_capacity = %d;\n\n", c->buffer_capacity); 

  fprintf(f,"  //the length of a waveform, in samples. \n"); 
  fprintf(f,"  waveform_length = %d;\n\n", c->waveform_length); 

  fprintf(f,"  //the pretrigger window length, in hardware units\n"); 
  fprintf(f,"  pretrigger = %d;\n\n", c->pretrigger); 

  fprintf(f,"  //calpulser state, 0 (off) , 2 (baseline)  or 3 (calpulser)\n"); 
  fprintf(f,"  calpulser_state = %d;\n\n", c->calpulser_state); 

  fprintf(f,"  // Whether or not to enable the external triggers\n"); 
  fprintf(f,"  enable_trigout = %d;\n\n", c->enable_trigout); 

  fprintf(f,"  //spi clock speed, MHz\n"); 
  fprintf(f,"  spi_clock = %d;\n\n", c->spi_clock); 
 
  fprintf(f,"  // True to apply attenuations \n"); 
  fprintf(f,"  apply_attenuations = %d;\n\n", c->apply_attenuations); 

  fprintf(f,"  // attenuation, per channel, if applied. \n"); 

  fprintf(f,"  attenuation = ( {"); 
  for (i = 0; i < NP_NUM_CHAN; i++)
    fprintf(f, i < NP_NUM_CHAN -1 ? "%d:%d," : "%d:%d", i, c->attenuation[0][i]); 
  fprintf(f, "},{"); 
  for (i = 0; i < NP_NUM_CHAN; i++) 
    fprintf(f, i < NP_NUM_CHAN -1 ? "%d:%d," : "%d:%d", i, c->attenuation[1][i]); 
  fprintf(f,"} );\n\n"); 

  fprintf(f,"  //which channels to digitize\n"); 
  fprintf(f,"  channel_read_mask = (0x%x, 0x%x); \n\n", c->channel_read_mask[0], c->channel_read_mask[1]); 

  fprintf(f,"  //command used to run the alignment program.\n"); 
  fprintf(f,"  alignment_command=\"%s\",\n\n", c->alignment_command); 

  fprintf(f,"};\n\n"); 


  fprintf(f,"//settings related to output\n"); 
  fprintf(f,"output: \n") ; 
  fprintf(f,"{\n"); 

  fprintf(f,"  // Run file, used to persist run number\n"); 
  fprintf(f,"  run_file = \"%s\";\n\n", c->run_file);  

  fprintf(f,"  // output directory, data will go here\n"); 
  fprintf(f,"  output_directory = \"%s\" ;\n\n", c->output_directory); 

  fprintf(f,"  //print to screen interval (0 to disable)\n"); 
  fprintf(f,"  print_interval = %d;\n\n", c->print_interval); 

  fprintf(f,"  // run length, in seconds\n"); 
  fprintf(f,"  run_length = %d; \n\n",c->run_length); 

  fprintf(f,"  //events per output file\n");
  fprintf(f,"  events_per_file = %d;\n\n", c->events_per_file); 

  fprintf(f,"  //statuses per output file\n"); 
  fprintf(f,"  status_per_file = %d;\n\n", c->status_per_file); 

  fprintf(f,"};\n\n"); 

  return 0; 
}








