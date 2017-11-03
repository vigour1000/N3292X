#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "w55fa92_reg.h"
#include "wblib.h"
#include "w55fa92_vpe.h"
#include "VPE_Emulation.h"
#include "nvtfat.h"
#include "W55FA92_SIC.h"
#include "w55fa92_vpost.h"

#define DBG_PRINTF(...)

UINT32 u3210msFlag=0;
void TimerBase(void)
{
	u3210msFlag = u3210msFlag+1;
}
void isr_card_insert(void)
{
	/* Init Storage Interface Controller */
	sicIoctl(SIC_SET_CLOCK, 27000, 0, 0);	
	sicOpen();	
	if (sicSdOpen0()<=0)
	{
		sysprintf("Error in initializing SD card !! \n");						
		while(1);
	}			
	fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);
}
void isr_card_remove(void)
{
	sicClose();
}
void init(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	INT32 result;
	UINT32 u32Channel;
	UINT32 u32PllOutHz;
	/* Cache on */ 
	//sysEnableCache(CACHE_WRITE_BACK);
	
	/* Init UART */
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;	//use APB clock
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	uart.uart_no = WB_UART_1;
	sysInitializeUART(&uart);
	
	/* Init Timer */
	u32ExtFreq = sysGetExternalClock();	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);		/* 100 ticks/per sec ==> 1tick/10ms */	
	u32Channel = sysSetTimerEvent(TIMER0, 1, (PVOID)TimerBase);	/* 1 ticks=10ms call back */	
	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);			
	
	/* Init file system */
	fsInitFileSystem();

	/* Init Storage Interface Controller */
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);      // clock from PLL
    	//--- Enable AHB clock for SIC/SD/NAND, interrupt ISR, DMA, and FMI engineer
    	sicOpen();
	//--- Initial callback function for card detection interrupt
    	sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);
    	//--- Initial SD card on port 0
    	result = sicSdOpen0();
    	if (result < 0)
    	{        	
		sysprintf("    SD Card Error Code %d\n", result);
		exit(-1);
    	}
	sysprintf("Detect SD card on port 0 with %d sectors.\n", result);
	//fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);
	sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);	
}

BOOL bIsVPECompleteInt = FALSE; 
BOOL bIsVPEScatterGatherOnePieceInt = FALSE;
BOOL bIsVPEBlockInt = FALSE;
BOOL bIsVPEBlockErrorInt = FALSE;
BOOL bIsVPEDMAErrorInt = FALSE;
UINT32 u32CompletCount = 0;


void vpeCompleteCallback(void)
{
	sysprintf("I bit in InISR = %d\n", sysGetIBitState());
	u32CompletCount= u32CompletCount+1;
	bIsVPECompleteInt = TRUE;
}	
extern unsigned int _mmuSectionTable[];

void vpeMacroBlockCallback(void)
{
	bIsVPEBlockInt = TRUE;
}
void vpeMacroBlockErrorCallback(void)
{
	bIsVPEBlockErrorInt = TRUE;
}	
void vpeDmaErrorCallback(void)
{
	bIsVPEDMAErrorInt = TRUE;
}
void vpeInit(void)		
{
	PFN_VPE_CALLBACK OldVpeCallback;
	
	vpeOpen();	//Assigned VPE working clock to 48MHz. 
	vpeInstallCallback(VPE_INT_COMP,
						vpeCompleteCallback, 
						&OldVpeCallback);				
#if 0						
	vpeInstallCallback(VPE_INT_PAGE_FAULT,
						vpePageFaultCallback, 
						&OldVpeCallback);						
	vpeInstallCallback(VPE_INT_PAGE_MISS,
						vpePageMissCallback, 
						&OldVpeCallback);
#endif						
	/* For C&M and JPEG	*/			
	vpeInstallCallback(VPE_INT_MB_COMP,
						vpeMacroBlockCallback, 
						&OldVpeCallback);
				
	vpeInstallCallback(VPE_INT_MB_ERR,
						vpeMacroBlockErrorCallback, 
						&OldVpeCallback);										
	vpeInstallCallback(VPE_INT_DMA_ERR,
						vpeDmaErrorCallback, 
						&OldVpeCallback);
	vpeEnableInt(VPE_INT_COMP);				
#if 0
	vpeEnableInt(VPE_INT_PAGE_FAULT);	
	vpeEnableInt(VPE_INT_PAGE_MISS);	

	vpeEnableInt(VPE_INT_MB_COMP);	
	vpeEnableInt(VPE_INT_MB_ERR);
#endif			
	vpeEnableInt(VPE_INT_DMA_ERR);		
						
}	
	
void EnableCache(void)
{
	if (! sysGetCacheState ()) {
		sysInvalidCache ();
///		sysEnableCache (CACHE_WRITE_THROUGH);
		sysEnableCache (CACHE_WRITE_BACK);
		sysFlushCache (I_D_CACHE);
	}
}			
INT32 main(void)
{
	UINT32 u32Item;		
	EnableCache();		
	init();
	vpeInit();	
	do
	{    	
		sysprintf("================================================================\n");
		sysprintf("						VPE demo code												\n");
		sysprintf(" [1] NormalFormatConversionRotationDownscale_TV\n");
		sysprintf("================================================================\n");
		
				
		//u32Item = sysGetChar();
		u32Item = '1';
		
		switch(u32Item)
		{		
			case '1':	NormalFormatConversionRotationDownscale_TV();						//MMU
//					while(1);
					break;	
	
		}
	}while((u32Item!= 'q') || (u32Item!= 'Q'));											
	return Successful;
} 
