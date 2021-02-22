/*
 * @ 名称: boot.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef BOOT_H_
#define BOOT_H_

#include "includes.h"

typedef void(*JumpToPtr)(void);

extern JumpToPtr pJumpTo;

/** \brief  Jump to App code
 */
#if defined (__GNUC__)
#define JumpToApp()\
    do\
    {\
        pJumpTo = (JumpToPtr)(*((unsigned int *)0x0001401C));\
        pJumpTo();\
    }while(0);
#else
#define JumpToApp()\
    do\
    {\
        __asm("ldr  r0, 0x0001401C");\
        __asm("blx r0");\
    }while(0);
#endif

extern uint32_t __APP_UPDATE_ADDR[];
//#define APP_UPDATE_ADDR ((uint32_t*)__APP_UPDATE_ADDR)
#define APP_UPDATE_ADDR (0x20006FF0u)

extern uint32_t __BOOTLOADER_REQUEST_ADDR[];
//#define BOOTLOADER_REQUEST_ADDR ((uint32_t*)__BOOTLOADER_REQUEST_ADDR)
#define BOOTLOADER_REQUEST_ADDR (0x20006FF1u)

#define REQUEST_BOOTLOADER_MODE (0x5Au)
#define REQUEST_APP_MODE (0xA5u)

/*update application successfull*/
#define UpdateAppSuccessfull() *((unsigned char *)APP_UPDATE_ADDR) = REQUEST_APP_MODE

/*Is updata application successfull?*/
#define IsUpdataAppSuccessfull() ((REQUEST_APP_MODE == *((unsigned char *)APP_UPDATE_ADDR)) ? TRUE : FALSE)

/*clear update app code flag*/
#define ClearUpdateAppCode() *((unsigned char *)APP_UPDATE_ADDR) = 0u

/*update application successfull*/
#define UpdateBOOTLOADER_REQUEST() *((unsigned char *)BOOTLOADER_REQUEST_ADDR) = REQUEST_BOOTLOADER_MODE

/*Is requeset bootloader? If request bootloader reutrn 1, else return 0*/
#define IsRequestBootloader() ((REQUEST_BOOTLOADER_MODE == *((unsigned char *)BOOTLOADER_REQUEST_ADDR)) ? TRUE : FALSE)

/*clear request bootloader flag*/
#define ClearRequestBootloader() *((unsigned char *)BOOTLOADER_REQUEST_ADDR) = 0u

/*jump to app code address*/
#define APP_CODE_ADDR (0x0001401Cu)

/*jump to app code address*/
extern void JumpToAppCode(void);

/*Is can jump to app code? If need update app code reutrn TRUE, else return FALSE.*/
extern unsigned char IsCanJumpToAppCode(void);

/*Is app code updated?*/
extern unsigned char IsAppCodeUpdated(void);

#endif /* BOOT_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
