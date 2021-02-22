/*
 * @ 名称: port.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
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
