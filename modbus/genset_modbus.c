#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "genset_modbus.h"
#include "../dumpcan.h"

void sigterm(int signo)
{
	//printf("setting running to 0\n");
	running = 0;
	mode = 0; 
	sleep(1);
	
	exit(1);
}

typedef struct _modbus modbus_t;
modbus_mapping_t* genset_modbus_mapping_new(uint8_t* coils, uint16_t num_coils,
   uint8_t* inputs, uint16_t num_inputs,
   uint16_t* regs, uint16_t num_regs,
   uint16_t* input_regs, uint16_t num_input_regs)
{
   modbus_mapping_t *map;

   map = (modbus_mapping_t*)malloc(sizeof(modbus_mapping_t));
   if (map == NULL)
      return NULL;

   /* 0xxxx Addresses */
   map->nb_bits = num_coils;
   map->tab_bits = coils;

   /* 1xxxx Addresses */
   map->nb_input_bits = num_inputs;
   map->tab_input_bits = inputs;

   /* 4xxxx Addresses */
   map->nb_registers = num_regs;
   map->tab_registers = regs;

   /* 3xxxx Addresses */
   map->nb_input_registers = num_input_regs;
   map->tab_input_registers = input_regs;

   return map;
}

void genset_modbus_mapping_init(modbus_mapping_t* map)
{
   int reg;

   for (reg=0; reg < (map->nb_input_registers); reg++)
      map->tab_input_registers[reg] = 0;
  
	for (reg=0; reg < (map->nb_registers); reg++)
      map->tab_registers[reg] = 0;
}

int genset_modbus_server_start(modbus_mapping_t* pMap)
{
    int s = -1;
    modbus_t* ctx;
    modbus_mapping_t* gensetMapping;
   
   signal(SIGINT, sigterm);
   signal(SIGHUP, sigterm);
   signal(SIGINT, sigterm);
   // setup a tcp modbus context
   // 502 is default port.  If admin priv's required try 1502 to run as user
   ctx = modbus_new_tcp_pi("::0", "502");

   modbus_set_debug(ctx, TRUE);

   gensetMapping = pMap;

   if (gensetMapping == NULL)
   {
	   
      fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
      modbus_free(ctx);
      return -1;
   }
#ifdef DEBUG
	printf("Starting server \n ");
#endif
   for (;;)
   {
		  s = modbus_tcp_pi_listen(ctx, 1);
		  modbus_tcp_pi_accept(ctx, &s);

		  for (;;)
		  {
			 uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
			 int rc;
			 
			//printf("mode %d \n ", mode);
				
			 if(mode == 10){ 
				close(s);
				printf("closing socket");
				return 1;
			 } else {
				rc = modbus_receive(ctx, query);
			 }
			
			if (rc > 0)
			 {

				modbus_reply(ctx, query, rc, gensetMapping);
				
			 }
			 else if (rc == -1)
			 {
				mode = 10; 
				close(s); 
				return 1;

			 }
		  }
      if (s != -1)
	   close(s); 
	//} 		 
   }
#ifdef DEBUG
   printf("Getting out of modbus server \n");
#endif
   return 0; 
}