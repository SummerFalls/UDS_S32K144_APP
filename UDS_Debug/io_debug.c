/*
 * @ ����: io_debug.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
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
