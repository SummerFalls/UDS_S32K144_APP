/*
 * @ 名称: timer.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "includes.h"
#include "Cpu.h"

#ifdef DebugIo
extern unsigned long GetTestTimer(void);
#endif

/*init timer. 10ms timer*/
extern void InitTimer(void);

#ifndef EN_LPTMR_INTERRUPT
/*Is 1ms tick timeout? If 1ms tick timeout return TRUE, else return FALSE.*/
extern unsigned char Is1msTickTimeout(void);

/*Is 10ms tick timeout? If 10 tick timeout retrun TRUE, else return FALSE.*/
extern unsigned char Is10msTickTimeout(void);
#endif  //end of EN_LPTMR_INTERRUPT

#endif /* TIMER_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
