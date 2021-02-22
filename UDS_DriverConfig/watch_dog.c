/*
 * @ 名称: watch_dog.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "watch_dog.h"
#include "Cpu.h"

/*Init watch dog*/
void InitWatchDog(void)
{
    WDOG_DRV_Init(INST_WATCHDOG1, &watchdog1_Config0);
}

/*fed watch dog*/
void FedWatchDog(void)
{
    /* Reset Watchdog counter */
    WDOG_DRV_Trigger(INST_WATCHDOG1);
}

/*system reset*/
void SystemRest(void)
{
    WDOG_DRV_SetTimeout(INST_WATCHDOG1, 0u);

    while (1);
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
