/*
 * @ 名称: fls_app.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "fls_app.h"
#include "uds_app.h"
#include "crc.h"
#include "flash.h"
#include "watch_dog.h"
#include "flash_cfg.h"
#include "Printf_debug.h"
//#include "com.h"

#define MAX_DATA_ITEM (16u)      /*max data item*/
#define MAX_DATA_BUF_LEN (128u)  /*max data buf len*/

typedef struct {
    unsigned long ulStartAddr;   /*start addr*/
    unsigned long ulDataLen;     /*data len*/
} tFlashDataInfo;

typedef struct {
    unsigned char ucNumberOfItem; /*number of item*/
    tFlashDataInfo astFlashDataItem[MAX_DATA_ITEM]; /*item flash data*/
    unsigned long xReceivedCrc;
} tFlashDataCheckInfo;

#define MAX_FLASH_DATA_LEN (200u)

/*flash software data*/
static unsigned char gs_aucFlashDataBuf[MAX_FLASH_DATA_LEN] = {0u};

/*flash data check sum info*/
static tFlashDataCheckInfo gs_stFlashDataCheckInfo;

/*flash download info*/
tFlsDownloadStateType g_stFlashDownloadInfo;

/*application flash status*/
tAppFlashStatus g_stAppFlashStatus;

/*program data size*/
static unsigned short gs_usProgramSize = 0u;

/*do operation response*/
static tpfResponse gs_pfResponse = NULL_PTR;

/*Is flash driver software data?*/
static unsigned char IsFlashDriverSoftwareData(void);

/*flash write */
static unsigned char FlashWrite(void);

/*flash check sum */
static unsigned char FlashChecksum(void);

/* Fash Erase*/
static unsigned char FlashErase(void);

/*save flash data buf*/
static unsigned char SavedFlashData(const unsigned char *i_pucDataBuf, const unsigned char i_ucDataLen);

/*read application informaiton from flash*/
static void ReadAppInfoFromFlash(void);

/*Is master MCU addr*/
static unsigned char IsMasterMCUAddr(const unsigned long i_ulDataAddr);


/*Init flash download*/
void InitFlashDowloadInfo(void)
{
    if (TRUE != g_stFlashDownloadInfo.ucIsFlashDrvDownloaded) {
        //g_stFlashDownloadInfo.ucErrorCode = g_pstFlashOptInfo->FLASH_EraseSector(APP_FLASH_INFO_ADDR, sizeof(tAppFlashStatus));
    }

    g_stFlashDownloadInfo.ucIsFingerPrintWritten = FALSE;

    g_stFlashDownloadInfo.ucIsFlashDrvDownloaded = FALSE;

    g_stFlashDownloadInfo.eDownloadStep = FL_REQUEST_STEP;

    g_stFlashDownloadInfo.eActiveJob = FALSH_IDLE;

    g_stFlashDownloadInfo.pstAppFlashStatus = &g_stAppFlashStatus;

    AppMemset(0x0u, sizeof(tFlashDataCheckInfo), &gs_stFlashDataCheckInfo);
    AppMemset(0xFFu, sizeof(tAppFlashStatus), &g_stAppFlashStatus);
}

/*flash operate main function*/
void FlashOperateMainFunction(void)
{
    switch (g_stFlashDownloadInfo.eActiveJob) {
        case FALSH_ERASING:
            /* do the flash erase*/
            g_stFlashDownloadInfo.ucErrorCode = FlashErase();
#ifdef EnSlaveBootloader

            if (STATUS_SUCCESS == g_stFlashDownloadInfo.ucErrorCode) {
                g_stFlashDownloadInfo.ucErrorCode = EraseSlaveFlash();
            }

#endif
            break;

        case FALSH_PROGRAMMING:

            if (TRUE == IsMasterMCUAddr(g_stFlashDownloadInfo.ulStartAddr)) {
                /* do the flash program*/
                g_stFlashDownloadInfo.ucErrorCode = FlashWrite();
            }

#ifdef EnSlaveBootloader
            else {
                //g_stFlashDownloadInfo.ulStartAddr -= SLAVE_APP_OFFSET_BASE;
                g_stFlashDownloadInfo.ucErrorCode = WriteSlaveFlash();
            }

#endif
            break;

        case FALSH_CHECKING:
            if (TRUE == IsMasterMCUAddr(gs_stFlashDataCheckInfo.astFlashDataItem[0].ulStartAddr)) {
                /* do the flash checksum*/
                g_stFlashDownloadInfo.ucErrorCode = FlashChecksum();
            }

#ifdef EnSlaveBootloader
            else {
                //g_stFlashDownloadInfo.ulStartAddr -= SLAVE_APP_OFFSET_BASE;
                g_stFlashDownloadInfo.ucErrorCode = CheckSlaveFlash();
            }

#endif
            break;

        default:
            break;
    }

    if ((g_stFlashDownloadInfo.ucErrorCode != STATUS_SUCCESS) &&
            ((FALSH_ERASING == g_stFlashDownloadInfo.eActiveJob) ||
             (FALSH_PROGRAMMING == g_stFlashDownloadInfo.eActiveJob) ||
             (FALSH_CHECKING == g_stFlashDownloadInfo.eActiveJob))) {
        /* initialize the flash download state */
        InitFlashDowloadInfo();
    }

    if (NULL_PTR != gs_pfResponse) {
        (*gs_pfResponse)(g_stFlashDownloadInfo.ucErrorCode);

        gs_pfResponse = NULL_PTR;
    }

    g_stFlashDownloadInfo.eActiveJob = FALSH_IDLE;

    return;
}

/*set wait do response*/
void SetWaitDoResponse(tpfResponse i_pfDoResponse)
{
    ASSERT(NULL_PTR == i_pfDoResponse);

    gs_pfResponse = i_pfDoResponse;
}

/*save will received data start address and data len*/
unsigned char SaveReceivedProgramInfo(const unsigned long i_ulStartAddr,
                                      const unsigned long i_ulDataLen)
{
    if (gs_stFlashDataCheckInfo.ucNumberOfItem >= MAX_DATA_ITEM) {
        return FALSE;
    }

    gs_stFlashDataCheckInfo.astFlashDataItem[gs_stFlashDataCheckInfo.ucNumberOfItem].ulStartAddr = (unsigned long)i_ulStartAddr;
    gs_stFlashDataCheckInfo.astFlashDataItem[gs_stFlashDataCheckInfo.ucNumberOfItem].ulDataLen = (unsigned long)i_ulDataLen;
    gs_stFlashDataCheckInfo.ucNumberOfItem++;

    return FALSE;
}

/*save flash data buf*/
static unsigned char SavedFlashData(const unsigned char *i_pucDataBuf, const unsigned char i_ucDataLen)
{
    ASSERT(NULL_PTR == i_pucDataBuf);

    if (i_ucDataLen > MAX_FLASH_DATA_LEN) {
        return FALSE;
    }

    AppMemcopy(i_pucDataBuf, i_ucDataLen, gs_aucFlashDataBuf);

    g_stFlashDownloadInfo.pucDataBuff = gs_aucFlashDataBuf;

    return TRUE;
}

/* Fash Erase*/
static unsigned char FlashErase(void)
{
    unsigned int failAddr = 0u;
    unsigned char ucIndex = 0u;
    unsigned char ucRet = STATUS_ERROR;
    unsigned long xCountCrc = 0l;

    //(void)g_pstFlashOptInfo->FLASH_EraseSector(&flashSSDConfig, APP_FLASH_INFO_ADDR, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);

    /* interrupt can break flash operation */
    INT_SYS_DisableIRQGlobal();
    /* Disable cache to ensure that all flash operations will take effect instantly,
       this is device dependent */
#ifdef S32K144_SERIES
    MSCM->OCMDR[0u] |= MSCM_OCMDR_OCM1(0xFu);
    MSCM->OCMDR[1u] |= MSCM_OCMDR_OCM1(0xFu);
    MSCM->OCMDR[2u] |= MSCM_OCMDR_OCM1(0xFu);
#endif /* S32K144_SERIES */

    while (ucIndex < g_ucBlockNum) {
        unsigned short Sector_Num = 0;
        unsigned short AllErase_Sector_Num = 0;
        unsigned char FirstErase_Sector_Num = 0;

        FirstErase_Sector_Num = g_astBlockNum[ucIndex].xBlockStartLogicalAddr / FEATURE_FLS_PF_BLOCK_SECTOR_SIZE;

        AllErase_Sector_Num = ((g_astBlockNum[ucIndex].xBlockEndLogicalAddr - \
                                g_astBlockNum[ucIndex].xBlockStartLogicalAddr)\
                               / FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);

        for (Sector_Num = 0; Sector_Num < AllErase_Sector_Num; Sector_Num++) {
            /*fed watchdog*/
            FedWatchDog();

            ucRet = g_pstFlashOptInfo->FLASH_EraseSector(&flashSSDConfig, ((FirstErase_Sector_Num + Sector_Num) * FEATURE_FLS_PF_BLOCK_SECTOR_SIZE), \
                                                         FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
#ifdef Debug_Printf

            if (STATUS_SUCCESS != ucRet) {
                DebugPrintf("Erase Sector line 245: %d is default\r\n", Sector_Num);
                //while(1);
            }

#endif  //end of Debug_Printf

            /* Verify the erase operation at margin level value of 1, user read */
            ucRet = g_pstFlashOptInfo->FLASH_VerifySection(&flashSSDConfig, ((FirstErase_Sector_Num + Sector_Num) * FEATURE_FLS_PF_BLOCK_SECTOR_SIZE), \
                                                           FEATURE_FLS_PF_BLOCK_SECTOR_SIZE / FTFx_DPHRASE_SIZE, 1u);
            //DEV_ASSERT(STATUS_SUCCESS == ret);
#ifdef Debug_Printf

            if (STATUS_SUCCESS != ucRet) {
                DebugPrintf("FLASH VerifySection 255: %d is default\r\n", Sector_Num);
                //while(1);
            }

#endif  //end of Debug_Printf
        }

        ucIndex++;
    }

    INT_SYS_EnableIRQGlobal();

    if (STATUS_SUCCESS == ucRet) {
        g_stAppFlashStatus.ucIsFlashErasedSuccessfull = TRUE;
        g_stAppFlashStatus.ucIsFlashProgramSuccessfull = FALSE;
        g_stAppFlashStatus.ucIsFlashStructValid = TRUE;
    } else {
        g_stAppFlashStatus.ucIsFlashErasedSuccessfull = FALSE;
        g_stAppFlashStatus.ucIsFlashProgramSuccessfull = FALSE;
        g_stAppFlashStatus.ucIsFlashStructValid = TRUE;

        InitFlashDowloadInfo();
    }

    /* I thank do not need CreatCrc */
    CreatCrc((unsigned char *)&g_stAppFlashStatus, sizeof(g_stAppFlashStatus) - 4u, &xCountCrc);

    g_stAppFlashStatus.ulCrc = xCountCrc;

    FedWatchDog();
#if 0
    /* interrupt can break flash operation */
    INT_SYS_DisableIRQGlobal();
    ucRet = STATUS_ERROR;
    ucRet = g_pstFlashOptInfo->FLASH_EraseSector(&flashSSDConfig, APP_FLASH_INFO_ADDR, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
    DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

    if (STATUS_SUCCESS != ucRet) {
        DebugPrintf("Erase Sector line 294 is default\r\n");
        //while(1);
    }

#endif  //end of Debug_Printf

    /* Verify the erase operation at margin level value of 1, user read */
    ucRet = g_pstFlashOptInfo->FLASH_VerifySection(&flashSSDConfig, APP_FLASH_INFO_ADDR, \
                                                   FEATURE_FLS_PF_BLOCK_SECTOR_SIZE / FTFx_DPHRASE_SIZE, 1u);
    //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

    if (STATUS_SUCCESS != ucRet) {
        DebugPrintf("FLASH VerifySection 312:  is default\r\n");
        //while(1);
    }

#endif  //end of Debug_Printf

    if (STATUS_SUCCESS == ucRet) {
        /*write data information in flash*/
        ucRet = g_pstFlashOptInfo->FLASH_Program(&flashSSDConfig, APP_FLASH_INFO_ADDR, sizeof(tAppFlashStatus), \
                                                 (unsigned char *)&g_stAppFlashStatus);
        //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

        if (STATUS_SUCCESS != ucRet) {
            DebugPrintf("FLASH Program 324: is default\r\n");
            //while(1);
        }

#endif  //end of Debug_Printf

        /* Verify the program operation at margin level value of 1, user margin */
        ucRet = g_pstFlashOptInfo->FLASH_ProgramCheck(&flashSSDConfig, APP_FLASH_INFO_ADDR, sizeof(tAppFlashStatus), \
                                                      (unsigned char *)&g_stAppFlashStatus, &failAddr, 1u);
        //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

        if (STATUS_SUCCESS != ucRet) {
            DebugPrintf("FLASH Program Check 336: is default\r\n");
            //while(1);
        }

#endif  //end of Debug_Printf
    }

    INT_SYS_EnableIRQGlobal();
#endif  //end of 0

    if ((STATUS_SUCCESS == ucRet)) {
        /*This is set RAM flash program successfull flag, bus don't write in flash. Only in check dependcy write
        RAM data in flash.*/
        //g_stAppFlashStatus.ucIsFlashProgramSuccessfull = TRUE;
    }

    return ucRet;
}


/*flash write */
static unsigned char FlashWrite(void)
{
    uint32_t    failAddr = 0u;
    unsigned char ucRet = STATUS_ERROR;
    unsigned long ulCountCrc = 0l;
    unsigned char ucFlashDataIndex = 0u;
    unsigned char ucFillCnt = 0u;

    ucRet = STATUS_SUCCESS;

    while (gs_usProgramSize >= PROGRAM_SIZE) {
#if 0
        /*read application flash informaiton*/
        ReadAppInfoFromFlash();
#endif  //end of 0

#ifdef UNUSE_CRC_CHKSUM
        /*count application flash crc*/
        CreatCrc((unsigned char *)&g_stAppFlashStatus, sizeof(g_stAppFlashStatus) - 4u, &ulCountCrc);

        if (ulCountCrc != g_stAppFlashStatus.ulCrc) {
            /*crc not right*/
            ucRet = STATUS_ERROR;

            break;
        }

#endif  //end of UNUSE_CRC_CHKSUM

        if ((TRUE == g_stAppFlashStatus.ucIsFlashErasedSuccessfull) &&
                (TRUE == g_stAppFlashStatus.ucIsFlashStructValid)) {
            FedWatchDog();
            /* interrupt can break flash operation */
            INT_SYS_DisableIRQGlobal();
            /*write data in flash*/
            ucRet = g_pstFlashOptInfo->FLASH_Program(&flashSSDConfig, g_stFlashDownloadInfo.ulStartAddr, PROGRAM_SIZE, \
                                                     &g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE]);
#ifdef Debug_Printf

            if (STATUS_SUCCESS != ucRet) {
                DebugPrintf("FLASH Program 387: %lx is default\r\n", g_stFlashDownloadInfo.ulStartAddr);
                //while(1);
            }

#endif  //end of Debug_Printf

            /* Verify the program operation at margin level value of 1, user margin */
            ucRet = g_pstFlashOptInfo->FLASH_ProgramCheck(&flashSSDConfig, g_stFlashDownloadInfo.ulStartAddr, PROGRAM_SIZE, \
                                                          &g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE], &failAddr, 1u);
            DEV_ASSERT(STATUS_SUCCESS == ucRet);
            INT_SYS_EnableIRQGlobal();

            if (STATUS_SUCCESS == ucRet) {
                g_stFlashDownloadInfo.ulLength -= PROGRAM_SIZE;
                gs_usProgramSize -= PROGRAM_SIZE;
                g_stFlashDownloadInfo.ulStartAddr += PROGRAM_SIZE;

                ucFlashDataIndex++;
            } else {
                ucRet = STATUS_ERROR;

                break;
            }
        } else {
            ucRet = STATUS_ERROR;

            break;
        }
    }

    /**/
    if ((0u != gs_usProgramSize) && (STATUS_SUCCESS == ucRet)) {
        ucFillCnt = (unsigned char)(gs_usProgramSize & 0x07u);
        ucFillCnt = (~ucFillCnt + 1u) & 0x0Fu;

        AppMemset(0xFFu, ucFillCnt,
                  (void *)&g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE + gs_usProgramSize]);

        gs_usProgramSize += ucFillCnt;

        /* interrupt can break flash operation */
        INT_SYS_DisableIRQGlobal();
        /*write data in flash*/
        ucRet = g_pstFlashOptInfo->FLASH_Program(&flashSSDConfig, g_stFlashDownloadInfo.ulStartAddr, gs_usProgramSize, \
                                                 &g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE]);
#ifdef Debug_Printf

        if (STATUS_SUCCESS != ucRet) {
            DebugPrintf("FLASH Program 436: %lx is default\r\n", g_stFlashDownloadInfo.ulStartAddr);
            //while(1);
        }

#endif  //end of Debug_Printf
        /* Verify the program operation at margin level value of 1, user margin */
        ucRet = g_pstFlashOptInfo->FLASH_ProgramCheck(&flashSSDConfig, g_stFlashDownloadInfo.ulStartAddr, gs_usProgramSize, \
                                                      &g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE], &failAddr, 1u);
        //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

        if (STATUS_SUCCESS != ucRet) {
            DebugPrintf("FLASH Program Check 454: %lx is default\r\n", g_stFlashDownloadInfo.ulStartAddr);
            //while(1);
        }

#endif  //end of Debug_Printf
        INT_SYS_EnableIRQGlobal();

        if (STATUS_SUCCESS == ucRet) {
            g_stFlashDownloadInfo.ulLength -= (gs_usProgramSize - ucFillCnt);
            gs_usProgramSize = 0;
            g_stFlashDownloadInfo.ulStartAddr += gs_usProgramSize;

            ucFlashDataIndex++;
        }

    }


    if (STATUS_SUCCESS == ucRet) {
        g_stAppFlashStatus.ucIsFlashProgramSuccessfull = TRUE;
#ifdef UNUSE_CRC_CHKSUM
        /* I thank do not need CreatCrc */
        CreatCrc((unsigned char *)&g_stAppFlashStatus, sizeof(g_stAppFlashStatus) - 4u, &ulCountCrc);

        g_stAppFlashStatus.ulCrc = ulCountCrc;
#endif  //end of UNUSE_CRC_CHKSUM

        return STATUS_SUCCESS;
    }

    InitFlashDowloadInfo();

    return STATUS_ERROR;
}

/*flash check sum */
static unsigned char FlashChecksum(void)
{
    unsigned char ucIndex = 0u;
    unsigned char aucDataBuf[MAX_DATA_BUF_LEN] = {0u};
    unsigned long xCountCrc = 0l;
    unsigned long ulReadDataLen = 0l;

    FedWatchDog();

    if ((TRUE == IsFlashDriverSoftwareData()) && (FALSE == g_stFlashDownloadInfo.ucIsFlashDrvDownloaded)) {
        CreatCrc((void *)g_stFlashDownloadInfo.ulStartAddr, g_stFlashDownloadInfo.ulLength, &xCountCrc);

        gs_stFlashDataCheckInfo.ucNumberOfItem--;

#ifdef DebugBootloader
        g_stFlashDownloadInfo.ucIsFlashDrvDownloaded = TRUE;

        InitFlashAPI();
        //Flash_test();
        return STATUS_SUCCESS;
#endif
    } else {
        /*fed watchdog*/
        FedWatchDog();

#ifdef MCU_USE_PAGING

        for (ucIndex = 0u; ucIndex < gs_stFlashDataCheckInfo.ucNumberOfItem; ucIndex++) {
            while (gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen) {
                /*If check sum startup address data, not check.*/
                if (STARTUP_ADDR == gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr) {
                    gs_stFlashDataCheckInfo.ucNumberOfItem = 0u;

                    return STATUS_SUCCESS;
                }

                /*fed watchdog*/
                FedWatchDog();

                if (gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen > MAX_DATA_BUF_LEN) {
                    /*read data from flash*/
                    ReadFlashMemory(gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr,
                                    MAX_DATA_BUF_LEN, aucDataBuf);

                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr += MAX_DATA_BUF_LEN;
                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen -= MAX_DATA_BUF_LEN;

                    ulReadDataLen = MAX_DATA_BUF_LEN;
                } else {
                    /*read data from flash*/
                    ReadFlashMemory(gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr, \
                                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen, aucDataBuf);

                    ulReadDataLen = gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen;
                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr += ulReadDataLen;
                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen = 0u;
                }
            }
        }

        gs_stFlashDataCheckInfo.ucNumberOfItem = 0u;

#else
        CreatCrc((uint8_t *)gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr, \
                 gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen, &xCountCrc);
#endif  //end of MCU_USE_PAGING
    }


#ifdef DebugBootloader
    return STATUS_SUCCESS;
#endif

    if (gs_stFlashDataCheckInfo.xReceivedCrc == xCountCrc) {
        if ((TRUE == IsFlashDriverSoftwareData()) && (FALSE == g_stFlashDownloadInfo.ucIsFlashDrvDownloaded)) {
            g_stFlashDownloadInfo.ucIsFlashDrvDownloaded = TRUE;

            InitFlashAPI();
            //Flash_test();

            /* now flash driver check crc successful, need init the flash api address */
            //InitFlashAPI();
        }

        return STATUS_SUCCESS;
    }

    AppMemset(0x0u, sizeof(tFlashDataCheckInfo), &gs_stFlashDataCheckInfo);

    InitFlashDowloadInfo();

    return STATUS_ERROR;
}

#ifdef EnSlaveBootloader
/*erase slave flash*/
static unsigned char EraseSlaveFlash(void)
{
    /*erase stm8 all flash memory*/
    return EraseSTM8Memory(0);
}

/*program slave flash*/
static unsigned char WriteSlaveFlash(void)
{
    unsigned char ucRet = STATUS_ERROR;
    unsigned long ulCountCrc = 0l;
    unsigned char ucFlashDataIndex = 0u;
    unsigned char ucFillCnt = 0u;

    ucRet = STATUS_SUCCESS;

    while (gs_usProgramSize >= PROGRAM_SIZE) {
        /*read application flash informaiton*/
        ReadAppInfoFromFlash();

        /*count application flash crc*/
        CreatCrc((unsigned char *)&g_stAppFlashStatus, sizeof(g_stAppFlashStatus) - 4u, &ulCountCrc);

        if (ulCountCrc != g_stAppFlashStatus.ulCrc) {
            /*crc not right*/
            ucRet = STATUS_ERROR;

            break;
        }

        if ((TRUE == g_stAppFlashStatus.ucIsFlashErasedSuccessfull) &&
                (TRUE == g_stAppFlashStatus.ucIsFlashStructValid)) {
            FedWatchDog();

            ucRet = WriteDataInSTM8Memory(g_stFlashDownloadInfo.ulStartAddr - SLAVE_APP_OFFSET_BASE,
                                          PROGRAM_SIZE,
                                          &g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE]);

            if (STATUS_SUCCESS == ucRet) {
                g_stFlashDownloadInfo.ulLength -= PROGRAM_SIZE;
                gs_usProgramSize -= PROGRAM_SIZE;
                g_stFlashDownloadInfo.ulStartAddr += PROGRAM_SIZE;

                ucFlashDataIndex++;
            } else {
                ucRet = STATUS_ERROR;

                break;
            }
        } else {
            ucRet = STATUS_ERROR;

            break;
        }
    }

    /**/
    if ((0u != gs_usProgramSize) && (STATUS_SUCCESS == ucRet)) {
        ucFillCnt = (unsigned char)(gs_usProgramSize & 0x07u);
        ucFillCnt = (~ucFillCnt + 1u) & 0x0Fu;

        AppMemset(0xFFu,
                  ucFillCnt,
                  (void *)&g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE + gs_usProgramSize]);

        gs_usProgramSize += ucFillCnt;

        /*write data in flash*/
        ucRet = WriteDataInSTM8Memory(g_stFlashDownloadInfo.ulStartAddr - SLAVE_APP_OFFSET_BASE,
                                      (unsigned char)gs_usProgramSize,
                                      &g_stFlashDownloadInfo.pucDataBuff[ucFlashDataIndex * PROGRAM_SIZE]);

        if (STATUS_SUCCESS == ucRet) {
            g_stFlashDownloadInfo.ulLength -= (gs_usProgramSize - ucFillCnt);
            gs_usProgramSize = 0;
            g_stFlashDownloadInfo.ulStartAddr += gs_usProgramSize;

            ucFlashDataIndex++;
        }

    }

    if (STATUS_SUCCESS == ucRet) {
        g_stAppFlashStatus.ucIsFlashProgramSuccessfull = TRUE;

        return STATUS_SUCCESS;
    }

    InitFlashDowloadInfo();

    return STATUS_ERROR;
}

/*check slave flash*/
static unsigned char CheckSlaveFlash(void)
{
    unsigned char ucIndex = 0u;
    unsigned char aucDataBuf[MAX_DATA_BUF_LEN] = {0u};
    unsigned long xCountCrc = 0l;
    unsigned long ulReadDataLen = 0l;

    FedWatchDog();

    return STATUS_SUCCESS;

    if ((TRUE == IsFlashDriverSoftwareData()) && (FALSE == g_stFlashDownloadInfo.ucIsFlashDrvDownloaded)) {
        CreatCrc((void *)g_stFlashDownloadInfo.ulStartAddr, g_stFlashDownloadInfo.ulLength, &xCountCrc);

        gs_stFlashDataCheckInfo.ucNumberOfItem--;

#ifdef DebugBootloader
        g_stFlashDownloadInfo.ucIsFlashDrvDownloaded = TRUE;

        return STATUS_SUCCESS;
#endif
    } else {
        for (ucIndex = 0u; ucIndex < gs_stFlashDataCheckInfo.ucNumberOfItem; ucIndex++) {
            while (gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen) {
                /*If check sum startup address data, not check.*/
                if (STARTUP_ADDR == gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr) {
                    gs_stFlashDataCheckInfo.ucNumberOfItem = 0u;

                    return STATUS_SUCCESS;
                }

                /*fed watchdog*/
                FedWatchDog();

                if (gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen > MAX_DATA_BUF_LEN) {
                    /*read data from flash*/
                    (void)ReadMemoryFromMCU(gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr - SLAVE_APP_OFFSET_BASE,
                                            MAX_DATA_BUF_LEN,
                                            aucDataBuf);

                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr += MAX_DATA_BUF_LEN;
                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen -= MAX_DATA_BUF_LEN;

                    ulReadDataLen = MAX_DATA_BUF_LEN;
                } else {
                    /*read data from flash*/
                    (void)ReadMemoryFromMCU(gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr - SLAVE_APP_OFFSET_BASE,
                                            (unsigned short)gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen,
                                            aucDataBuf);

                    ulReadDataLen = gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen;
                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulStartAddr += ulReadDataLen;
                    gs_stFlashDataCheckInfo.astFlashDataItem[ucIndex].ulDataLen = 0u;
                }

                CreatCrc(aucDataBuf, ulReadDataLen, &xCountCrc);
            }
        }

        gs_stFlashDataCheckInfo.ucNumberOfItem = 0u;
    }

    if (gs_stFlashDataCheckInfo.xReceivedCrc == xCountCrc) {
        if ((TRUE == IsFlashDriverSoftwareData()) && (FALSE == g_stFlashDownloadInfo.ucIsFlashDrvDownloaded)) {
            g_stFlashDownloadInfo.ucIsFlashDrvDownloaded = TRUE;
        }

        return STATUS_SUCCESS;
    }

    AppMemset(0x0u, sizeof(tFlashDataCheckInfo), &gs_stFlashDataCheckInfo);

    InitFlashDowloadInfo();

    return STATUS_ERROR;
}
#endif

/*save received check sum crc*/
void SavedReceivedCheckSumCrc(unsigned long i_ulReceivedCrc)
{
    gs_stFlashDataCheckInfo.xReceivedCrc = i_ulReceivedCrc;
}

/*Flash program region. Called by uds servive 0x36u*/
unsigned char FlashProgramRegion(const unsigned long i_ulAddr,
                                 const unsigned char *i_pucDataBuf,
                                 const unsigned long i_ulDataLen)
{
    unsigned char ucDataLen = (unsigned char)i_ulDataLen;
    unsigned char ucRet = TRUE;

    ASSERT(NULL_PTR == i_pucDataBuf);

    ucRet = TRUE;

    if (FL_TRANSFER_STEP != g_stFlashDownloadInfo.eDownloadStep) {
        ucRet = FALSE;
    }

    /*saved flash data*/
    if (TRUE != SavedFlashData(i_pucDataBuf, ucDataLen) && (TRUE == ucRet)) {
        ucRet = FALSE;
    }

    if (TRUE == ucRet) {
        if (FALSE == g_stFlashDownloadInfo.ucIsFlashDrvDownloaded) {
            /*dowload flash driver or down slave software data*/

            /*if flash driver, copy the data to RAM*/
            if (TRUE == IsFlashDriverSoftwareData()) {
                AppMemcopy((void *)i_pucDataBuf, ucDataLen, (void *)i_ulAddr);
            } else {
                /*if slave software data, tranmit the data slave mcu with SPI.*/
            }

            g_stFlashDownloadInfo.eActiveJob = FALSH_IDLE;
        } else {
            /*cann't write data in startup addr*/
            if (STARTUP_ADDR != i_ulAddr) {
                gs_usProgramSize = (unsigned short)i_ulDataLen;

                g_stFlashDownloadInfo.eActiveJob = FALSH_PROGRAMMING;
                g_stFlashDownloadInfo.ucErrorCode = STATUS_SUCCESS;
            }
        }
    }

    /*received erro data.*/
    if (TRUE != ucRet) {
        InitFlashDowloadInfo();
    }

    return ucRet;
}

/*Is master MCU addr*/
static unsigned char IsMasterMCUAddr(const unsigned long i_ulDataAddr)
{
#ifdef EnSlaveBootloader

    if (i_ulDataAddr < SLAVE_APP_OFFSET_BASE) {
        return TRUE;
    }

    return FALSE;
#else

    return TRUE;

#endif
}

/*Is flash driver software data?*/
static unsigned char IsFlashDriverSoftwareData(void)
{
    if ((g_stFlashDownloadInfo.ulStartAddr >= FLASH_DRIVER_START_ADDR) &&
            ((g_stFlashDownloadInfo.ulStartAddr + g_stFlashDownloadInfo.ulLength) <= FLASH_DRIVER_END_ADDR)) {
        return TRUE;
    }

    return FALSE;
}

/*read application informaiton from flash*/
static void ReadAppInfoFromFlash(void)
{
    ReadFlashMemory(APP_FLASH_INFO_ADDR,
                    sizeof(g_stAppFlashStatus),
                    (unsigned char *)&g_stAppFlashStatus);
}

/*Is read application inforemation from flash valid?*/
unsigned char IsReadAppInfoFromFlashValid(void)
{
    unsigned long xCrc = 0u;

    /*read application information from flash*/
    ReadAppInfoFromFlash();

    CreatCrc((unsigned char *)&g_stAppFlashStatus, sizeof(g_stAppFlashStatus) - 4u, &xCrc);

    if (xCrc == g_stAppFlashStatus.ulCrc) {
        return TRUE;
    }

    return FALSE;
}

/*Is application in flash valid? If valid return TRUE, else return FALSE.*/
unsigned char IsAppInFlashValid(void)
{
    if (((TRUE == g_stAppFlashStatus.ucIsFlashProgramSuccessfull) &&
            (TRUE == g_stAppFlashStatus.ucIsFlashErasedSuccessfull)) &&
            (TRUE == g_stAppFlashStatus.ucIsFlashStructValid)) {
        return TRUE;
    }

    return FALSE;
}

/*erase flash driver in RAM*/
void EraseFlashDriverInRAM(void)
{
    AppMemset(0x0u, FLASH_DRIVER_LEN, (void *)FLASH_DRIVER_START_ADDR);
}

/*save printfigner*/
void SavePrintfigner(const unsigned char *i_pucPrintfigner, const unsigned char i_ucPrintfinerLen)
{
    //unsigned char ucIndex = 0u;
    unsigned char ucPrintfignerLen = 0u;

    ASSERT(NULL_PTR == i_pucPrintfigner);

    if (i_ucPrintfinerLen > FL_FINGER_PRINT_LENGTH) {
        ucPrintfignerLen =  FL_FINGER_PRINT_LENGTH;
    } else {
        ucPrintfignerLen =  (unsigned char)i_ucPrintfinerLen;
    }

    AppMemcopy((const void *)i_pucPrintfigner, ucPrintfignerLen, (void *)g_stFlashDownloadInfo.pstAppFlashStatus->aucFingerPrint);
}

/*init flash bootloader*/
void InitFlashBootLoader(void)
{
    /* transfer SDK2.0 Flash driver FLASH_DRV_Init API to init flash */
    InitFlash();

#ifndef DebugBootloader
    pstFlashOptInfo->FLASH_Program = NULL_PTR;
    pstFlashOptInfo->FLASH_EraseSector = NULL_PTR;
#endif
}

/*write flash application information called by bootloader last step*/
unsigned char WriteFlashAppInfo(void)
{
    unsigned int  failAddr = 0u;
    unsigned char ucRet = STATUS_ERROR;

    g_stAppFlashStatus.ulCrc = 0u;
    CreatCrc((unsigned char *)&g_stAppFlashStatus, sizeof(g_stAppFlashStatus) - 4u, &g_stAppFlashStatus.ulCrc);

    ucRet = STATUS_SUCCESS;
    /* interrupt can break flash operation */
    INT_SYS_DisableIRQGlobal();
#if 0
    ucRet = g_pstFlashOptInfo->FLASH_EraseSector(&flashSSDConfig, APP_FLASH_INFO_ADDR, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
#ifdef Debug_Printf

    if (STATUS_SUCCESS != ucRet) {
        DebugPrintf("Erase Sector line 924: is default\r\n");
        //while(1);
    }

#endif  //end of Debug_Printf
    /* Verify the erase operation at margin level value of 1, user read */
    ucRet = g_pstFlashOptInfo->FLASH_VerifySection(&flashSSDConfig, APP_FLASH_INFO_ADDR, \
                                                   FEATURE_FLS_PF_BLOCK_SECTOR_SIZE / FTFx_DPHRASE_SIZE, 1u);
    //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

    if (STATUS_SUCCESS != ucRet) {
        DebugPrintf("FLASH VerifySection 949: is default\r\n");
        //while(1);
    }

#endif  //end of Debug_Printf
#endif //end of 0

    if (STATUS_SUCCESS == ucRet) {
        /*write data information in flash*/
        ucRet =  g_pstFlashOptInfo->FLASH_Program(&flashSSDConfig, APP_FLASH_INFO_ADDR, sizeof(tAppFlashStatus), \
                                                  (unsigned char *)&g_stAppFlashStatus);
        //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

        if (STATUS_SUCCESS != ucRet) {
            DebugPrintf("FLASH Program 980: is default\r\n");
            //while(1);
        }

#endif  //end of Debug_Printf
        /* Verify the program operation at margin level value of 1, user margin */
        ucRet = g_pstFlashOptInfo->FLASH_ProgramCheck(&flashSSDConfig, APP_FLASH_INFO_ADDR, sizeof(tAppFlashStatus), \
                                                      (unsigned char *)&g_stAppFlashStatus, (uint32_t *)&failAddr, 1u);
        //DEV_ASSERT(STATUS_SUCCESS == ucRet);
#ifdef Debug_Printf

        if (STATUS_SUCCESS != ucRet) {
            DebugPrintf("FLASH Program Check 991: is default\r\n");
            //while(1);
        }

#endif  //end of Debug_Printf
    }

    INT_SYS_EnableIRQGlobal();
    return ucRet;
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
