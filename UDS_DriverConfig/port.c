/*
 * @ ����: port.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "port.h"
#include "Cpu.h"

/*******************************************************************************
* @name   : InitPort
* @brief  :
* @param  : void
* @retval : void
*******************************************************************************/
void InitPort(void)
{
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
