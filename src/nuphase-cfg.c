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
  c->set_attenuation_cmd = "cd /home/nuphase/nuphase-python; python set_attenuation.py"; 
  c->reconfigure_fpga_cmd = "cd /home/nuphase/nuphase-python; ./reconfigureFPGA -a 1; ./reconfigureFPGA -a 0; "; 
  c->desired_rms_master = 4.2; 
  c->desired_rms_slave = 7.0; 
  c->out_dir = "/data/startup/"; 
  c->nchecks = 3; 
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
  config_lookup_int(&cfg,"nchecks", &c->nchecks);
  config_lookup_int(&cfg,"heater_current", &c->heater_current);
  config_lookup_int(&cfg,"poll_interval", &c->poll_interval);
  config_lookup_float(&cfg,"desired_rms_master", &c->desired_rms_master);
  config_lookup_float(&cfg,"desired_rms_slave", &c->desired_rms_slave);

  lookup_asps_method(&cfg, &c->asps_method, "asps_method"); 

  const char * attenuation_cmd; 
  if (config_lookup_string(&cfg, "set_attenuation_cmd", &attenuation_cmd))
  {
    c->set_attenuation_cmd = strdup(attenuation_cmd); //memory leak :( 

 
  const char * reconfigure_cmd; 
  if (config_lookup_string(&cfg, "reconfigure_fpga_cmd", &reconfigure_cmd))
  {
    c->reconfigure_fpga_cmd = strdup(reconfigure_cmd); //memory leak :( 
  }
 }

  const char * out_dir; 
  if (config_lookup_string(&cfg, "out_dir", &out_dir))
  {
    c->out_dir = strdup(out_dir); //memory leak :( 
  }


  config_destroy(&cfg); 
  return 0; 
}

int nuphase_start_config_write(const char * file, const nuphase_start_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 

  if (!f) return 1; 

  fprintf(f,"//Configuration file for nuphase-start\n\n");  
  fprintf(f, "//Minimum temperature, in C, to turn on FPGA's\n"); 
  fprintf(f, "min_temperature=%d;\n\n", c->min_temperature); 
  fprintf(f,"// Current in mA to run the heater at\n"); 
  fprintf(f, "heater_current=%d;\n\n", c->heater_current); 
  fprintf(f,"// ASPS method, \"serial\" or \"http\"\n"); 
  fprintf(f,"asps_method = \"%s\";\n\n", c->asps_method == NP_ASPS_HTTP ? "http": "serial");  
  fprintf(f,"// The polling interval (to check temperature and heater state) in seconds\n"); 
  fprintf(f,"poll_interval = %d;\n\n", c->poll_interval); 
  fprintf(f,"// Command to run after turning on boards to tune attenuations \n"); 
  fprintf(f,"set_attenuation_cmd = \"%s\";\n\n", c->set_attenuation_cmd); 
  fprintf(f,"// Command to run after turning on boards to reconfigure FGPA's\n"); 
  fprintf(f,"reconfigure_fpga_cmd = \"%s\";\n\n", c->reconfigure_fpga_cmd); 
  fprintf(f,"//rms goal for master \n"); 
  fprintf(f,"desired_rms_master = %f;\n\n", c->desired_rms_master); 
  fprintf(f,"//rms goal for slave \n"); 
  fprintf(f,"desired_rms_slave = %f;\n\n", c->desired_rms_slave); 
  fprintf(f, "//output directory \n"); 
  fprintf(f, "out_dir=\"%s\";\n\n", c->out_dir); 
  fprintf(f, "//number of checks for each temperature\n"); 
  fprintf(f, "nchecks=%d;\n\n", c->nchecks); 
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
  c->shm_name = "/hk.bin"; 
  c->print_to_screen = 1; 
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
  config_lookup_int(&cfg,"print_to_screen", &c->print_to_screen);
  config_lookup_int(&cfg,"max_secs_per_file", &c->max_secs_per_file);
  lookup_asps_method(&cfg, &c->asps_method, "asps_method"); 


  const char * outdir_str; 
  if (config_lookup_string(&cfg,"out_dir", &outdir_str))
  {
    c->out_dir = strdup(outdir_str); //memory leak, but not easy to do anything else here. 
  }
  const char * shm_str; 
  if (config_lookup_string(&cfg,"shm_name", &shm_str))
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
  fprintf(f, "asps_method=\"%s\";\n\n", c->asps_method == NP_ASPS_HTTP  ? "http" : "serial" ); 
  fprintf(f, "//max seconds per file, in seconds. Treated as integer.  \n"); 
  fprintf(f, "max_secs_per_file=%d;\n\n", c->max_secs_per_file); 
  fprintf(f, "//output directory \n"); 
  fprintf(f, "out_dir=\"%s\";\n\n", c->out_dir); 
  fprintf(f, "//shared binary data name \n"); 
  fprintf(f, "shm_name=\"%s\";\n\n", c->shm_name); 
  fprintf(f, "//1 to print to screen\n"); 
  fprintf(f, "print_to_screen=%d;\n\n", c->print_to_screen); 
  fclose(f); 

  return 0; 
}


/////////////////////////////////////////////////////
// copy config 
/////////////////////////////////////////////////////

void nuphase_copy_config_init(nuphase_copy_cfg_t * c) 
{
  c->remote_user = "radio" ;
  c->remote_hostname = "radproc"; 
  c->local_path = "/data" ;
  c->remote_path = "/data/nuphase" ;
  c->free_space_delete_threshold = 12000; 
  c->delete_files_older_than = 7;  // ? hopefully this is enough! 
  c->wakeup_interval = 600; // every 10 mins
  c->dummy_mode = 0; 
}


int nuphase_copy_config_read(const char * file, nuphase_copy_cfg_t * c) 
{

  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg,CONFIG_TRUE); 

  config_read_file(&cfg,file); 
 
  const char * remote_hostname_str; 
  if (config_lookup_string(&cfg,"remote_hostname", &remote_hostname_str))
  {
    c->remote_hostname = strdup(remote_hostname_str); //memory leak, but not easy to do anything else here. 
  } 

  const char * remote_path_str; 
  if (config_lookup_string(&cfg,"remote_path", &remote_path_str))
  {
    c->remote_path = strdup(remote_path_str); //memory leak, but not easy to do anything else here. 
  } 

  const char * remote_user_str; 
  if (config_lookup_string(&cfg,"remote_user", &remote_user_str))
  {
    c->remote_user = strdup(remote_user_str); //memory leak, but not easy to do anything else here. 
  } 

  const char * local_path_str; 
  if (config_lookup_string(&cfg,"local_path", &local_path_str))
  {
    c->local_path = strdup(local_path_str); //memory leak, but not easy to do anything else here. 
  } 

  config_lookup_int(&cfg,"free_space_delete_threshold",&c->free_space_delete_threshold); 
  config_lookup_int(&cfg,"delete_files_older_than",&c->delete_files_older_than); 
  config_lookup_int(&cfg,"wakeup_interval",&c->wakeup_interval); 
  config_lookup_int(&cfg,"dummy_mode",&c->dummy_mode); 


  config_destroy(&cfg); 


  return 0; 
}

int nuphase_copy_config_write(const char * file, const nuphase_copy_cfg_t * c) 
{
  FILE * f = fopen(file,"w"); 
  if (!f) return 1; 
  fprintf(f,"//Configuration file for nuphase-copy\n\n"); 
  fprintf(f,"//The host to copy data to\n"); 
  fprintf(f,"remote_hostname = \"%s\";\n\n", c->remote_hostname); 
  fprintf(f,"//The remote path to copy data to\n"); 
  fprintf(f,"remote_path = \"%s\";\n\n", c->remote_path); 
  fprintf(f,"//The remote user to copy data as (if you didn't set up ssh keys, this won't work so well)\n"); 
  fprintf(f,"remote_user = \"%s\";\n\n", c->remote_user); 
  fprintf(f,"//The local path to copy data from (note that the CONTENTS of this directory are copied, e.g. an extra / is added to the rsync source)\n"); 
  fprintf(f,"local_path = \"%s\";\n\n", c->local_path); 
  fprintf(f,"//Only attempt to automatically delete old files when free space is below this threshold (in MB)\n"); 
  fprintf(f,"free_space_delete_threshold = %d;\n\n", c->free_space_delete_threshold); 
  fprintf(f,"//Delete files from local path with modifications times GREATER than this number of days (i.e. if 7, will delete files 8 days and older)\n"); 
  fprintf(f,"delete_files_older_than = %d;\n\n", c->delete_files_older_than); 
  fprintf(f,"//This controls how long the process sleeps between copies / deletes\n"); 
  fprintf(f,"wakeup_interval = %d;\n\n", c->wakeup_interval); 
  fprintf(f,"//If non-zero, won't actually delete any files\n"); 
  fprintf(f,"dummy_mode = %d;\n\n", c->dummy_mode); 
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
  c->run_file = "/nuphase/runfile" ; 
  c->status_save_file = "/nuphase/last.st.bin"; 
  c->output_directory = "/data/" ; 
  c->alignment_command = "cd /home/nuphase/nuphase-python/;  python align_adcs.py" ; 

  c->load_thresholds_from_status_file = 1; 

  int i; 
  for ( i = 0; i < NP_NUM_BEAMS; i++) c->scaler_goal[i] = 1; 


  //TODO tune this 
  c->k_p = 10; 
  c->k_i = 10; 
  c->k_d = 0; 
  c->max_threshold_increase = 500; 
  c->trigger_mask = 0x7fff; 
  c->channel_mask = 0xff; 
  c->channel_read_mask[0] = 0xff;
  c->channel_read_mask[1] = 0xf;
  c->min_threshold = 5000; 

  c->buffer_capacity = 100; 
  c->monitor_interval = 1.0; 
  c->sw_trigger_interval = 1; 
  c->print_interval = 5; 
  c->poll_usecs = 500; 

  c->run_length = 7200; 
  c->spi_clock = 20; 
  c->waveform_length = 512; 
  c->enable_phased_trigger = 1; 
  c->calpulser_state = 0; 


  c->apply_attenuations = 0; 
  c->enable_trigout=1; 
  c->enable_extin = 0; 
  c->trigout_width = 3; 
  c->disable_trigout_on_exit = 1; 

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
  c->subtract_gated = 1; 
  c->secs_before_phased_trigger = 20; 
  c->events_per_file = 1000; 
  c->status_per_file = 200; 
  c->n_fast_scaler_avg = 20; 
  c->realtime_priority = 20; 

  c->copy_paths_to_rundir = "/home/nuphase/nuphase-python/output:/proc/loadavg"; 
  c->copy_configs = 1; 
  memset(c->trig_delays,0,sizeof(c->trig_delays)); 
}

int nuphase_acq_config_read(const char * fi, nuphase_acq_cfg_t * c) 
{

  config_t cfg; 
  config_init(&cfg); 
  config_set_auto_convert(&cfg, CONFIG_TRUE); 
  if (!config_read_file(&cfg, fi))
  {
     fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
     config_error_line(&cfg), config_error_text(&cfg));

     config_destroy(&cfg); 
     return 1; 
  }
 
  
  int i; 
  for (i = 0; i < NP_NUM_BEAMS; i++) 
  {
    char buf[128]; 
    sprintf(buf, "control.scaler_goal.beam%d",i); 
    config_lookup_float(&cfg, buf, &c->scaler_goal[i]); 
  }

  int tmp; 
  if ( config_lookup_int(&cfg,"control.trigger_mask",&tmp))
    c->trigger_mask = tmp; 
  if (config_lookup_int(&cfg,"control.channel_mask",&tmp))
    c->channel_mask = tmp; 
  config_lookup_float(&cfg,"control.k_p",&c->k_p); 
  config_lookup_float(&cfg,"control.k_i",&c->k_i); 
  config_lookup_float(&cfg,"control.k_d",&c->k_d); 
  config_lookup_int(&cfg,"control.max_threshold_increase",&tmp); 
  c->max_threshold_increase = tmp; 
  config_lookup_int(&cfg,"control.min_threshold",&tmp); 
  c->min_threshold = tmp; 
  config_lookup_float(&cfg,"control.monitor_interval",&c->monitor_interval); 
  config_lookup_float(&cfg,"control.sw_trigger_interval",&c->sw_trigger_interval); 
  config_lookup_int(&cfg,"control.enable_phased_trigger",&c->enable_phased_trigger); 
  config_lookup_int(&cfg,"control.secs_before_phased_trigger",&c->secs_before_phased_trigger); 
  config_lookup_float(&cfg,"control.fast_scaler_weight",&c->fast_scaler_weight); 
  config_lookup_float(&cfg,"control.slow_scaler_weight",&c->slow_scaler_weight); 
  config_lookup_int(&cfg,"control.n_fast_scaler_avg",&c->n_fast_scaler_avg); 
  config_lookup_int(&cfg,"control.subtract_gated",&c->subtract_gated); 
  config_lookup_int(&cfg,"realtime_priority",&c->realtime_priority); 
  config_lookup_int(&cfg,"poll_usecs",&tmp); 
  c->poll_usecs = tmp; 



  const char * status_save = 0; 

  if (config_lookup_string(&cfg, "control.status_save_file", &status_save))
  {
    c->status_save_file = strdup(status_save); 
  }

  config_lookup_int(&cfg,"control.load_thresholds_from_status_file",&c->load_thresholds_from_status_file); 


  const char *spi = 0; 

  if (config_lookup_string(&cfg, "device.spi_devices.[0]", &spi))
  {
    c->spi_devices[0] = strdup(spi); 
  }
 
  if (config_lookup_string(&cfg, "device.spi_devices.[1]", &spi))
  {
    c->spi_devices[1] = strdup(spi); 
  }



  config_lookup_int(&cfg,"device.buffer_capacity", &c->buffer_capacity); 
  config_lookup_int(&cfg,"device.waveform_length", &c->waveform_length); 
  config_lookup_int(&cfg,"device.pretrigger", &c->pretrigger); 
  config_lookup_int(&cfg,"device.calpulser_state", &c->calpulser_state); 
  config_lookup_int(&cfg,"device.enable_trigout", &c->enable_trigout); 
  config_lookup_int(&cfg,"device.enable_extin", &c->enable_extin); 
  config_lookup_int(&cfg,"device.trigout_width", &c->trigout_width); 
  config_lookup_int(&cfg,"device.disable_trigout_on_exit", &c->disable_trigout_on_exit); 
  config_lookup_int(&cfg,"device.spi_clock", &c->spi_clock); 
  config_lookup_int(&cfg,"device.apply_attenuations", &c->apply_attenuations); 

  int b;
  for (b = 0; b < NP_MAX_BOARDS; b++)
  {
    for (i = 0; i < NP_NUM_CHAN; i++) 
    {
      char buf[128]; 
      sprintf(buf,"device.attenuation.[%d].ch%d", b,i); 
      if (config_lookup_int(&cfg,buf, &tmp) )
      {
        c->attenuation[b][i]=tmp; 
      }
    }
  }

  
  if (config_lookup_int(&cfg, "device.channel_read_mask.[0]", &tmp)) 
    c->channel_read_mask[0] = tmp; 
  if (config_lookup_int(&cfg, "device.channel_read_mask.[1]", &tmp))
    c->channel_read_mask[1] = tmp; 

  const char * cmd; 
  if (config_lookup_string(&cfg, "device.alignment_command", &cmd) )
  {
    c->alignment_command = strdup (cmd); 
  }

  const char * run_file ; 
  if (config_lookup_string( &cfg, "output.run_file", &run_file))
  {
    c->run_file = strdup(run_file); 
  }

  const char * output_directory ; 
  if (config_lookup_string( &cfg, "output.output_directory", &output_directory))
  {
    c->output_directory = strdup(output_directory); 
  }

  const char * copy_paths; 
  if (config_lookup_string( &cfg, "output.copy_paths_to_rundir", &copy_paths))
  {
    c->copy_paths_to_rundir = strdup(copy_paths); 
  }


  config_lookup_int(&cfg,"output.print_interval", &c->print_interval); 
  config_lookup_int(&cfg,"output.run_length", &c->run_length); 
  config_lookup_int(&cfg,"output.events_per_file", &c->events_per_file); 
  config_lookup_int(&cfg,"output.status_per_file", &c->status_per_file); 
  config_lookup_int(&cfg,"output.copy_configs", &c->copy_configs); 

  for (i = 0; i < NP_NUM_CHAN; i++)
  {
    char buf[128]; 
    sprintf(buf,"device.trig_delays.ch%d",i); 
    if (config_lookup_int(&cfg,buf,&tmp))
    {
      c->trig_delays[i] = tmp; 
    }
  }



  return 0; 

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
  fprintf(f,"   scaler_goal = {\n"); 
  for (i = 0; i < NP_NUM_BEAMS; i++)
  {
    fprintf(f, "     beam%d : %g;\n", i, c->scaler_goal[i]); 
  }
  fprintf(f,"    };\n\n"); 

  fprintf(f,"   //the beams allowed to participate in the trigger\n"); 
  fprintf(f,"   trigger_mask = 0x%x;\n\n", c->trigger_mask);  

  fprintf(f,"   // the channels on the master allowed to participate in the trigger\n"); 
  fprintf(f,"   channel_mask = 0x%x;\n\n", c->channel_mask); 

  fprintf(f,"   // pid loop proportional term\n"); 
  fprintf(f,"   k_p = %g;\n\n", c->k_p); 

  fprintf(f,"   // pid loop integral term\n"); 
  fprintf(f,"   k_i = %g;\n\n", c->k_i);

  fprintf(f,"   // pid loop differential term\n"); 
  fprintf(f,"   k_d = %g;\n\n", c->k_d);

  fprintf(f,"   // max threshold increase per step \n"); 
  fprintf(f,"   max_threshold_increase=%u;\n\n", c->max_threshold_increase); 

  fprintf(f,"   // minimum threshold for any beam\n"); 
  fprintf(f,"   min_threshold=%u;\n\n", c->min_threshold); 

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

  fprintf(f,"   //number of fast scalers to average\n"); 
  fprintf(f,"   n_fast_scaler_avg = %d;\n\n", c->n_fast_scaler_avg); 

  fprintf(f,"   //Whether or not to subtract off gated scalers\n"); 
  fprintf(f,"   subtract_gated = %d;\n\n", c->subtract_gated); 


  fprintf(f,"   //File to persist the status info (primarily for saving thresholds between restarts)\n") ;
  fprintf(f,"   status_save_file = \"%s\"\n\n", c->status_save_file); 

  fprintf(f,"   // load thresholds from status file on start.\n");  
  fprintf(f,"   load_thresholds_from_status_file=%d\n\n", c->load_thresholds_from_status_file); 


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

  fprintf(f,"  // Whether or not to enable the trigger output\n"); 
  fprintf(f,"  enable_trigout = %d;\n\n", c->enable_trigout); 

  fprintf(f,"  // Whether or not to enable external trigger input\n"); 
  fprintf(f,"  enable_extin = %d;\n\n", c->enable_extin); 

  fprintf(f,"  // The width of the trigger output in 40 ns intervals\n"); 
  fprintf(f,"  trigout_width = %d;\n\n", c->trigout_width); 

  fprintf(f,"  // Whether or not to disable the trigger output on exit\n"); 
  fprintf(f,"  disable_trigout_on_exit = %d;\n\n", c->disable_trigout_on_exit); 

  fprintf(f,"  //spi clock speed, MHz\n"); 
  fprintf(f,"  spi_clock = %d;\n\n", c->spi_clock); 
 
  fprintf(f,"  // True to apply attenuations \n"); 
  fprintf(f,"  apply_attenuations = %d;\n\n", c->apply_attenuations); 

  fprintf(f,"  // attenuation, per channel, if applied. \n"); 

  fprintf(f,"  attenuation = ( \n               {"); 
  for (i = 0; i < NP_NUM_CHAN; i++)
    fprintf(f,  "ch%d: %d;  " , i, c->attenuation[0][i]); 
  fprintf(f, "} ,\n               {"); 
  for (i = 0; i < NP_NUM_CHAN; i++) 
    fprintf(f,  "ch%d: %d;  " , i, c->attenuation[1][i]); 
  fprintf(f,"} ) ;\n\n"); 

  fprintf(f,"  //which channels to digitize\n"); 
  fprintf(f,"  channel_read_mask = (0x%x, 0x%x); \n\n", c->channel_read_mask[0], c->channel_read_mask[1]); 

  fprintf(f,"  //command used to run the alignment program.\n"); 
  fprintf(f,"  alignment_command=\"%s\",\n\n", c->alignment_command); 

  fprintf(f,"  //channel trig delays (right now can be 0-3)\n"); 
  fprintf(f,"  trig_delays = {\n"); 
  for (i = 0; i < NP_NUM_CHAN; i++)
  {
    fprintf(f, "     ch%d : %u;\n", i, c->trig_delays[i]); 
  }
  fprintf(f,"  };\n\n"); 
 
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

  fprintf(f,"  //realtime priority setting. If 0, will use non-realtime priority. Otherwise, SCHED_FIFO is used with the given priority\n"); 
  fprintf(f,"  realtime_priority = %d;\n\n", c->realtime_priority); 

  fprintf(f,"  //Interval between polling SPI link for data. 0 to just sched_yield\n"); 
  fprintf(f,"  poll_usecs = %u;\n\n", c->poll_usecs); 

  fprintf(f,"  // Colon separated list of paths to copy (recursively) into run dir at start of run\n");
  fprintf(f,"  copy_paths_to_rundir = \"%s\";\n\n", c->copy_paths_to_rundir); 

  fprintf(f,"  //Whether or not to copy configs into run dir\n"); 
  fprintf(f,"  copy_configs = %d;\n", c->copy_configs); 

  fprintf(f,"};\n\n"); 

  return 0; 
}








