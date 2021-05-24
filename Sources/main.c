/* ###################################################################
**     Filename    : main.c
**     Processor   : S32K1xx
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.00
** @brief
**         Main module.
**         This module contains user's application code.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/
/* MODULE main */


/* Including necessary module. Cpu.h contains other modules needed for compiling.*/
#include "Cpu.h"

volatile int exit_code = 0;

/* User includes (#include below this line is not maintained by Processor Expert) */
#include "includes.h"
#include "watch_dog.h"
#include "boot.h"
#include "timer.h"
#include "uds_app.h"
#include "fls_app.h"
#include "can_driver.h"
#include "can_cfg.h"
#include "clock_cfg.h"
#include "can_tp.h"
#include "flash.h"
#include "port.h"
#include "crc.h"

#define APP_VERSION ("V0.1")


#define MAX_WAIT_TIME_MS (5000u)
unsigned short g_usMaxDelayUdsMsgTime = MAX_WAIT_TIME_MS;
unsigned char g_ucIsRxUdsMsg = FALSE;

#define DOWNLOAD_APP_MASK (0xA5u)
#define DONWLOAD_APP_ADDR (0x20006FF0u)

#define REQUEST_ENTER_BOOTLOADER_MASK (0x5Au)
#define REQUEST_ENTER_BOOTLOADER_ADDR (0x20006FF1u)

uint8_t *g_pDownloadAPPFlagAddr = (uint8_t *)DONWLOAD_APP_ADDR;
uint8_t *g_pRequestEnterBootloaderAddr = (uint8_t *)REQUEST_ENTER_BOOTLOADER_ADDR;

volatile uint8_t g_IsReqEnterBootloaderMode = FALSE;

void ReqEnterBootloaderMode(void)
{
    uint32_t crc = 0u;

    *g_pRequestEnterBootloaderAddr = REQUEST_ENTER_BOOTLOADER_MASK;

    CreatCrc((uint8_t *)DONWLOAD_APP_ADDR, 14, &crc);

    *(uint16_t *)(DONWLOAD_APP_ADDR + 14u) = crc;
}

/*!
  \brief The main function for the project.
  \details The startup initialization sequence is the following:
 * - startup asm routine
 * - main()
*/
int main(void)
{
    /* Write your local variable definition here */
    uint32_t keyValue = 0;
    uint32_t crc = 0u;
    uint16_t stroageCrc = 0u;
    uint8_t aTxFirstEnterAPPMsg[8u] = {0x02u, 0x51, 0x01};
    uint8_t aRxMsgReqEnterBootloader[8u] = {0x02, 0x10, 0x02};
    uint8_t aTxMsgReqEnterBootloader[8u] = {0x03u, 0x7Fu, 0x10, 0x02};
    uint8_t timer1msCnt = 0u;
#ifdef DebugIo
    unsigned char ucTimeCnt = 0u;
#endif

    /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
#ifdef PEX_RTOS_INIT
    PEX_RTOS_INIT();                   /* Initialization of the selected RTOS. Macro is defined by the RTOS component. */
#endif
    /*** End of Processor Expert internal initialization.                    ***/

    /* Write your code here */
    InitClock();

    InitPort();

#if 1
    InitWatchDog();
#endif

    InitCAN();

    InitTimer();

    g_usMaxDelayUdsMsgTime = MAX_WAIT_TIME_MS;

    //    printf("Hello world\n");
#ifdef Debug_Printf
    DebugPrintf("Hello world\n");
#endif  //end of Debug_Printf

    InitQueue();

    for (;;) {
        if (TRUE == Is1msTickTimeout()) {
            CanTpMainFun();
            timer1msCnt++;
        }

        if (timer1msCnt >= 250u) {
            timer1msCnt = 0u;

            TrigDebugIo();
        }

#if 0

        if (TRUE == g_IsReqEnterBootloaderMode) {
            CreatCrc((uint8_t *)DONWLOAD_APP_ADDR, 14, &crc);
            *g_pRequestEnterBootloaderAddr = REQUEST_ENTER_BOOTLOADER_MASK;

            *(uint16_t *)(DONWLOAD_APP_ADDR + 14u) = crc;

            g_IsReqEnterBootloaderMode = FALSE;
        }

#endif
        CreatCrc((uint8_t *)DONWLOAD_APP_ADDR, 14, &crc);
        stroageCrc = *(uint16_t *)(DONWLOAD_APP_ADDR + 14u);

        if (crc == stroageCrc) {
#if 0

            if (*g_pRequestEnterBootloaderAddr == REQUEST_ENTER_BOOTLOADER_MASK) {
                TransmiteCANMsg(TX_ID, 8, aTxMsgReqEnterBootloader);

                DebugPrintf("\n APP --> Request Enter bootloader mode!\n");

                SystemRest();
            }

#endif

            if (*g_pDownloadAPPFlagAddr == DOWNLOAD_APP_MASK) {
                TransmiteCANMsg(TX_ID, 8, aTxFirstEnterAPPMsg);

                DebugPrintf("\n First time in APP successful!\n");

                *g_pDownloadAPPFlagAddr = 0u;

                CreatCrc((uint8_t *)DONWLOAD_APP_ADDR, 14, &crc);

                *(uint16_t *)(DONWLOAD_APP_ADDR + 14u) = (uint16_t)crc;
            }
        }

        /*10ms timeout*/
        if (TRUE == Is10msTickTimeout()) {
            FedWatchDog();

            UDSMainFun();
        }

    } /* loop forever */

    /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;) {
    if(exit_code != 0) {
      break;
    }
  }
  return exit_code;
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the NXP S32K series of microcontrollers.
**
** ###################################################################
*/
