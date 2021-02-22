/*
 * @ ����: clock_cfg.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "clock_cfg.h"
#include "Cpu.h"
#include "includes.h"

/*******************************************************************************
* @name   : InitClock
* @brief  :
* @param  : void
* @retval : void
*******************************************************************************/
void InitClock(void)
{
    /* Clock Device init and config */
    CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT, g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
    CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
