/*
 * @ ����: watch_dog.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
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
