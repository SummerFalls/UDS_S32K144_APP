/*
 * @ 名称: printf_debug.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef PRINTF_DEBUG_H_
#define PRINTF_DEBUG_H_

#define Debug_Printf

/*define debug printf*/
#ifdef Debug_Printf
#include <uart.h>
#include <stdio.h>
#define DebugPrintf printf
#else
//#define DebugPrintf(fmt, args...)
#endif  //end of TOMLIN_DEBUG

/*define assert*/
#define EN_ASSERT
#ifdef EN_ASSERT
#define ASSERT(xValue)\
    do{\
        if(xValue)\
        {\
            while(1);\
        }\
    }while(0)

#define ASSERT_Printf(pString, xValue)\
    do{\
        if(FALSE != xValue)\
        {\
            DebugPrintf(pString);\
        }\
    }while(0)

#define ASSERT_DebugPrintf(pString, xValue)\
    do{\
        if(FALSE != xValue)\
        {\
            DebugPrintf((pString));\
            while(1u);\
        }\
    }while(0)
#else
#define ASSERT(xValue)
#define ASSERT_Printf(pString, xValue)
#define ASSERT_DebugPrintf(pString, xValue)
#endif

#endif /* PRINTF_DEBUG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
