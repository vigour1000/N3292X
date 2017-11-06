/***************************************************************************
 * Copyright (c) 2013 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     main.c
 * DESCRIPTION
 *     The main file for SIC/SD demo code.
 **************************************************************************/
 #include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

#include "w55fa92_reg.h"
#include "w55fa92_sic.h"
//#include "nvtfat.h"
#include "fmi.h"

/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/
// Define DBG_PRINTF to sysprintf to show more information about testing
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

#define OK      TRUE
#define FAIL    FALSE


/*-----------------------------------------------------------------------------
 * For global variables
 *---------------------------------------------------------------------------*/
#define SECTOR_SIZE         512
#define MAX_SECTOR_COUNT    512
#define BUF_SIZE        (SECTOR_SIZE * MAX_SECTOR_COUNT)
__align (32) UINT8 g_ram0[BUF_SIZE];
__align (32) UINT8 g_ram1[BUF_SIZE];

extern FMI_SD_INFO_T *pSD0, *pSD1, *pSD2; // define in SIC driver
FMI_SD_INFO_T *pSD;


/*-----------------------------------------------------------------------------
 * show data by hex format
 *---------------------------------------------------------------------------*/
void show_hex_data(unsigned char *ptr, unsigned int length)
{
    unsigned int line_len = 8;
    unsigned int i;

    for (i=0; i<length; i++)
    {
        if (i % line_len == 0)
            DBG_PRINTF("        ");
        DBG_PRINTF("0x%02x ", *(ptr+i));
        if (i % line_len == line_len-1)
            DBG_PRINTF("\n");
    }
    if (i % line_len != 0)
        DBG_PRINTF("\n");
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card insert
 *---------------------------------------------------------------------------*/
void isr_card_insert()
{
    UINT32 result;
    sysprintf("--- ISR: card inserted on SD port 0 ---\n\n");
    result = sicSdOpen0();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
        fmiSD_Show_info(0);
    }
    else if (result == FMI_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 0);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SD/MMC card %d, result = 0x%x !\n", 0, result);
    }
    return;
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card remove
 *---------------------------------------------------------------------------*/
void isr_card_remove()
{
    sysprintf("--- ISR: card removed on SD port 0 ---\n\n");
    sicSdClose0();
    return;
}


/*-----------------------------------------------------------------------------
 * To do test for SD card access. Write, read, and compare data on random sectors.
 *---------------------------------------------------------------------------*/
unsigned int sd_access_test(int sdport)
{
    UINT32 ii, sectorIndex;
    UINT32 result;
    UINT32 u32SecCnt;
    UINT8  *ptr_g_ram0, *ptr_g_ram1;
    DISK_DATA_T info;

    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);    // non-cache
    ptr_g_ram1 = (UINT8 *)((UINT32)g_ram1 | 0x80000000);    // non-cache

    for(ii=0; ii<BUF_SIZE; ii++)
    {
        ptr_g_ram0[ii] = rand() & 0xFF;
    }

    // get information about SD card
    switch (sdport)
    {
        case 0: fmiGet_SD_info(pSD0, &info);    break;
        case 1: fmiGet_SD_info(pSD1, &info);    break;
        case 2: fmiGet_SD_info(pSD2, &info);    break;
        default:
            sysprintf("ERROR: sd_access_test(): invalid SD port %d !!\n", sdport);
            return FAIL;
    }

    sectorIndex = rand() % info.totalSectorN;
    u32SecCnt   = rand() % MAX_SECTOR_COUNT;
    if (u32SecCnt == 0)
        u32SecCnt = 1;
    if (sectorIndex + u32SecCnt > info.totalSectorN)
        sectorIndex = info.totalSectorN - u32SecCnt;

    switch (sdport)
    {
        case 0: result = sicSdWrite0(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram0);   break;
        case 1: result = sicSdWrite1(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram0);   break;
        case 2: result = sicSdWrite2(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram0);   break;
    }
    DBG_PRINTF("    Write g_ram0 to SD card, result = 0x%x\n", result);
    show_hex_data(ptr_g_ram0, 16);

    memset(ptr_g_ram1, 0x5a, u32SecCnt * SECTOR_SIZE);
    switch (sdport)
    {
        case 0: result = sicSdRead0(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram1);    break;
        case 1: result = sicSdRead1(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram1);    break;
        case 2: result = sicSdRead2(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram1);    break;
    }
    DBG_PRINTF("    Read g_ram1 to SD card, result = 0x%x\n", result);
    show_hex_data(ptr_g_ram1, 16);

    if(memcmp(ptr_g_ram0, ptr_g_ram1, u32SecCnt * SECTOR_SIZE) == 0)
    {
        result = OK;
        sysprintf("    Data compare OK at sector %d, sector count = %d\n", sectorIndex, u32SecCnt);
    }
    else
    {
        result = FAIL;
        sysprintf("    ERROR: Data compare ERROR at sector %d, sector count = %d\n", sectorIndex, u32SecCnt);
    }
    return result;
}


/*-----------------------------------------------------------------------------
 * Initial UART0.
 *---------------------------------------------------------------------------*/
void init_UART()
{
    UINT32 u32ExtFreq;
    WB_UART_T uart;

    u32ExtFreq = sysGetExternalClock();
    sysUartPort(1);
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    uart.uart_no = WB_UART_1;
    sysInitializeUART(&uart);
}


/*-----------------------------------------------------------------------------
 * Initial Timer0 interrupt for system tick.
 *---------------------------------------------------------------------------*/
void init_timer()
{
    sysSetTimerReferenceClock(TIMER0, sysGetExternalClock());   // External Crystal
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);                  // 100 ticks/per sec ==> 1tick/10ms
    sysSetLocalInterrupt(ENABLE_IRQ);
}


/*-----------------------------------------------------------------------------*/

int main(void)
{
    int result, i, pass, fail;
    int target_port = 0;

    init_UART();
    init_timer();

    sysprintf("\n=====> W55FA92 Non-OS SIC/SD Driver Sample Code [tick %d] <=====\n", sysGetTicks(0));

    //sysprintf("REG_CLKDIV4 = 0x%08X\n", inp32(REG_CLKDIV4));    // default HCLK234 should be 0
    //outp32(REG_CLKDIV4, 0x00000310);                            // set HCLK234 to 1
    //sysprintf("REG_CLKDIV4 = 0x%08X\n", inp32(REG_CLKDIV4));

    srand(time(NULL));

    //--- Initial system clock
#ifdef OPT_FPGA_DEBUG
    sicIoctl(SIC_SET_CLOCK, 27000, 0, 0);   // clock from FPGA clock in
#else
    sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000, 0, 0);    // clock from PLL
#endif

    //--- Enable AHB clock for SIC/SD/NAND, interrupt ISR, DMA, and FMI engineer
    sicOpen();

    //--- Initial callback function for card detection interrupt
    //sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);

    //--- Initial SD card on port 0
    switch (target_port)
    {
        case 0: result = sicSdOpen0();   break;
        case 1: result = sicSdOpen1();   break;
        case 2: result = sicSdOpen2();   break;
    }
    if (result < 0)
    {
        sysprintf("ERROR: Open SD card on port %d fail. Return = 0x%x.\n", target_port, result);
        sicSdClose0();

        //--- Example code to enable/disable SD0 Card Detect feature.
        sysprintf("SD0 card detect feature is DISABLED by default.\n");
        sysprintf("The card status will always be INSERTED if card detect feature disabled.\n");
        sicIoctl(SIC_GET_CARD_STATUS, (INT32)&i, 0, 0);
        sysprintf("Check SD0 card status ..... is %s\n", i ? "INSERTED" : "REMOVED");

        sysprintf("Now, try to enable SD0 card detect feature ... Done!\n");
        sicIoctl(SIC_SET_CARD_DETECT, TRUE, 0, 0);  // MUST call sicIoctl() BEFORE sicSdOpen0()
        sicSdOpen0();

        i = FALSE;
        sysprintf("Wait SD0 card insert ... \n");
        while (i == FALSE)
        {
            sicIoctl(SIC_GET_CARD_STATUS, (INT32)&i, 0, 0);
        }
        sysprintf("Card inserted !!\n");
        sicSdClose0();

        sysprintf("Now, Try to disable SD0 card detect feature ... Done!\n");
        sicIoctl(SIC_SET_CARD_DETECT, FALSE, 0, 0); // MUST call sicIoctl() BEFORE sicSdOpen0()
        sicSdOpen0();
    }
    else
    {
        sysprintf("Detect SD card on port %d with %d sectors.\n", target_port, result);

        DBG_PRINTF("Do basic SD port %d access test ...\n", target_port);
        i = 0;
        pass = 0;
        fail = 0;
        while(i<10)
        {
            i++;
            DBG_PRINTF("=== Loop %d ...\n", i);
            result = sd_access_test(target_port);
            if (result == OK)
            {
                DBG_PRINTF("SD access test is SUCCESSFUL!!!\n");
                pass++;
            }
            else
            {
                DBG_PRINTF("SD access test is FAIL!!!\n");
                fail++;
            }
            DBG_PRINTF("=== Pass %d, Fail %d\n", pass, fail);
        }
        switch (target_port)
        {
            case 0: sicSdClose0();   break;
            case 1: sicSdClose1();   break;
            case 2: sicSdClose2();   break;
        }
    }

    sysprintf("\n===== THE END ===== [tick %d]\n", sysGetTicks(0));
    return OK;
}
