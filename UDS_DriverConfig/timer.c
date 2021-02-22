/*
 * @ 名称: timer.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "timer.h"

//#define EN_LPTMR_INTERRUPT

volatile unsigned char g_ucIs10MsTimeout = FALSE;

/*init timer. 10ms timer*/
void InitTimer(void)
{
    LPTMR_DRV_Init(INST_LPTMR1, &lpTmr1_config0, false);

#ifdef EN_LPTMR_INTERRUPT
    /* Install IRQ handler for LPTMR interrupt */
    INT_SYS_InstallHandler(LPTMR0_IRQn, &RTITimerISR, (isr_t *)0);
    /* Enable IRQ for LPTMR */
    INT_SYS_EnableIRQ(LPTMR0_IRQn);
#endif  //end of EN_LPTMR_INTERRUPT

    /* Start LPTMR counter */
    LPTMR_DRV_StartCounter(INST_LPTMR1);
}

#ifdef DebugIo
volatile unsigned long g_ulTimer = 0u;
#endif

#ifndef EN_LPTMR_INTERRUPT
/*Is 1ms tick timeout? If 1ms tick timeout return TRUE, else return FALSE.*/
unsigned char Is1msTickTimeout(void)
{
    static unsigned char s_uc1msTickCnt = 0u;

    if (true == LPTMR_DRV_GetCompareFlag(INST_LPTMR1)) {
        /* Clear compare flag */
        LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);

        s_uc1msTickCnt++;

        if (s_uc1msTickCnt >= 10u) {
            s_uc1msTickCnt = 0u;

            g_ucIs10MsTimeout = TRUE;
        }

        return TRUE;
    }

    return FALSE;
}

/*Is 10ms tick timeout? If 10 tick timeout retrun TRUE, else return FALSE.*/
unsigned char Is10msTickTimeout(void)
{
    if (TRUE == g_ucIs10MsTimeout) {
        g_ucIs10MsTimeout = FALSE;

        return TRUE;
    }

    return FALSE;
}
#endif  //end of EN_LPTMR_INTERRUPT

#ifdef DebugIo
unsigned long GetTestTimer(void)
{
    return g_ulTimer;
}
#endif


#ifdef EN_LPTMR_INTERRUPT
/*RTI timer ISR*/
void RTITimerISR(void)
{
    static unsigned char s_ucTimeCnt = 0u;

    /* Clear compare flag */
    LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);

    s_ucTimeCnt++;

    if (s_ucTimeCnt >= 10) {
        g_ucIs10MsTimeout = TRUE;
        s_ucTimeCnt = 0u;
    }

}
#endif  //end of EN_LPTMR_INTERRUPT

/* -------------------------------------------- END OF FILE -------------------------------------------- */
