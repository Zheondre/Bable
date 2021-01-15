/*
 * canItf.c
 *
 * Copyright (c) 2002-2009 Volkswagen Group Electronic Research
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Send feedback to <linux-can@vger.kernel.org>
 *
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <stdbool.h>

#include <linux/can/raw.h>

#include <syscall.h>

#include "terminal.h"
#include "lib.h"
#include "dumpcan.h"
#include "genset_regs.h"

/* for hardware timestamps - since Linux 2.6.30 */
#ifndef SO_TIMESTAMPING
#define SO_TIMESTAMPING 37
#endif

/* from #include <linux/net_tstamp.h> - since Linux 2.6.30 */
#define SOF_TIMESTAMPING_SOFTWARE (1<<4)
#define SOF_TIMESTAMPING_RX_SOFTWARE (1<<3)
#define SOF_TIMESTAMPING_RAW_HARDWARE (1<<6)

#define MAXSOCK 16    /* max. number of CAN interfaces given on the cmdline */
#define MAXIFNAMES 30 /* size of receive name index to omit ioctls */
#define MAXCOL 6      /* number of different colors for colorized output */
#define ANYDEV "any"  /* name of interface to receive from any CAN interface */
#define ANL "\r\n"    /* newline in ASC mode */

#define SILENT_INI 42 /* detect user setting on commandline */
#define SILENT_OFF 0  /* no silent mode */
#define SILENT_ANI 1  /* silent mode with animation */
#define SILENT_ON  2  /* silent mode (completely silent) */

#define BOLD    ATTBOLD
#define RED     ATTBOLD FGRED
#define GREEN   ATTBOLD FGGREEN
#define YELLOW  ATTBOLD FGYELLOW
#define BLUE    ATTBOLD FGBLUE
#define MAGENTA ATTBOLD FGMAGENTA
#define CYAN    ATTBOLD FGCYAN


#define UIMODEON 1
//#define DEBUG 1

//struct timeval tval_before, tval_after, tval_result;

const char col_on [MAXCOL][19] = {BLUE, RED, GREEN, BOLD, MAGENTA, CYAN};
const char col_off [] = ATTRESET;

static char *cmdlinename[MAXSOCK];
static __u32 dropcnt[MAXSOCK];
static __u32 last_dropcnt[MAXSOCK];
static char devname[MAXIFNAMES][IFNAMSIZ+1];
static int  dindex[MAXIFNAMES];
static int  max_devname_len; /* to prevent frazzled device name output */ 
const int canfd_on = 1;

#define MAXANI 4
const char anichar[MAXANI] = {'|', '/', '-', '\\'};
const char extra_m_info[4][4] = {"- -", "B -", "- E", "B E"};

extern int optind, opterr, optopt;

int running = 1;

static modbus_mapping_t *pModbusmap; 
/*
void sigterm(int signo)
{
	//printf("setting running to 0\n");
	running = 0;
	mode = 0; 
} */ 

void pointToMap(modbus_mapping_t *pMbusmap){ 
	pModbusmap = pMbusmap; 
}
//////////////////////////////////////////////////////////////////////////////////////////////

int32_t getmodbus8bit( modbus_mapping_t* pModbusMap, GensetRegisters2 nIndex)
{//grabs the modbos message 

        int32_t nRetVal = -1;

        if (    pModbusMap == NULL ||
                pModbusMap->tab_registers == NULL)
                return nRetVal;

        return pModbusMap->tab_registers[nIndex];
}

int32_t format16bit(uint32_t nLength, uint8_t* pData, modbus_mapping_t* pModbusMap, GensetRegisters nIndex)
{
        int32_t nRetVal = -1;

        if (pData == NULL ||
                pModbusMap == NULL ||
                pModbusMap->tab_input_registers == NULL ||
                nLength < 2)
                return nRetVal;

        pModbusMap->tab_input_registers[nIndex] = ((uint16_t)pData[0] << 8) | pData[1];

        nRetVal = 0;
        return nRetVal;
}
int32_t format8bit(uint32_t nLength, uint8_t* pData, modbus_mapping_t* pModbusMap, GensetRegisters nIndex)
{
        int32_t nRetVal = -1;

        if (pData == NULL ||
                pModbusMap == NULL ||
                pModbusMap->tab_input_registers == NULL ||
                nLength < 1)
                return nRetVal;

        pModbusMap->tab_input_registers[nIndex] = pData[0];

        nRetVal = 0;
        return nRetVal;
}
 volatile int prevMode = 0; 
void formatbits( uint8_t pData, modbus_mapping_t* pModbusMap, GensetRegisters nIndex)
{
       pModbusMap->tab_input_registers[nIndex] = pData;
}
/////////////////////////////////////////////////////////////////////////////////

char *genSend(){ 

	int temp = 1, modbusTemp;  
	 
	 modbusTemp = getmodbus8bit(pModbusmap,LaunchPadMesGenSetStatus_ModbusIndex);
	 //printf( "modbustemp is %d \n", modbusTemp);
	 if(modbusTemp > 0) { 
		temp = modbusTemp;
	 }	else { 
		temp = prevMode; 
	 }	

	if((sysMode == 1) | (sysMode == 3)){ 
		temp = mode; 
	}
	
	if((temp > 0))
		prevMode = temp; 
	//printf("temp is %d \n", temp);
	//printf("sysmode is %d \n", sysMode);
	switch(temp){ 
		case 1: 
		case 0x55:
			return("18FF1610#55");
		case 2:
		case 0x65:
			return("18FF1610#65");
		case 3:
		case 0xA5:
			return("18FF1610#A5");	
		case 4:
		case 0x59:
			return("18FF1610#59");	
		case 5:
		case 0x69:
			return("18FF1610#69");
		case 6:
		case 0xA9:
			return("18FF1610#A9");
			//break; 	
		case 7:
		case 0x56:
			return("18FF1610#56");
			//break; 	
		case 8:
		case 0x66:
			return("18FF1610#66");
			//break; 	
		case 9:
		case 0xAA:			
			return("18FF1610#AA");
			//break; 
		case 11:
		case 12: 
		temp = prevMode; 
		default:
		temp = 1;
		 return "18FF1610#55";
	}
}

void dumpFlter(int id, long long data){ 
   
  uint8_t temp; double measure; int address = id >> 8;
  id = id & 0b11111111;
  
  printf("Generator ID %d \n", id);
 
 switch(address){ 

  case 0x18FF17:
    
    temp = (data >> 4) & 0b11; 
    printf("Genset Status: ");
    
    switch(temp) { 
      
    case 0: 
      printf("OFF + ");
      break;
    case 1: 
      printf("Prime and Run AUX fuel + ");
      break;
    case 2: 
      printf("Prime and Run + ");
      break;
    case 3: 
      printf("Start + ");
      break;
    }
   // temp = data >> 4;
    temp = data & 0b1111; 
    switch(temp){ 
      
    case 0: 
      printf("Not ready to crank \n");
      break;
    case 1: 
      printf("Ready to crank \n");
      break;
    case 2: 
      printf("Delay to crank, wtr kit \n");
      break;
    case 3: 
      printf("Delay to crank, glw plg \n");
      break;
    case 4:
      printf("Crank\n");			
      break;
    case 5: 
      printf("Running\n");
      break;
    case 6: 
      printf("Emergency Stop \n");
      break;
    case 7: 
      printf("Idle Mode \n");
      break;
    case 8: 
      printf("Powering down \n");
      break;
    case 9:
      printf("Factory test \n");
      break; 
    case 10:
      printf("Delay to crank, winter kit test \n");
      break;
    case 11: 
      printf("Delay to crank, winter kit detect \n");
      break;
    case 12: 
      printf("Delay to crank, ECM data save \n");
      break;
    case 13: 
      printf("Running, Synchronizing \n");
      break;
    case 14: 
      printf("Running, Synchronized\n");
      break;
    case 15: 
      printf("Running, load share \n");
      break;	
    }
    break;  
 
	printf("Unknown Encoding Value \n");
	break;  
    
  case 0x18FF1E:
    if(data == 0 )
      printf("Generator ON \n"); 
    else 
      printf("Generator ON \n"); 
    break;  
  case 0x18FF21:
	printf("Constant %d \n",(data & 0b1111111111111111)); 
	data = data >> 16; 
	printf("Reserved %d \n",(data & 0b1111111111111111)); 
	data = data >> 16;
	printf("Counter %d \n",(data & 0b11111111)); 
	data = data >> 8; 
	printf("ID %d \n",data); 
    break;
  case 0x18FF22:
	printf("Constant %d \n",(data & 0b1111111111111111)); 
	data = data >> 16; 
	printf("Reserved %d \n",(data & 0b1111111111111111)); 
	data = data >> 16;
	printf("Counter %d \n",(data & 0b11111111)); 
	data = data >> 8; 
	printf("ID %d \n",data); 

    break;
    
  case 0x18FF23:
  
    printf("Fuel Level %.2f \n", (data & 0b1111111111111111)/(float)10); //has to be a float/double since they want 0.1& for all three
    data = data >> 16; 
    printf(" Maintenacen Hours %.2f \n", (data & 0b1111111111111111)/(float)10);
    data = data >> 16;
    printf(" Run Hours %.2f \n", (data & 0b111111111111111111111111)/(float)10);
    break;  
    
  case 0x18FF24:
    
	if((data & 0b11111111) == 0xF8)	
		printf("CB close \n");
	else if((data & 0b11111111) == 0xF0)	
			printf("CB Open \n");

    printf("Active Warning %d ", (data & 0b1111111111111111)); 
    data = data >> 16; 
    printf("Active Fault %d ", (data & 0b1111111111111111)); 
    data = data >> 16;
    
    if((data & 0b1))
      printf("E-Stop fault ON ");	
    else
      printf("E-Stop fault OFF ");
    data = data >> 1;
    
    if((data & 0b1))
      printf("Battle short ON ");	
    else
      printf("Battle short OFF ");
    data = data >> 1;
    
    if((data & 0b1))
      printf("Dead bus local ON ");	
    else
      printf("Dead bus local OFF ");
    data = data >> 1;
    
    if((data & 0b1))
      printf("Genset CB Position ON ");	
    else
      printf("Genset CB Position OFF \n ");
    break;
    
  case 0x18FF25:

    printf("Config checksum %d \n",(data & 0b1111111111111111) ); 
    data = data >> 16; 
    
    printf("Load on generator. %d \n",  (data & 0b11111111));
    data = data >> 8;
    
	 //Generator Status
	 printf("Generator Status : "); 
    switch((data & 0b11111111)){ 
    case 0x55:
      printf("Not in auto mode \n"); 
      break; 
    case 0x45:
      printf("unknown \n"); 
      break; 
    case 0x25:
      printf("Ready/Running \n"); 
      break; 
    case 0x15:
      printf("Getting ready \n"); 
      break;
    case 0x05:
      printf("Idle \n"); 
      break;	
	default: 
	printf("Unknown Encoding Value %d\n", (data & 0b11111111) );
	break;	  
    }
    data = data >> 8;
     printf("Generator Load Status: "); 
    switch((data & 0b11111111)){ 
    case 0x21:
      printf("Overloaded  \n"); 
      break; 
    case 0x11:
      printf("unknown \n"); 
      break; 
    case 0x01:
      printf("Normal state\n"); 
      break; 	
	default: 
	printf("Unknown Encoding Value \n");
	break;	  
    }
    data = data >> 8;
    //unknown state always 0
    
    //(data & 0b11111111)
    data = data >> 8;
    
    if((data & 0b1111) == 0x0A)
      printf("ID Status Remediating conflicting generator ID \n"); 			
    else if((data & 0b1111) == 0x08)
      printf("ID Status Normal State \n"); 
    data = data >> 4;
    
    break;
    
  case 0x18FF26:
    printf("Stop Limit %d% ", (data & 0b1111111)); 
    data = data >> 7; 
    
    if((data & 0b1)) 
      printf("Auto type Run Time ");
    else 
      printf("Auto type Fixed priority ");
    data = data >> 1;
    
    printf("Start Limit %d%", (data & 0b1111111) );
    data = data >> 7; 
    
    //unknown
    data = data >> 1; 
    
    printf("Priority 1 GenID %d ", (data & 0b111)); 
    data = data >> 3; 
    printf("Priority 2 GenID %d ", (data & 0b111)); 
    data = data >> 3;
    printf("Priority 3 GenID %d ", (data & 0b111)); 
    data = data >> 3;
    printf("Priority 4 GenID %d ", (data & 0b111)); 
    data = data >> 3;
    printf("Priority 5 GenID %d ", (data & 0b111)); 
    data = data >> 3;
    printf("Priority 6 GenID %d ", (data & 0b111)); 
    data = data >> 3;
    
    printf("Start Limit Integer, kW %d ", (data & 0b111111111)); 
    data = data >> 9;
    printf("Stop Limit Integer, kW %d ", (data & 0b111111111)); 
    data = data >> 9;
    
    //unknown (data & 0b1)
    data = data >> 1;
    //unknown (data & 0b1)
    data = data >> 1; 
    
    switch ((data & 0b11)){ 
    case 0: 
      printf("Disable \n");
      break;
    case 1: 
      printf("Warm up \n");
      break;
    case 2: 
      printf("Enable battery \n");
      break;
    case 3: 
      printf(" Enable all \n");
      break;
      
    }
    data = data >> 2; 
    break;
    
  case 0x18FF27:
    
    printf("Run time delta %d Hours", (data & 0b11111111)); 
    data = data >> 8; 
    printf("Init delay %d ", (data & 0b11111111111)); 
    data = data >> 11; 
    printf("Start delay %d ", (data & 0b11111111111)); 
    data = data >> 11;
    printf("Stop delay %d ", (data & 0b11111111111)); 
    data = data >> 11;
    printf("Fail  delay %d \n", (data & 0b11111111111)); 
    data = data >> 11; 		
    
    break;
  default: 
    printf("Unknown PGN Address"); 
    break; 
  } 
  
}

int modbusWrite(int id, uint32_t nLength, long long data, uint8_t* pData, modbus_mapping_t* pModbusMap){ 
  // dont know why i put the size of, going to take them out and just & then shift
  int32_t nRetVal = -1;

      if (pData == NULL ||
                pModbusMap == NULL ||
                pModbusMap->tab_input_registers == NULL) // should check for an acceptable length
                return nRetVal;
				
  int temp; double measure;

int address = id >> 8;
  id = id & 0b11111111;
  switch(address){ 
  //0x18FF1720
  case 0x18FF17: //6 bits 

   pModbusMap->tab_input_registers[LaunchPadGensetStatus_ModbusIndex] = (data & 0b00001111);
   data = data >> 4;
   pModbusMap->tab_input_registers[LaunchPadGenControlSwitch_ModbusIndex] = (data & 0b0000011);
   data = data >> 2;
   
   //reserved 57 bits
   pModbusMap->tab_input_registers[LaunchPadReserved_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
   data = data >> 16;
   pModbusMap->tab_input_registers[LaunchPadReserved_Bytes3and2_ModbusIndex] = (data & 0b1111111111111111);
   data = data >> 16;
   pModbusMap->tab_input_registers[LaunchPadReserved_Bytes5and4_ModbusIndex] = (data & 0b1111111111111111);
   data = data >> 16;
   pModbusMap->tab_input_registers[LaunchPadReserved_Bytes7and6_ModbusIndex] = data & 0b0000000000000001;
    return 1;
	
  case 0x18FF1E: //6 bits
 
  pModbusMap->tab_input_registers[HeartbeatGenStatus_ModbusIndex] = (data & 0b00111111);
   data = data >> 6;
   //reserved 57 bits
   pModbusMap->tab_input_registers[HeartbeatReserved_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
   data = data >> 16;
   pModbusMap->tab_input_registers[HeartbeatReserved_Bytes3and2_ModbusIndex] = (data & 0b1111111111111111);
   data = data >> 16;
   pModbusMap->tab_input_registers[HeartbeatReserved_Bytes5and4_ModbusIndex] = (data & 0b1111111111111111);
   data = data >> 16;
   pModbusMap->tab_input_registers[HeartbeatReserved_Bytes7and6_ModbusIndex] = data & 0b0000000000000001;
  
	return 1;

  case 0x18FF21: //64 bits 
  //constant   
  format16bit(nLength, (pData  + (6* sizeof(uint8_t))), pModbusMap, AnotherGenOnConstant_Bytes1and0_ModbusIndex);
  //reserved
  format16bit(nLength, (pData  + (4* sizeof(uint8_t))), pModbusMap, AnotherGenOnReserved_Bytes1and0_ModbusIndex);
  //counter
  format8bit(nLength, (pData  + (3* sizeof(uint8_t))), pModbusMap, AnotherGenOnCounter_ModbusIndex);
  //id
  format16bit(nLength, (pData  + sizeof(uint8_t)), pModbusMap, AnotherGenOnID_Bytes1and0_ModbusIndex);
  format8bit(nLength, pData, pModbusMap, AnotherGenOnID_Bytes3and2_ModbusIndex);
  
return 1; 

  case 0x18FF22://64 bits 
	//constant   
  format16bit(nLength, (pData + (6* sizeof(uint8_t))), pModbusMap, GenOnConstant_Bytes1and0_ModbusIndex);
  //reserved
  format16bit(nLength, (pData + (4* sizeof(uint8_t))), pModbusMap, GenOnReserved_Bytes1and0_ModbusIndex);
  //counter
  format8bit(nLength, (pData + (3* sizeof(uint8_t))),  pModbusMap, GenOnCounter_ModbusIndex);
  //id
  format16bit(nLength, (pData + sizeof(uint8_t)), pModbusMap, GenOnID_Bytes1and0_ModbusIndex);
  format8bit(nLength, pData,  pModbusMap, GenOnID_Bytes3and2_ModbusIndex);
  
    return 1; 
	
  case 0x18FF23: //64 bits Maintenance info 
  //Fuel level
  
	pModbusMap->tab_input_registers[MaintenanceInfoFuelLevel_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
	data = data >> 16; 
	pModbusMap->tab_input_registers[MaintenanceInfoMtnenceHours_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
	data = data >> 16; 
	pModbusMap->tab_input_registers[MaintenanceInfoRunHours_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
	data = data >> 16; 
	pModbusMap->tab_input_registers[MaintenanceInfoRunHours_Bytes3and2_ModbusIndex] = (data & 0b11111111);
	data = data >> 8; 
	pModbusMap->tab_input_registers[MaintenanceInfoReserved_Bytes1and0_ModbusIndex] = (data & 0b1111111);

return 1;  

  case 0x18FF24: //64 bits

  //Active Warning
    pModbusMap->tab_input_registers[FltWarningActiveWarning_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
	data = data >> 16;
	//Active Fault
	pModbusMap->tab_input_registers[FltWarningActiveFault_Bytes1and0_ModbusIndex] = (data & 0b1111111111111111);
	data = data >> 16;
	
	//Save the first 4 bits of the 3rd byte
	//data = (pData + (3* sizeof(uint8_t)))[0] & 0b00001111; 
	//E-Stop fault
	formatbits((data & 0b00000001), pModbusMap, FltWarningActiveEstopFault_ModbusIndex);
	data = data >> 1;
	//Battle short
	formatbits((data & 0b00000001), pModbusMap, FltWarningActiveBattleShort_ModbusIndex);
	data = data >> 1;
	//Dead bus local
	formatbits((data & 0b00000001), pModbusMap, FltWarningActiveDeadBusLocal_ModbusIndex);
	data = data >> 1;
	//Genset CB
	formatbits((data & 0b00000001), pModbusMap, FltWarningActiveGensetCBPosition_ModbusIndex);
	data = data >> 1; 
	//reserved;
	pModbusMap->tab_input_registers[FltWarningActiveGensetReserved_Bytes1and0_ModbusIndex] = data & 0b1111111111111111;
//************************go over this part********************** 
	pModbusMap->tab_input_registers[FltWarningActiveGensetReserved_Bytes3and2_ModbusIndex] = data;
    return 1; 
 
 case 0x18FF25:
 
 //Config checksum 
 	pModbusMap->tab_input_registers[GenStatusConfigChecksum_Bytes1and0_ModbusIndex] = data & 0b1111111111111111;
data = data >> 16;
//Load on generator
	pModbusMap->tab_input_registers[GenStatusLoadOnGen_ModbusIndex] = data & 0b0000000011111111;
data = data >> 8;
//Generator Status
	pModbusMap->tab_input_registers[GenStatusGenStatus_ModbusIndex] = data & 0b0000000011111111;
data = data >> 8;
	  //Gen load status
  pModbusMap->tab_input_registers[GenStatusGenLoad_ModbusIndex] = data & 0b0000000011111111; 
data = data >> 8;
  //unknown  
  pModbusMap->tab_input_registers[GenStatusUnknown_ModbusIndex] = data & 0b0000000011111111;
  data = data >> 8;
//id status
 pModbusMap->tab_input_registers[GenStatusIDStatus_ModbusIndex] = data & 0b0000000000001111;
 data = data >> 4;
//reserved
    pModbusMap->tab_input_registers[FltWarningActiveGensetReserved_Bytes1and0_ModbusIndex] = data;


 case 0x18FF26:
   //Stop limit percentage
   formatbits((data & 0b01111111), pModbusMap, ConfigMesStopLimitPercent_ModbusIndex);
    data = data >> 7; 
    //Auto type
	formatbits((data & 0b00000001), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 1;

   //Start Limit percentage
    formatbits((data & 0b01111111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 7; 
    
    //unknown
	formatbits((data & 0b00000001), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 1; 
    
    //printf("Priority 1 GenID %d ", (data & 0b111)); 
	formatbits((data & 0b00000111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 3; 
    //printf("Priority 2 GenID %d ", (data & 0b111)); 
	formatbits((data & 0b00000111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 3;
    //printf("Priority 3 GenID %d ", (data & 0b111)); 
	formatbits((data & 0b00000111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 3;
    //printf("Priority 4 GenID %d ", (data & 0b111)); 
	formatbits((data & 0b00000111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 3;
    //printf("Priority 5 GenID %d ", (data & 0b111)); 
	formatbits((data & 0b00000111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 3;
    //printf("Priority 6 GenID %d ", (data & 0b111)); 
    data = data >> 3;
	formatbits((data & 0b00000111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    //printf("Start Limit Integer, kW %d ", (data & 0b111111111)); 
	 pModbusMap->tab_input_registers[ConfigMesStartLimit_Bytes1and0_ModbusIndex] = ((uint16_t)data & 0b0000000111111111);
	formatbits((data & 0b111111111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 9;
    //printf("Stop Limit Integer, kW %d ", (data & 0b111111111)); 
	formatbits((data & 0b111111111), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 9;
    //unknown (data & 0b1)
	formatbits((data & 0b00000001), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 1;
    //unknown (data & 0b1)
	formatbits((data & 0b00000001), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
    data = data >> 1; 
	//Start Assurance 
	formatbits((data & 0b00000011), pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
	data = data >> 2; 
	//Reserved 
	formatbits(data, pModbusMap, ConfigMesAutoType_ModbusIndex_ModbusIndex);
	
  return 1; 
  case 0x18FF27:
    
    //printf("Run time delta %d Hours", (data & 0b11111111)); 
	format8bit( nLength, (pData + (7* sizeof(uint8_t))), pModbusMap, TransmitRateRunTimeDelta_ModbusIndex);
  //init delay 11 bits
  	pModbusMap->tab_input_registers[TransmitInitDelay_Bytes1and0_ModbusIndex] = ((((uint16_t)pData[5]&0b00000111)<<8)| pData[6]);
  //start delay 11 bits 
	pModbusMap->tab_input_registers[TransmitStartDelay_Bytes1and0_ModbusIndex] = (((uint16_t)pData[4]&0b11111100)<<3) | ((pData[5]&0b11111000)>>3);
    //Stop delay 11bits 
    pModbusMap->tab_input_registers[TransmitStopDelay_Bytes1and0_ModbusIndex] = ((uint16_t)pData[3]<<2) | ((pData[4]&0b11000000)>>6);
    //Fail delay 11 bits 
	pModbusMap->tab_input_registers[TransmitStopFail_Bytes1and0_ModbusIndex] =  (((uint16_t)pData[3]&0b00000111)<< 8) | pData[2];
    //Reserved 11bits ((((((uint16_t)pData[0]&0x11100000)>>5)|0b00000000) << 8 ) | pData[1])
	pModbusMap->tab_input_registers[TransmitReserved_Bytes1and0_ModbusIndex] = (((uint16_t)pData[0]&0b01110000)<<1) | ((pData[1]&0b11111000)>> 3);
	
    break;
  default: 
    //printf("Unknown PGN Address"); 
    break; 
  } 
  
}

int socketinit(int *s, int currmax) {
	
  char *ptr, *nptr;
  int i, nbytes, numfilter, join_filter, rcvbuf_size = 0;
  can_err_mask_t err_mask;
  unsigned char hwtimestamp = 0;
  struct sockaddr_can addr; 
  struct ifreq ifr;
  struct can_filter *rfilter; 
    ptr = "can0";
  //loop through each can interface
  for (i=0; i < currmax; i++) {

    //ptr = argv[optind+i];
    nptr = strchr(ptr, ',');

    #ifdef DEBUG2
    printf("open %d '%s'.\n", i, ptr);
    #endif

    s[i] = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    //printf("SOCKET CREATED \n");
    if (s[i] < 0) {
      perror("socket");
      return 1;
    }

    cmdlinename[i] = ptr; /* save pointer to cmdline name of this socket */

    if (nptr)
      nbytes = nptr - ptr;  /* interface name is up the first ',' */
    else
      nbytes = strlen(ptr); /* no ',' found => no filter definitions */

    if (nbytes >= IFNAMSIZ) {
      fprintf(stderr, "name of CAN device '%s' is too long!\n", ptr);
      return 1;
    }

    if (nbytes > max_devname_len)
      max_devname_len = nbytes; /* for nice printing */

    addr.can_family = AF_CAN;

    memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    strncpy(ifr.ifr_name, ptr, nbytes);

    #ifdef DEBUG2
    printf("\n using interface name '%s'.\n", ifr.ifr_name);
    #endif

    if (strcmp(ANYDEV, ifr.ifr_name)) {
      if (ioctl(s[i], SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        exit(1);
      }
      addr.can_ifindex = ifr.ifr_ifindex;
    } else
      addr.can_ifindex = 0; /* any can interface */

    if (nptr) {

      /* found a ',' after the interface name => check for filters */

      /* determine number of filters to alloc the filter space */
      numfilter = 0;
      ptr = nptr;
      while (ptr) {
        numfilter++;
        ptr++; /* hop behind the ',' */
        ptr = strchr(ptr, ','); /* exit condition */
      }

      rfilter = malloc(sizeof(struct can_filter) * numfilter);
      if (!rfilter) {
        fprintf(stderr, "Failed to create filter space!\n");
        return 1;
      }

      numfilter = 0;
      err_mask = 0;
      join_filter = 0;

      while (nptr) {

        ptr = nptr+1; /* hop behind the ',' */
        nptr = strchr(ptr, ','); /* update exit condition */

        if (sscanf(ptr, "%x:%x",
                   &rfilter[numfilter].can_id,
                   &rfilter[numfilter].can_mask) == 2) {
          rfilter[numfilter].can_mask &= ~CAN_ERR_FLAG;
          numfilter++;
        } else if (sscanf(ptr, "%x~%x",
                          &rfilter[numfilter].can_id,
                          &rfilter[numfilter].can_mask) == 2) {
          rfilter[numfilter].can_id |= CAN_INV_FILTER;
          rfilter[numfilter].can_mask &= ~CAN_ERR_FLAG;
          numfilter++;
        } else if (*ptr == 'j' || *ptr == 'J') {
          join_filter = 1;
        } else if (sscanf(ptr, "#%x", &err_mask) != 1) {
          fprintf(stderr, "Error in filter option parsing: '%s'\n", ptr);
          return 1;
        }
      }

      if (err_mask)
        setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
                   &err_mask, sizeof(err_mask));

      free(rfilter);

    } /* if (nptr) */

    /* try to switch the socket into CAN FD mode */
    setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

    if (rcvbuf_size) {

      int curr_rcvbuf_size;
      socklen_t curr_rcvbuf_size_len = sizeof(curr_rcvbuf_size);

      /* try SO_RCVBUFFORCE first, if we run with CAP_NET_ADMIN */
      if (setsockopt(s[i], SOL_SOCKET, SO_RCVBUFFORCE,
                     &rcvbuf_size, sizeof(rcvbuf_size)) < 0) {
        #ifdef DEBUG
        printf("SO_RCVBUFFORCE failed so try SO_RCVBUF ...\n");
        #endif
        if (setsockopt(s[i], SOL_SOCKET, SO_RCVBUF,
                       &rcvbuf_size, sizeof(rcvbuf_size)) < 0) {
          perror("setsockopt SO_RCVBUF");
          return 1;
        }

        if (getsockopt(s[i], SOL_SOCKET, SO_RCVBUF,
                       &curr_rcvbuf_size, &curr_rcvbuf_size_len) < 0) {
          perror("getsockopt SO_RCVBUF");
          return 1;
        }

        /* Only print a warning the first time we detect the adjustment */
        /* n.b.: The wanted size is doubled in Linux in net/sore/sock.c */
        if (!i && curr_rcvbuf_size < rcvbuf_size*2)
          fprintf(stderr, "The socket receive buffer size was "
                  "adjusted due to /proc/sys/net/core/rmem_max.\n");
      }
    }

    if (hwtimestamp) {
      const int timestamping_flags = (SOF_TIMESTAMPING_SOFTWARE |\
                                      SOF_TIMESTAMPING_RX_SOFTWARE |\
                                      SOF_TIMESTAMPING_RAW_HARDWARE);

      if (setsockopt(s[i], SOL_SOCKET, SO_TIMESTAMPING,
                     &timestamping_flags, sizeof(timestamping_flags)) < 0) {
        perror("setsockopt SO_TIMESTAMPING is not supported by your Linux kernel");
        return 1;
      }
    } else {
      const int timestamp_on = 1;

      if (setsockopt(s[i], SOL_SOCKET, SO_TIMESTAMP,
                     &timestamp_on, sizeof(timestamp_on)) < 0) {
        perror("setsockopt SO_TIMESTAMP");
        return 1;
      }
    }

    const int dropmonitor_on = 1;

    if (setsockopt(s[i], SOL_SOCKET, SO_RXQ_OVFL,
                   &dropmonitor_on, sizeof(dropmonitor_on)) < 0) {
      perror("setsockopt SO_RXQ_OVFL not supported by your Linux Kernel");
      return 1;
    }

    if (bind(s[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("BIND ERROR:");
      return 1;
    }

  }

  return 0;
}

int canOut(FILE *lgFile, char *canMes, int *s){ 

fd_set rdfs;
	//make a list of can frames to send out instead of converting string to can frame to save space & memory 
	int required_mtu, mtu, nbytes;
	int enable_canfd = 1;
	struct sockaddr_can addr;
	struct canfd_frame frame;
	struct ifreq ifr;
	unsigned long msgsRecv = 0;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))]; 
	struct iovec iov; 
	struct cmsghdr *cmsg; 
	struct msghdr msg;	
	char buf[CL_CFSZ]; /* max length */
	iov.iov_base = &frame; 
	msg.msg_name = &addr; 
	msg.msg_iov = &iov; 
	msg.msg_iovlen = 1; 
	msg.msg_control = &ctrlmsg; 
	time_t timer;
	char buffer[26];
	struct tm* tm_info;

	required_mtu = parse_canframe(canMes, &frame);

	
	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_result);
	int w  = write(s[0], &frame, required_mtu);
	//sprint_canframe(buf, &frame, 0, maxdlen);
	//gettimeofday(&tval_before, NULL);
	if (w != required_mtu) {
		close(s[0]);
		
		perror("Can't send CAN message");
		printf("Exiting..\n ");
		//mode = 10; 
	return 1;
	} else { 

		time(&timer);
		tm_info = localtime(&timer);
		strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	
		fprintf(lgFile, "%s  can0  %s TX\n", buffer, canMes);
	}

	out_fflush:
		fflush(stdout);

	return 0; 

}

int idx2dindex(int ifidx, int socket) {

	int i;
	struct ifreq ifr;

	for (i=0; i < MAXIFNAMES; i++) {
		if (dindex[i] == ifidx)
			return i;
	}

	/* create new interface index cache entry */

	/* remove index cache zombies first */
	for (i=0; i < MAXIFNAMES; i++) {
		if (dindex[i]) {
			ifr.ifr_ifindex = dindex[i];
			if (ioctl(socket, SIOCGIFNAME, &ifr) < 0)
				dindex[i] = 0;
		}
	}

	for (i=0; i < MAXIFNAMES; i++)
		if (!dindex[i]) /* free entry */
			break;

	if (i == MAXIFNAMES) {
		fprintf(stderr, "Interface index cache only supports %d interfaces.\n",
		       MAXIFNAMES);
		exit(1);
	}

	dindex[i] = ifidx;

	ifr.ifr_ifindex = ifidx;
	if (ioctl(socket, SIOCGIFNAME, &ifr) < 0)
		perror("SIOCGIFNAME");

	if (max_devname_len < strlen(ifr.ifr_name))
		max_devname_len = strlen(ifr.ifr_name);

	strcpy(devname[i], ifr.ifr_name);

#ifdef DEBUG
	printf("new index %d (%s)\n", i, devname[i]);
#endif

	return i;
}

int candump(){

  fd_set rdfs;
  int s[MAXSOCK];
  int bridge = 0;
  useconds_t bridge_delay = 0;
  unsigned char timestamp = 0;
  unsigned char hwtimestamp = 0;
  unsigned char down_causes_exit = 1;
  unsigned char dropmonitor = 0;
  unsigned char silent = SILENT_INI;
  unsigned char view = 0;
  unsigned char log = 0;
  unsigned char logfrmt = 0;
  int count = 0;
  int rcvbuf_size = 0;
  int opt, ret, required_mtu, mtu;
  int currmax, numfilter;
  int join_filter;
  char *ptr, *nptr;
  struct sockaddr_can addr;
  char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3*sizeof(struct timespec) + sizeof(__u32))];
  struct iovec iov;
  struct msghdr msg;
  struct cmsghdr *cmsg;
  struct can_filter *rfilter;
  can_err_mask_t err_mask;
  struct canfd_frame frame;
  int nbytes, i, maxdlen;
  struct ifreq ifr;
  struct timeval tv, last_tv;
  struct timeval timeout, timeout_config = { 0, 0 }, *timeout_current = NULL;
  int enable_canfd = 1;
  
  FILE *logfile = NULL;
  
  //signal(SIGTERM, sigterm);
  //signal(SIGHUP, sigterm);
  //signal(SIGINT, sigterm);
  
  last_tv.tv_sec  = 0;
  last_tv.tv_usec = 0;
  int argc = 2; 
  log = 1;
  ptr = "can0";
  currmax = 1; 
  int errmode = 0; 
  int fileCount = 1; 
int currentCount= 1; 

  if(socketinit(s,currmax)!= 0){ 
	printf("Socket error \n"); 
	return 0; 
  } 
 
 /////check if any files exist if so grab the file of largest number
  
  #include <dirent.h>

	int file_count = 0;
	DIR * dirp;
	struct dirent * entry;
	int fileNumber = 0; 
    int z, j; 
	int zt = 0; 
	dirp = opendir("/home/debian/logs"); /* There should be error handling after this */
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_type == DT_REG) { /* If the entry is a regular file */
			
			 char *dot = strrchr(entry->d_name, '.');
			 if(dot && !strcmp(dot, ".log")){ //if a log file get the num
					//printf("%s\n", entry->d_name);
			 
				 for(z = 0; entry->d_name[z] != '.' ; z++ ){ 
				  //printf("%c \n", entry->d_name[z] );
					zt *= 10; 
					zt = zt + entry->d_name[z] - '0'; 
				 }
				 
				 if(fileNumber == 0){
					fileCount = zt;
				 }
				if(zt > fileCount){
					fileCount = zt ;
				}
				zt = 0;
				 //need to double check 
				 //printf("file number %d\n", fileNumber);
				 //printf("fileCount %d\n", fileCount);
				 fileNumber++;
				 file_count++; 
			 }
		}
	}
	
	closedir(dirp);
	  
 ////////////////////LOGFILE//////////////////////////////// 
    time_t currtime;
    struct tm now, start;
    //char fname[sizeof("/home/debian/logs/candump-2006-11-20_20:20:26.log")+1];
    char fname[sizeof("/home/debian/logs/9999999999.log")+1];
    
	if (time(&currtime) == (time_t)-1) {
      perror("time");
      return 1;
    }
    
    localtime_r(&currtime, &now);
    localtime_r(&currtime, &start);
	
	/*
    sprintf(fname, "/home/debian/logs/candump-%04d-%02d-%02d_%02d:%02d:%02d.log",
	    now.tm_year + 1900,
	    now.tm_mon + 1,
	    now.tm_mday,
	    now.tm_hour,
	    now.tm_min,
	    now.tm_sec);*/
		
		
		if(file_count!= 0 ){
		fileCount++;
		}		
	sprintf(fname, "/home/debian/logs/%d.log",
	    fileCount);
		
if((sysMode == 1) | (sysMode == 3)){   
    fprintf(stderr, "\nEnabling Logfile '%s'\n\n", fname);
}
    logfile = fopen(fname, "w");
    if (!logfile) {
      perror("logfile");
      return 1;
    }
/////////////////////////////////////////////////////////////////////

  /* these settings are static and can be held out of the hot path */
  iov.iov_base = &frame;
  msg.msg_name = &addr;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = &ctrlmsg;
  
  int init[8] = {0}; 
  int statusView = 1;  
  struct canfd_frame prevFrame; 
  int selectErr = 0; 
 // int resERR = 3; 
  int resErrtime = 5; 
	prevFrame.can_id = 0; prevFrame.len = 0; prevFrame.flags = 0; 
	prevFrame.__res0= 0; prevFrame.__res1 = 0;
	//  memcpy(prevFrame.data,0, 8 * sizeof(int)) ; 
 
  int bSendHeartbeat = 1;
  int bCmdReceived = 0;
  timeout_config.tv_sec = 1; //5
  timeout_config.tv_usec = 0;
  
  system("/home/debian/logs/deleteLogs.sh 1");
  
  while (running) {

	 if (time(&currtime) == (time_t)-1) {
      perror("time");
      return 1;
    }
    
	localtime_r(&currtime, &now);
	
	if((now.tm_hour - start.tm_hour) >= 4){ 	
	//if((now.tm_min - start.tm_min) >= 3){	
		//printf("%d \n", (now.tm_min - start.tm_min) );
		
		fclose(logfile);
		
		localtime_r(&currtime, &start);
		localtime_r(&currtime, &now);
	
		fileCount++;
		
		if(fileCount > 67295){ 
			fileCount = 1;
			system("/home/debian/logs/deleteLogs.sh 2");
		}
		sprintf(fname, "/home/debian/logs/%d.log",
	    fileCount);
		/*
		sprintf(fname, "/home/debian/logs/candump-%04d-%02d-%02d_%02d:%02d:%02d.log",
			now.tm_year + 1900,
			now.tm_mon + 1,
			now.tm_mday,
			now.tm_hour,
			now.tm_min,
			now.tm_sec);*/
			
		logfile = fopen(fname, "w");
		if (!logfile) {
		  perror("logfile");
		  return 1;
		}
		
		system("/home/debian/logs/deleteLogs.sh 1");
	 }
	  
    if(mode == 10){
      running = 0; 
		break; 
	}

	char buf[CL_CFSZ]; /* max length */struct tm tm;char timestring[25];

	if (bSendHeartbeat)
	{
		if(sysMode != 1)
			canOut(logfile, genSend(), s);

		bSendHeartbeat = 0;
	}

    FD_ZERO(&rdfs);
    
    for (i=0; i<currmax; i++)
      FD_SET(s[i], &rdfs);
    
	if(selectErr){ 

		fprintf(logfile,"No response from generator\n");

	    timeout_config.tv_sec = 1;
		timeout_config.tv_usec = 0;

		//reset canif for every timeout 

		if(sysMode != 1){	
			for (i=0; i<currmax; i++)
				close(s[i]);

			system("ifconfig can0 down");
			
			fprintf(logfile,"Time Out Error: CAN line might be disconnected or generator can interface is down\n"); 
			fprintf(logfile,"Resetting CAN interface on server\n");
			 
			system("ifconfig can0 up");
			
			if(socketinit(s,currmax)!= 0){ 
				printf("Socket error \n"); 
				return 0; 
			} 
		}
		selectErr = 0;
	}
    if(resErrtime < 0){ 
		if((sysMode == 1) | (sysMode == 3)){
			printf("\n No response from generator\n");
		}
		fprintf(logfile,"No response from generator\n");
	}
	
	ret = select(s[currmax-1]+1, &rdfs, NULL, NULL, &timeout_config);
    if (ret < 0) 
	{
	  selectErr = 1;
    //  printf("**********Select error %d***********\n", ret);
      continue;
    }
	else if (ret == 0)
	{   bSendHeartbeat = 1; //making sure we send out every second or when ff25 comes in 
		
		if (bCmdReceived)
		{
			bSendHeartbeat = 1;
			bCmdReceived = 0;
	        timeout_config.tv_sec = 1;
		    timeout_config.tv_usec = 0;
			continue;
		}
		selectErr = 1;
		continue;
	}
	else
	{
		bCmdReceived = 1;
	}

	selectErr = 0;
	
#ifdef DEBUG3
    printf("End select, running=%d, mode=%d\n", running, mode);
	printf("m %d sV %d \n",mode , statusView );
#endif 
    for (i=0; i<currmax; i++) {  /* check all CAN RAW sockets */
	

      if (FD_ISSET(s[i], &rdfs)) {
		int idx;
		
		/* these settings may be modified by recvmsg() */
		iov.iov_len = sizeof(frame);
		msg.msg_namelen = sizeof(addr);
		msg.msg_controllen = sizeof(ctrlmsg);  
		msg.msg_flags = 0; 
		
		//need to make sure there isnt a write conflict with the time
		//gettimeofday(&tval_after, NULL);
		//timersub(&tval_after, &tval_before, &tval_result);
		
		nbytes = recvmsg(s[i], &msg, 0);
		
		//gettimeofday(&tval_before, NULL);
		
		idx = idx2dindex(addr.can_ifindex, s[i]);

		if (nbytes < 0) {
		  if ((errno == ENETDOWN) && !down_causes_exit) {
			fprintf(stderr, "%s: interface down\n", devname[idx]);
			continue;
		  }
		  perror("read");
		  return 1;
		}
		
		if ((size_t)nbytes == CAN_MTU)
		  maxdlen = CAN_MAX_DLEN;
		else if ((size_t)nbytes == CANFD_MTU)
		  maxdlen = CANFD_MAX_DLEN;
		else {
		  fprintf(stderr, "read: incomplete CAN frame\n");
		  return 1;
		}
		
		if (count && (--count == 0))
		  running = 0;
		
		if (bridge) {
		  if (bridge_delay)
			usleep(bridge_delay);
		  
		  nbytes = write(bridge, &frame, nbytes);
		  if (nbytes < 0) {
			perror("bridge write");
			return 1;
		  } else if ((size_t)nbytes != CAN_MTU && (size_t)nbytes != CANFD_MTU) {
			fprintf(stderr,"bridge write: incomplete CAN frame\n");
			return 1;
		  }
		}
		
		for (cmsg = CMSG_FIRSTHDR(&msg);
			 cmsg && (cmsg->cmsg_level == SOL_SOCKET);
			 cmsg = CMSG_NXTHDR(&msg,cmsg)) {
		  if (cmsg->cmsg_type == SO_TIMESTAMP) {
			memcpy(&tv, CMSG_DATA(cmsg), sizeof(tv));
		  } else if (cmsg->cmsg_type == SO_TIMESTAMPING) {
			
			struct timespec *stamp = (struct timespec *)CMSG_DATA(cmsg);
			
			/*
			 * stamp[0] is the software timestamp
			 * stamp[1] is deprecated
			 * stamp[2] is the raw hardware timestamp
			 * See chapter 2.1.2 Receive timestamps in
			 * linux/Documentation/networking/timestamping.txt
			 */
			tv.tv_sec = stamp[2].tv_sec;
			tv.tv_usec = stamp[2].tv_nsec/1000;
		  } else if (cmsg->cmsg_type == SO_RXQ_OVFL)
			memcpy(&dropcnt[i], CMSG_DATA(cmsg), sizeof(__u32));
		}
		tm = *localtime(&tv.tv_sec);
		strftime(timestring, 24, "%Y-%m-%d %H:%M:%S", &tm);
		
		// need to find out why this doesnt work 
		if (dropcnt[i] != last_dropcnt[i]) {
		  
		  __u32 frames = dropcnt[i] - last_dropcnt[i];
		  
	
			printf("DROPCOUNT: dropped %d CAN frame%s on '%s' socket (total drops %d)\n",
			   frames, (frames > 1)?"s":"", devname[idx], dropcnt[i]);
		  
			fprintf(logfile, "DROPCOUNT: dropped %d CAN frame%s on '%s' socket (total drops %d)\n",
				frames, (frames > 1)?"s":"", devname[idx], dropcnt[i]);
		  
		  last_dropcnt[i] = dropcnt[i];
		}
		/////////////////////////////////////////////////////
		long long data = 0; int j = 0; bool pass = 0;
		
		if(frame.len == prevFrame.len){
			for(j=frame.len - 1; j > -1; j--){ 
				if(frame.data[j] != prevFrame.data[j]) {	
					pass = 1; 
				}
				data <<= 8; 
				data |= frame.data[j];
			}	
		}
		if(frame.len != prevFrame.len){ 
			for(j=frame.len - 1; j > -1; j--){ 
				data <<= 8; 
				data |= frame.data[j];
			}	
			pass = 1;
		}
			sprint_canframe(buf, &frame, 0, maxdlen);
		  if (!(msg.msg_flags & MSG_DONTROUTE)){ //recieved 
				resErrtime = 2; 
				
				/////////////////////////////////////////////////////////
				//Seth's HACK OF BADNESS PLEASE FIND A BETTER WAY
				if (((frame.can_id & 0b11111111111111111111111111111) >> 8) == 0x18ff25)
					bSendHeartbeat = 1;
				
			  	////////////////////////////////////////////////////////
				//Writing Recieved Can messages to Modbus
				
				errmode = modbusWrite((frame.can_id & 0b11111111111111111111111111111), frame.len, data, frame.data, pModbusmap);
				if(errmode < 0){
					//printf("Something is wrong exiting \n ");
					//mode = 10; 
					//running = 0;
					//break; 
				}
				
				///////////////////////////////////////////////////////////////// 
					if(frame.can_id != prevFrame.can_id){ 
						pass = 1; 
					}
					if(pass){
						
						if((sysMode == 1) | (sysMode == 3)){
							printf("\n\nGen Report Status\n\n");
							dumpFlter((frame.can_id & 0b11111111111111111111111111111), data);
						}
						prevFrame = frame;
					}			
			
				//printf("%s  %*s  %s RX\n", timestring,max_devname_len, devname[idx], buf);
				fprintf(logfile, "%s  %*s  %s RX\n", timestring,max_devname_len, devname[idx], buf);	
		  }
		  resErrtime--; 
		}
   out_fflush:
      fflush(stdout);
      
    }//END OF FOR 
    
  }//END OF WHILE
  
  for (i=0; i<currmax; i++)
    close(s[i]);
  
  if (bridge)
    close(bridge);

  if((sysMode == 1) | (sysMode == 3)){
	fprintf(stderr, "\nFinished writing to log file '%s'\n\n", fname);
  }
  
    fclose(logfile);
  //printf("****End of canIF***\n "); 
  return 0;
}
