#ifndef _GENSET_MODBUS_H_
#define _GENSET_MODBUS_H_

#include "modbus.h"
extern volatile int mode;
modbus_mapping_t* genset_modbus_mapping_new(uint8_t* coils, uint16_t num_coils,
   uint8_t* inputs, uint16_t num_inputs,
   uint16_t* regs, uint16_t num_regs,
   uint16_t* input_regs, uint16_t num_input_regs);
void genset_modbus_mapping_init(modbus_mapping_t* map);
int genset_modbus_server_start(modbus_mapping_t* pMap);

#endif
