//canserver.c 
//need to support multiple cans 
// need to check if the network is up

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h> 
#include <syscall.h>

#include "modbus/genset_modbus.h"
#include "genset_regs.h"
#include "dumpcan.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

volatile sig_atomic_t flag = 0;
int sysMode; 
volatile int mode = 1; 
//extern volatile int running;
static modbus_mapping_t *pModbusmap; 


void *UI(){ 
	int temp;
	char sTemp[8];
	//int numTemp;
	//signal(SIGINT, my_function); 

	//******dont use scanf*********** 
#ifdef DEBUG
	printf("UIthread running %d\n", syscall(SYS_gettid));
#endif
int tv; 
	while(mode != 10){ 
//"\n\nMode\n 1	All Off/Stop \n 2	Run + CB Open \n 3	Run + CB Close \n 4	All Off/Stop + Battleshort \n 5	Run + CB Open + Battleshort \n 6	Run + CB Close + Battleshort \n 7	All Off/Stop + E-stop  \n 8	Run + CB Open + E-stop\n 9	Run + CB Close + E-stop\n 10	End Program\n 11	View Generator Status\n 12	Hide Generator Status\n 13	Send Via Modbus \n\n"
		printf( 
		"\n\nMode\n 1	All Off/Stop \n 2	Run + CB Open \n 3	Run + CB Close \n 4	All Off/Stop + Battleshort \n 5	Run + CB Open + Battleshort \n 6	Run + CB Close + Battleshort \n 7	All Off/Stop + E-stop  \n 8	Run + CB Open + E-stop\n 9	Run + CB Close + E-stop\n 10	End Program\n 11	View Generator Status\n 12	Hide Generator Status\n\n"
		);
	
		printf("\n Current mode is: %d\n Please choose a mode or enter 10 to exit: ", mode); 
		if(flag){ 
			flag = 0; 
			mode = 10; 
		} else {
			
			if(scanf("%s", &sTemp) != 1 ){ 
				printf("Bad input try again");
			} else {

				temp = atoi(sTemp);
				
				if(temp) { 
					if((temp < 1) || (temp > 13))
						printf("\n Choose a value between 1 & 12. \n");
					else 
						mode = temp; 
				} else{
					//mode = 10; 
					printf("\n Numbers only, try again. \n");
				}
			}
			fflush(stdin);
		}
	}
	running = 0;
	return NULL;
}

void* canIf(){
#ifdef DEBUG
	printf("CanIF thread running %d\n",  syscall(SYS_gettid));
#endif
	candump();
	
	if(mode == 10)
		exit(1);
}

int main(int argc, char *argv[]){ 

	if(argc == 2){ 
		if((strcmp("snoop\n",argv[1]) == 0) || (strcmp("snoop", argv[1]) == 0) ){ 
			// UI ON & heart beat message OFF
			//printf("Snoop mode\n");
			sysMode = 1;
		} else if((strcmp("noui\n",argv[1]) == 0 )|| (strcmp("noui", argv[1]) == 0)) { 
			// UI OFF & heart beat message ON 
			sysMode = 2; 
			//printf("noui mode %d\n",sysMode );
		} else {
			//Invalid command should we say so ? 
			// UI ON & heart beat message ON
			sysMode = 3; 
			//printf("default mode \n");
		}
	} else { 
		// UI off & heart beat message ON
		sysMode = 2; 	
	}
	

	modbus_mapping_t* pMap;
	
    uint16_t* pGensetRegs;
    uint16_t* pRWGensetRegs;
	
	pthread_t canIfid;
	pthread_t UIid;

	setbuf(stdout, NULL);
	
	pGensetRegs = (uint16_t*)malloc(NumRegs * sizeof(uint16_t));
	pRWGensetRegs = (uint16_t*)malloc(NumRWRegs * sizeof(uint16_t));
	
	if (pGensetRegs == NULL || pRWGensetRegs == NULL)
		return 1;

	pMap = genset_modbus_mapping_new(NULL, 0, NULL, 0, pRWGensetRegs, NumRWRegs, pGensetRegs, NumRegs);

	
	if (pMap == NULL)
	{
		free (pGensetRegs);
		free (pRWGensetRegs);
		return 2;
	}
	
	genset_modbus_mapping_init(pMap);
	pointToMap(pMap); 

	pthread_create(&canIfid, NULL, canIf, NULL); 
	
	if((sysMode == 1) | (sysMode == 3)){
		pthread_create(&UIid, NULL, UI, NULL);
	}
	
	genset_modbus_server_start(pMap);
	
#ifdef DEBUG
	printf("\n\n Modbus Server done \n\n");
#endif
	
	pthread_join(canIfid, NULL);
	
#ifdef DEBUG
	printf("Canid done \n");
#endif

	if((sysMode == 1) | (sysMode == 3)){
		pthread_join(UIid, NULL);
#ifdef DEBUG
	printf("UIid done \n");
#endif
	}
}



