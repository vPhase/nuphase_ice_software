#ifndef _NUPHASE_COMMON_H 
#define _NUPHASE_COMMON_H 
#include <time.h> 

/** Various common things used by multiple programs */ 



/** Get the difference between two timespecs as a float */ 
float timespec_difference_float(const struct timespec * a, const struct timespec * b); 


typedef enum nuphase_program
{
  NUPHASE_STARTUP, 
  NUPHASE_HK, 
  NUPHASE_ACQ, 
  NUPHASE_COPY 
}  nuphase_program_t; 


#define CONFIG_DIR_ENV "NUPHASE_CONFIG_DIR" 
#define CONFIG_DIR_ACQ_NAME "acq.cfg" 
#define CONFIG_DIR_COPY_NAME "copy.cfg" 
#define CONFIG_DIR_HK_NAME "hk.cfg" 
#define CONFIG_DIR_STARTUP_NAME "startup.cfg" 
#define CONFIG_REREAD_SIGNAL SIGUSR1; 


int nuphase_get_cfg_file(char ** name, nuphase_program_t program); 


/* a bunch of directory making things */ 



/* Makes a directory if not already there 
 * If 0 is returned, it should be there */ 
int mkdir_if_needed(const char * path); 


/** Makes directories like:  
 *   prefix/(patttern} 
 *
 *   where pattern is something like yyyy/mm/dd/hh/ or sometihng like that 
 */ 
int mkdirs_for_time(const char * prefix, const char * pattern,  time_t when); 



#endif
