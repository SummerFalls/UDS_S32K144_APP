/*
 * @ 名称: includes.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef INCLUDES_H_
#define INCLUDES_H_

#include "Cpu.h"

#include "io_debug.h"
#include "printf_debug.h"

/* Define a string for adding my code */
#define NXF47391

/************************************************************
**  compile level.Use close some check.Theses marco reserved.
**  SAFE_LEVEL_O0        Highest
**  SAFE_LEVEL_O1
**  SAFE_LEVEL_O2
**  SAFE_LEVEL_O3        Lowest

**  SAFE_LEVEL_O0
**  SAFE_LEVEL_O1
**  SAFE_LEVEL_O2        Check input parameter over max or less than min?
**  SAFE_LEVEL_O3        Check pointer is safe?
************************************************************/
#define SAFE_LEVEL_O0    /**/
#define SAFE_LEVEL_O1    /**/
#define SAFE_LEVEL_O2    /*Check input parameter over max or less than min? */
#define SAFE_LEVEL_O3    /*Check pointer is safe?*/
/***********************************************************/

#ifndef TRUE
#define TRUE (1u)
#endif

#ifndef FALSE
#define FALSE (!TRUE)
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void *)0u)
#endif

#ifndef NULL
#define NULL ((void *)0u)
#endif



#define DebugBootloader

#endif /* INCLUDES_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
