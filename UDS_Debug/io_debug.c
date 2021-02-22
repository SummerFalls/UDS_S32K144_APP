/*
 * @ 名称: io_debug.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "io_debug.h"

#ifdef DebugIo

void SetDebugPinLow(void)
{
    PINS_DRV_WritePin(BUG_GPIO, BUGIO, 0u);
}

void SetDebugPinHigh(void)
{
    PINS_DRV_WritePin(BUG_GPIO, BUGIO, 1u);
}

void TriggerDebugPin(void)
{
    PINS_DRV_TogglePins(BUG_GPIO, 1 << BUGIO);
}

#endif  //end of DebugIo

/* -------------------------------------------- END OF FILE -------------------------------------------- */
