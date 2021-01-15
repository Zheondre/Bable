#ifndef _GENSET_REGS_H_
#define _GENSET_REGS_H_

typedef enum
{
   //NOTE:
   // The Genset has been defined to have 'ReadOnly' registers starting
   // at address 30001.  In the Modicon convention notation for Modbus registers
   // the 3XXXX address indicates ALL of these registers are input
   // registers.  Furthermore, the 3XXXX is implicit and the registers
   // are addressed starting with register 30001 @ offset 0.  The
   // enumeration here defines the offset into the holding regiser
   
   ReadOnlyRegsStart = 0,

   LaunchPadGensetStatus_ModbusIndex, //4bits 
   LaunchPadGenControlSwitch_ModbusIndex,//2bits
   LaunchPadGensetReserved_ModbusIndex, 
   LaunchPadReserved_Bytes7and6_ModbusIndex,
   LaunchPadReserved_Bytes5and4_ModbusIndex, 
   LaunchPadReserved_Bytes3and2_ModbusIndex,
   LaunchPadReserved_Bytes1and0_ModbusIndex,
   HeartbeatGenStatus_ModbusIndex,// 6 bits
   HeartbeatReserved_Bytes7and6_ModbusIndex,
   HeartbeatReserved_Bytes5and4_ModbusIndex, 
   HeartbeatReserved_Bytes3and2_ModbusIndex,
   HeartbeatReserved_Bytes1and0_ModbusIndex, 
   AnotherGenOnConstant_Bytes1and0_ModbusIndex,
   AnotherGenOnReserved_Bytes1and0_ModbusIndex,
   AnotherGenOnCounter_ModbusIndex,
   AnotherGenOnID_Bytes3and2_ModbusIndex,
   AnotherGenOnID_Bytes1and0_ModbusIndex,
   GenOnConstant_Bytes1and0_ModbusIndex,
   GenOnReserved_Bytes1and0_ModbusIndex,
   GenOnCounter_ModbusIndex,
   GenOnID_Bytes3and2_ModbusIndex,
   GenOnID_Bytes1and0_ModbusIndex,
   MaintenanceInfoFuelLevel_Bytes1and0_ModbusIndex,
   MaintenanceInfoMtnenceHours_Bytes1and0_ModbusIndex,
   MaintenanceInfoRunHours_Bytes3and2_ModbusIndex,
   MaintenanceInfoRunHours_Bytes1and0_ModbusIndex,
   MaintenanceInfoReserved_Bytes1and0_ModbusIndex,
   FltWarningActiveWarning_Bytes1and0_ModbusIndex,
   FltWarningActiveFault_Bytes1and0_ModbusIndex,
   FltWarningActiveEstopFault_ModbusIndex,
   FltWarningActiveBattleShort_ModbusIndex,
   FltWarningActiveDeadBusLocal_ModbusIndex,
   FltWarningActiveGensetCBPosition_ModbusIndex,
   FltWarningActiveGensetReserved_Bytes3and2_ModbusIndex,
   FltWarningActiveGensetReserved_Bytes1and0_ModbusIndex,
   GenStatusConfigChecksum_Bytes1and0_ModbusIndex,
   GenStatusLoadOnGen_ModbusIndex,
   GenStatusGenStatus_ModbusIndex,
   GenStatusGenLoad_ModbusIndex,
   GenStatusUnknown_ModbusIndex,
   GenStatusIDStatus_ModbusIndex,
   GenStatusIDReserved_Bits10and0_ModbusIndex,
   ConfigMesStopLimitPercent_ModbusIndex, //7bits
   ConfigMesAutoType_ModbusIndex_ModbusIndex, //1bit
   ConfigMesStartLimitPercent_ModbusIndex,//7bits
   ConfigMesUnknown_ModbusIndex,//1bit
   ConfigMesUnknownPriority1Gen_ModbusIndex, //3 bits 
   ConfigMesUnknownPriority2Gen_ModbusIndex, //3 bits 
   ConfigMesUnknownPriority3Gen_ModbusIndex, //3 bits 
   ConfigMesUnknownPriority4Gen_ModbusIndex, //3 bits 
   ConfigMesUnknownPriority5Gen_ModbusIndex, //3 bits 
   ConfigMesStartLimit_Bytes1and0_ModbusIndex, //9bits
   ConfigMesStopLimit_Bytes1and0_ModbusIndex, //9bits
   ConfigMesStartAssurance_ModbusIndex,
   ConfigMesReserved_Bytes1and0_ModbusIndex,
   TransmitRateRunTimeDelta_ModbusIndex,
   TransmitInitDelay_Bytes1and0_ModbusIndex,
   TransmitStartDelay_Bytes1and0_ModbusIndex,
   TransmitStopDelay_Bytes1and0_ModbusIndex,
   TransmitStopFail_Bytes1and0_ModbusIndex,
   TransmitReserved_Bytes1and0_ModbusIndex,

   ReadOnlyRegsEnd,
   NumRegs = ReadOnlyRegsEnd,
   /*
   ReadOnlyRegsEnd,
   ReadWriteRegsStart = ReadOnlyRegsEnd,
   ReadWriteRegsEnd = ReadWriteRegsStart,
   NumRegs = ReadWriteRegsEnd,*/
} GensetRegisters;

typedef enum
{
   //NOTE:
   // The Genset has been defined to have 'Read write ' registers starting
   // at address 40001.  In the Modicon convention notation for Modbus registers
   // the 4XXXX address indicates ALL of these registers are input/output
   // registers.  Furthermore, the 4XXXX is implicit and the registers
   // are addressed starting with register 40001 @ offset 0.  The
   // enumeration here defines the offset into the holding regiser
   // map.
   ReadWriteOnlyRegsStart_ModbusIndex = 0,
   
   LaunchPadMesGenSetStatus_ModbusIndex, //bits0 - 7
 
   ReadWriteOnlyRegsEnd,

   ReadWriteRegsStart = ReadWriteOnlyRegsStart_ModbusIndex,
   ReadWriteRegsEnd = ReadWriteOnlyRegsEnd,

   NumRWRegs = ReadWriteRegsEnd,

//hello
} GensetRegisters2;

#endif

