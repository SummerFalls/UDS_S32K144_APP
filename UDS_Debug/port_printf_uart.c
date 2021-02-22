/*
 * @ 名称: port_printf_uart.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "Cpu.h"
#include "uart.h"

UARTError InitializeUART(UARTBaudRate baudRate)
{

    LPUART_DRV_Init(INST_LPUART1, &lpuart1_State, &lpuart1_InitConfig0);

    return kUARTNoError;
}

UARTError ReadUARTN(void *bytes, unsigned long limit)
{
    UARTError err;

    /*
     * call LINFlexD to receive several byte within 1000 count delay
     */
    err = (UARTError)LPUART_DRV_ReceiveDataBlocking(INST_LPUART1, (uint8_t *)bytes, limit, OSIF_WAIT_FOREVER);

    return err;
}


UARTError ReadUARTPoll(char *c)
{
    UARTError err = kUARTNoError;

    /*
    * call LINFlexD to receive several bytes within 1000 count delay
    */
    err = (UARTError)LPUART_DRV_ReceiveDataBlocking(INST_LPUART1, (uint8_t *)c, 1, 0);

    return err;
}

UARTError WriteUARTN(const void *bytes, unsigned long length)
{
    UARTError err = kUARTNoError;

    err = (UARTError)LPUART_DRV_SendDataBlocking(INST_LPUART1, (uint8_t *)bytes, length, OSIF_WAIT_FOREVER);

    return err;
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
