#ifndef DUMP_CAN
#define DUMP_CAN

#include <stdio.h>
#include "modbus/genset_modbus.h"

extern int sysMode; 
extern int running;
extern volatile int mode;

void pointToMap(modbus_mapping_t *map);
char *genSend(); 
int candump();
#endif