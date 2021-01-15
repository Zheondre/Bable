#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "genset_modbus.h"
#include "genset_regs.h"

int main (int argc, char* argv[])
{
   modbus_mapping_t* pMap;
   uint16_t* pGensetRegs;

   pGensetRegs = (uint16_t*)malloc(NumRegs * sizeof(uint16_t));
   if (pGensetRegs == NULL)
      return 1;

   pMap = genset_modbus_mapping_new(NULL, 0, NULL, 0, NULL, 0, pGensetRegs, NumRegs);
   if (pMap == NULL)
   {
      free (pGensetRegs);
      return 2;
   }

   genset_modbus_mapping_init(pMap);

   return genset_modbus_server_start(pMap);
}

