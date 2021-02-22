/*
 * @ 名称: fls_app.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef FLS_APP_H_
#define FLS_APP_H_

#include "includes.h"

typedef enum {
    FALSH_IDLE,           /*flash idle*/
    FALSH_ERASING,        /*erase flash */
    FALSH_PROGRAMMING,    /*program flash*/
    FALSH_CHECKING,       /*check flash*/
} tFlshJobModle;

typedef enum {
    FLASH_OPERATE_ERRO,  /*flash operate erro*/
    FLASH_OPERATE_RIGHT  /*flash operate right*/
} tFlashErroCode;

/** flashloader download step */
typedef enum {
    FL_REQUEST_STEP,      /*flash request step*/
    FL_TRANSFER_STEP,     /*flash transfer data step*/
    FL_EXIT_TRANSFER_STEP,/*exit transfter data step*/
    FL_CHECKSUM_STEP      /*check sum step*/

} tFlDownloadStepType;

#define FL_FINGER_PRINT_LENGTH  (17u) /*Flash finger print length*/

/*input parameter : TRUE/FALSE. TRUE = operation successfull, else failled.*/
typedef void (*tpfResponse)(unsigned char);

/* sizeof(tAppFlashStatus) % 8 must = 0 */
typedef struct {
    /*flash programming successfull? If programming successfull, the value set TRUE, else set FALSE*/
    unsigned char ucIsFlashProgramSuccessfull;

    /*Is erase flash successfull? If erased flash successfull, set the TRUE, else set the FALSE.*/
    unsigned char ucIsFlashErasedSuccessfull;

    /*Is Flash struct data valid? If writen set the value is TRUE, else set the valid FALSE*/
    unsigned char ucIsFlashStructValid;

    /* flag if fingerprint buffer */
    unsigned char aucFingerPrint[FL_FINGER_PRINT_LENGTH];

    /*count CRC*/
    unsigned long ulCrc;
} tAppFlashStatus;

/*application flash status*/
extern tAppFlashStatus g_stAppFlashStatus;

typedef struct {
    /* flag if fingerprint has written */
    unsigned char ucIsFingerPrintWritten;

    /* flag if flash driver has downloaded */
    unsigned char ucIsFlashDrvDownloaded;

    /* error code for flash active job */
    unsigned char ucErrorCode;

    /* current procees start address */
    unsigned long ulStartAddr;

    /* current procees length */
    unsigned long ulLength;

    /* current procees buffer point,point to buffer supplied from DCM */
    const unsigned char *pucDataBuff;

    /* flashloader download step */
    tFlDownloadStepType eDownloadStep;

    /* current job status */
    tFlshJobModle eActiveJob;

    /*point app flash status*/
    tAppFlashStatus *pstAppFlashStatus;
} tFlsDownloadStateType;

/*flash download info*/
extern tFlsDownloadStateType g_stFlashDownloadInfo;

/*Init flash download*/
extern void InitFlashDowloadInfo(void);

/*flash operate main function*/
extern void FlashOperateMainFunction(void);

/*save will received data start address and data len*/
extern unsigned char SaveReceivedProgramInfo(const unsigned long i_ulStartAddr,
                                             const unsigned long i_ulDataLen);

/*Flash program region. Called by uds servive 0x36u*/
extern unsigned char FlashProgramRegion(const unsigned long i_ulAddr,
                                        const unsigned char *i_pucDataBuf,
                                        const unsigned long i_ulDataLen);

/*Is read application inforemation from flash valid?*/
extern unsigned char IsReadAppInfoFromFlashValid(void);

/*Is application in flash valid? If valid return TRUE, else return FALSE.*/
extern unsigned char IsAppInFlashValid(void);

/*save received check sum crc*/
extern void SavedReceivedCheckSumCrc(unsigned long i_ulReceivedCrc);

/*erase flash driver in RAM*/
extern void EraseFlashDriverInRAM(void);

/*set wait do response*/
extern void SetWaitDoResponse(tpfResponse i_pfDoResponse);

/*save printfigner*/
extern void SavePrintfigner(const unsigned char *i_pucPrintfigner, const unsigned char i_ucPrintfinerLen);

/*init flash bootloader*/
extern void InitFlashBootLoader(void);

/*write flash application information called by bootloader last step*/
extern unsigned char WriteFlashAppInfo(void);

#endif /* FLS_APP_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
