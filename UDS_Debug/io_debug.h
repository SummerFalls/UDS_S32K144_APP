/*
 * @ 名称: io_debug.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef IO_DEBUG_H_
#define IO_DEBUG_H_

#define DebugIo

#ifdef DebugIo
#include "cpu.h"
#include "timer.h"

#define BUGIO               15U /* pin PTD15 of LED RGB on DEV-KIT    */
#define BUG_GPIO            PTD /* LED GPIO type                      */

extern void SetDebugPinLow(void);
extern void SetDebugPinHigh(void);
extern void TriggerDebugPin(void);


#define TrigDebugIo() TriggerDebugPin()
#define SetDebugIoHigh() SetDebugPinHigh()
#define SetDebugIoLow() SetDebugPinLow()
#define GetSystemTimer() GetTestTimer()

#else
#define TrigDebugIo()
#define SetDebugIoHigh()
#define SetDebugIoLow()
#define GetSystemTimer() 0u

#endif //end of DebugIo

#endif /* IO_DEBUG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
