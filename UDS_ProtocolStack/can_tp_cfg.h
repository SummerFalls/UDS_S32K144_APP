/*
 * @ 名称: can_tp_cfg.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef CAN_TP_CFG_H_
#define CAN_TP_CFG_H_

#include "includes.h"

/*******************************************************
**  Description : ISO 15765-2 config file
*******************************************************/

typedef unsigned int tUdsId;
typedef unsigned int tUdsLen;
typedef unsigned short tNetTime;
typedef unsigned short tBlockSize;
typedef unsigned char (*tNetTx)(const tUdsId, const unsigned short, const unsigned char *);
typedef unsigned char (*tNetRx)(tUdsId *, unsigned char *, unsigned char *);
typedef unsigned int tCanTpDataLen;

#define MAX_CF_DATA_LEN (150u) /*max first frame data len */



typedef struct {
    unsigned char ucCalledPeriod;/*called CAN tp main function period*/
    tUdsId xRxFunId;             /*rx function ID*/
    tUdsId xRxPhyId;             /*Rx phy ID*/
    tUdsId xTxId;                /*Tx ID*/
    tBlockSize xBlockSize;       /*BS*/
    tNetTime xSTmin;             /*STmin*/
    tNetTime xNAs;               /*N_As*/
    tNetTime xNAr;               /*N_Ar*/
    tNetTime xNBs;               /*N_Bs*/
    tNetTime xNBr;               /*N_Br*/
    tNetTime xNCs;               /*N_Cs*/
    tNetTime xNCr;               /*N_Cr*/
    tNetTx pfNetTx;              /*net tx*/
    tNetRx pfNetRx;              /*net rx*/
} tUdsNetLayerCfg;

typedef struct {
    tUdsLen ucRxDataLen;      /*RX can harware data len*/
    tUdsId usRxDataId;      /*RX data ID */
    unsigned char aucDataBuf[64u];   /*RX data buf*/
} tTpRxCanMsg;
#define TpRxCanMsgHeardLen  (8u)//sizoeof(tUdsLen) + sizeof(tUdsId)

//#define EN_UDS_DEBUG
#define EN_UDS_ASSERT
#define USED_FIFO

#ifndef TRUE
#define TRUE (1u)
#endif

#ifndef FALSE
#define FALSE (!TRUE)
#endif

#ifndef NULL
#define NULL ((void *)0u)
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void *)0u)
#endif

#ifdef EN_UDS_DEBUG
#include <stdio.h>
#define UdsPrintf(fmt, arg...) printf(fmt, ##arg)
#define UdsAssertPrintf(pxString) UdsPrintf(pxString)
#else
#define UdsAssertPrintf(pxString)
#endif

#ifdef EN_UDS_ASSERT
#define UdsAssert(xValue)\
    do{\
        if((xValue))\
        {\
            UdsAssertPrintf("UDS assert failed\n");\
        }\
    }while(0u)
#else
#define UdsAssert(xValue)
#endif

/*uds netwrok layer cfg info */
extern const tUdsNetLayerCfg g_stUdsNetLayerCfgInfo;

#endif /* CAN_TP_CFG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
