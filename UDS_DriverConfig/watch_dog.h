/*
 * @ 名称: watch_dog.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef WATCH_DOG_H_
#define WATCH_DOG_H_

#include "includes.h"

/*Init watch dog*/
extern void InitWatchDog(void);

/*fed watch dog*/
extern void FedWatchDog(void);

/*system reset*/
extern void SystemRest(void);

#endif /* WATCH_DOG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
