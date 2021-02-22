/*
 * @ 名称: can_tp.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "can_tp.h"
#include "can_driver.h"
/*********************************************************
**  SF  --  signle frame
**  FF  --  first frame
**  FC  --  flow control
**  CF  --  consective frame
*********************************************************/

typedef enum {
    IDLE,      /*idle*/
    RX_SF,   /*wait signle frame*/
    RX_FF,   /*wait first frame*/
    RX_FC,   /*wait flow control frame*/
    RX_CF,   /*wait consective frame*/

    TX_SF,     /*tx signle frame*/
    TX_FF,     /*tx first frame*/
    TX_FC,     /*tx flow control*/
    TX_CF,     /*tx consective frame*/

    WAIT_CONFIRM /*wait confrim*/
} tCanTpWorkStatus;

typedef enum {
    SF,        /*signle frame value*/
    FF,        /*first frame value*/
    CF,        /*consective frame value*/
    FC         /*flow control value*/
} tNetWorkFrameType;

typedef enum {
    CONTINUE_TO_SEND, /*continue to send*/
    WAIT_FC,          /*wait flow control*/
    OVERFLOW_BUF      /*overflow buf*/
} tFlowStatus;

#define DATA_LEN (8u)

#define SF_DATA_MAX_LEN (7u)  /*max signle frame data len*/
#define FF_DATA_MIN_LEN (8u)  /*min fiirst frame data len*/

#define CF_DATA_MAX_LEN (7u)  /*signle conective frame max data len*/

typedef struct {
    tUdsId xCanTpId;                           /*can tp message id*/
    tCanTpDataLen xPduDataLen;                 /*pdu data len(Rx/Tx data len)*/
    tCanTpDataLen xFFDataLen;                  /*Rx/Tx FF data len*/
    unsigned char aucDataBuf[MAX_CF_DATA_LEN]; /*Rx/Tx data buf*/
} tCanTpDataInfo;

typedef struct {
    unsigned char ucSN;          /*SN*/
    unsigned char ucBlockSize;   /*Block size*/
    tNetTime xSTmin;             /*STmin*/
    tNetTime xMaxWatiTimeout;    /*timeout time*/
    tCanTpDataInfo stCanTpDataInfo;
} tCanTpInfo;

typedef struct {
    unsigned char ucIsFree;            /*rx message status. TRUE = not received messag.*/
    tUdsId xMsgId;                     /*received message id*/
    unsigned char ucMsgLen;            /*received message len*/
    unsigned char aucMsgBuf[DATA_LEN]; /*message data buf*/
} tCanTpMsg;

typedef unsigned char (*tpfCanTpFun)(tCanTpWorkStatus *);
typedef struct {
    tCanTpWorkStatus eCanTpStaus;
    tpfCanTpFun pfCanTpFun;
} tCanTpFunInfo;

#ifdef USED_FIFO
#define RX_CAN_TP_QUEUE_ID ('R')  /*RX CAN TP queue ID*/
#define TX_CAN_TP_QUEUE_ID ('T')  /*TX CAN TP queue ID*/
#endif

/***********************Global value*************************/
static volatile tCanTpInfo gs_stTxDataInfo; /*can tp tx data*/
static volatile tNetTime gs_xTxSTmin = 0u; /*tx STmin*/
static volatile tCanTpInfo gs_stRxDataInfo; /*can tp rx data*/
static volatile tCanTpMsg gs_stRxCanTpMsg = {TRUE, 0u, 0u, {0u}};
/*********************************************************/

/***********************Static function***********************/
#define CanTpTimeToCount(xTime) ((xTime) / g_stUdsNetLayerCfgInfo.ucCalledPeriod)
#define IsSF(xNetWorkFrameType) ((((xNetWorkFrameType) >> 4u) ==  SF) ? TRUE : FALSE)
#define IsFF(xNetWorkFrameType) ((((xNetWorkFrameType) >> 4u) ==  FF) ? TRUE : FALSE)
#define IsCF(xNetWorkFrameType) ((((xNetWorkFrameType) >> 4u) ==  CF) ? TRUE : FALSE)
#define IsFC(xNetWorkFrameType) ((((xNetWorkFrameType)>> 4u) ==  FC) ? TRUE : FALSE)
#define IsRevSNValid(xSN) ((gs_stRxDataInfo.ucSN == ((xSN) & 0x0Fu)) ? TRUE : FALSE)
#define AddWaitSN()\
    do{\
        gs_stRxDataInfo.ucSN++;\
        if(gs_stRxDataInfo.ucSN > 0x0Fu)\
        {\
            gs_stRxDataInfo.ucSN = 0u;\
        }\
    }while(0u)

#define GetFrameLen(pucRevData, pxDataLen)\
    do{\
        if(TRUE == IsFF(pucRevData[0u]))\
        {\
            *(pxDataLen) = ((unsigned short)(pucRevData[0u] & 0x0fu) << 8u) | (unsigned short)pucRevData[1u];\
        }\
        else\
        {\
            *(pxDataLen) = (unsigned char)(pucRevData[0u] & 0x0fu);\
        }\
    }while(0u)

/*save FF data len*/
#define SaveFFDataLen(i_xRevFFDataLen) (gs_stRxDataInfo.stCanTpDataInfo.xFFDataLen = i_xRevFFDataLen)

/*set BS*/
#define SetBlockSize(pucBSBuf, xBlockSize) (*(pucBSBuf) = (unsigned char)(xBlockSize))

/*add block size*/
#define AddBlockSize()\
    do{\
        if(0u != g_stUdsNetLayerCfgInfo.xBlockSize)\
        {\
            gs_stRxDataInfo.ucBlockSize++;\
        }\
    }while(0u)

/*set STmin*/
#define SetSTmin(pucSTminBuf, xSTmin) (*(pucSTminBuf) = (unsigned char)(xSTmin))

/*set wait  STmin*/
#define SetWaitSTmin() (gs_stRxDataInfo.xSTmin = CanTpTimeToCount(g_stUdsNetLayerCfgInfo.xSTmin))

/*set wait frame time*/
#define SetWaitFrameTime(xWaitTimeout) (gs_stRxDataInfo.xMaxWatiTimeout = CanTpTimeToCount(xWaitTimeout))

/*set wait SN*/
#define SetWaitSN(xSN) (gs_stRxDataInfo.ucSN = xSN)

/*set FS*/
#define SetFS(pucFsBuf, xFlowStatus) (*(pucFsBuf) = (*(pucFsBuf) & 0xF0u) | (unsigned char)(xFlowStatus))

/*clear receive data buf*/
#define ClearRevDataBuf()\
    do{\
        CanTpMemset(sizeof(gs_stRxDataInfo), 0u, &gs_stRxDataInfo);\
    }while(0u)

/*add rev data len*/
#define AddRevDataLen(xRevDataLen) (gs_stRxDataInfo.stCanTpDataInfo.xPduDataLen += (xRevDataLen))

/*Is rev conective frame all.*/
#define IsReciveCFAll(xCFDataLen) ((gs_stRxDataInfo.stCanTpDataInfo.xPduDataLen + (unsigned char)(xCFDataLen)) >= (gs_stRxDataInfo.stCanTpDataInfo.xFFDataLen))

/*Is STmin timeout?*/
#define IsSTminTimeOut() ((0u == gs_stRxDataInfo.xSTmin) ? TRUE : FALSE)

/*Is wait Flow control timeout?*/
#define IsWaitFCTimeout()  ((0u == gs_stRxDataInfo.xMaxWatiTimeout) ? TRUE : FALSE)

/*Is wait conective frame timeout?*/
#define IsWaitCFTimeout() ((0u == gs_stRxDataInfo.xMaxWatiTimeout) ? TRUE : FALSE)

/*Is block sizeo overflow*/
#define IsRxBlockSizeOverflow() (((0u != g_stUdsNetLayerCfgInfo.xBlockSize) &&\
                                  (gs_stRxDataInfo.ucBlockSize >= g_stUdsNetLayerCfgInfo.xBlockSize))\
                                 ? TRUE : FALSE)

/*can tp system tick control*/
#define CanTpSytstemTickConttol()\
    do{\
        if(gs_stRxDataInfo.xSTmin)\
        {\
            gs_stRxDataInfo.xSTmin--;\
        }\
        if(gs_stRxDataInfo.xMaxWatiTimeout)\
        {\
            gs_stRxDataInfo.xMaxWatiTimeout--;\
        }\
        if(gs_stTxDataInfo.xSTmin)\
        {\
            gs_stTxDataInfo.xSTmin--;\
        }\
        if(gs_stTxDataInfo.xMaxWatiTimeout)\
        {\
            gs_stTxDataInfo.xMaxWatiTimeout--;\
        }\
    }while(0u)

/*Is transmitted data len overflow max SF?*/
#define IsTxDataLenOverflowSF() ((gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen > SF_DATA_MAX_LEN) ? TRUE : FALSE)

/*Is transmitted data less than min?*/
#define IsTxDataLenLessSF() ((0u == gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen) ? TRUE : FALSE)

/*set transmitted SF data len*/
#define SetTxSFDataLen(pucSFDataLenBuf, xTxSFDataLen) \
    do{\
        *(pucSFDataLenBuf) &= 0xF0u;\
        (*(pucSFDataLenBuf) |= (xTxSFDataLen));\
    }while(0u)

/*set transmitted FF data len*/
#define SetTxFFDataLen(pucTxFFDataLenBuf, xTxFFDataLen)\
    do{\
        *(pucTxFFDataLenBuf + 0u) &= 0xF0u;\
        *(pucTxFFDataLenBuf + 0u) |= (unsigned char)((xTxFFDataLen) >> 8u);\
        *(pucTxFFDataLenBuf + 1u) |= (unsigned char)(xTxFFDataLen);\
    }while(0u)

/*
#define SetTxFFDataLen(pucTxFFDataLenBuf, xTxFFDataLen)\
do{\
    *(pucTxFFDataLenBuf + 0u) = (unsigned char)xTxFFDataLen;\
}while(0u)
*/

/*add Tx data len*/
#define AddTxDataLen(xTxDataLen) (gs_stTxDataInfo.stCanTpDataInfo.xPduDataLen += (xTxDataLen))

/*set tx STmin */
#define SetTxSTmin() (gs_stTxDataInfo.xSTmin = CanTpTimeToCount(gs_xTxSTmin))

/*save Tx STmin*/
#define SaveTxSTmin(xTxSTmin) (gs_xTxSTmin = xTxSTmin)

/*Is Tx STmin timeout?*/
#define IsTxSTminTimeout() ((0u == gs_stTxDataInfo.xSTmin) ? TRUE : FALSE)

/*Set tx wait frame time*/
#define SetTxWaitFrameTime(xWaitTime) (gs_stTxDataInfo.xMaxWatiTimeout = CanTpTimeToCount(xWaitTime))

/*Is Tx wait frame timeout?*/
#define IsTxWaitFrameTimeout() ((0u == gs_stTxDataInfo.xMaxWatiTimeout) ? TRUE : FALSE)

/*Get FS*/
#define GetFS(ucFlowStaus, pxFlowStatusBuf) (*(pxFlowStatusBuf) = (ucFlowStaus) & 0x0Fu)

/*set tx SN*/
#define SetTxSN(pucSNBuf) (*(pucSNBuf) = gs_stTxDataInfo.ucSN | (*(pucSNBuf) & 0xF0u))

/*Add Tx SN*/
#define AddTxSN()\
    do{\
        gs_stTxDataInfo.ucSN++;\
        if(gs_stTxDataInfo.ucSN > 0x0Fu)\
        {\
            gs_stTxDataInfo.ucSN = 0u;\
        }\
    }while(0u)

/*Is Tx all*/
#define IsTxAll() ((gs_stTxDataInfo.stCanTpDataInfo.xPduDataLen >= \
                    gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen) ? TRUE : FALSE)

/*save received message ID*/
#define SaveRxMsgId(xMsgId) (gs_stRxDataInfo.stCanTpDataInfo.xCanTpId = (xMsgId))

/*clear can tp Rx msg buf*/
#define ClearCanTpRxMsgBuf()\
    do{\
        gs_stRxCanTpMsg.ucIsFree = TRUE;\
        gs_stRxCanTpMsg.ucMsgLen = 0u;\
        gs_stRxCanTpMsg.xMsgId = 0u;\
    }while(0u)

/*can tp IDLE*/
static unsigned char DoCanTpIdle(tCanTpWorkStatus *m_peNextStatus);

/*do receive signle frame*/
static unsigned char DoReceiveSF(tCanTpWorkStatus *m_peNextStatus);

/*do receive first frame*/
static unsigned char DoReceiveFF(tCanTpWorkStatus *m_peNextStatus);

/*do receive conective frame*/
static unsigned char DoReceiveCF(tCanTpWorkStatus *m_peNextStatus);

/*Transmit flow control frame*/
static unsigned char DoTransmitFC(tCanTpWorkStatus *m_peNextStatus);

/*transmit signle frame*/
static unsigned char DoTransmitSF(tCanTpWorkStatus *m_peNextStatus);

/*transmitt first frame*/
static unsigned char DoTransmitFF(tCanTpWorkStatus *m_peNextStatus);

/*wait flow control frame*/
static unsigned char DoReceiveFC(tCanTpWorkStatus *m_peNextStatus);

/*transmit conective frame*/
static unsigned char DoTransmitCF(tCanTpWorkStatus *m_peNextStatus);

/*set transmit frame type*/
static unsigned char SetFrameType(const tNetWorkFrameType i_eFrameType,
                                  unsigned char *o_pucFrameType);

/*can tp memcpy*/
static void CanTpMemcpy(const void *i_pvSource,
                        const unsigned short i_usCpyLen,
                        void *o_pvDest);

/*Can tp memset*/
static void CanTpMemset(const unsigned short i_usDataLen,
                        unsigned char i_ucSetValue,
                        void *m_pvSource);

/*received a can tp frame, copy these data in fifo.*/
static unsigned char CopyAFrameDataInRxFifo(const tUdsId i_xRxCanID,
                                            const tLen i_xRxDataLen,
                                            const unsigned char *i_pucDataBuf
                                           );

/*uds transmitted a application frame data, copy these data in TX fifo.*/
static unsigned char CopyAFrameFromFifoToBuf(tUdsId *o_pxTxCanID,
                                             unsigned char *o_pucTxDataLen,
                                             unsigned char *o_pucDataBuf);


/*********************************************************/

static const tCanTpFunInfo gs_astCanTpFunInfo[] = {
    {IDLE, DoCanTpIdle},
    {RX_SF, DoReceiveSF},
    {RX_FF, DoReceiveFF},
    {TX_FC, DoTransmitFC},
    {RX_CF, DoReceiveCF},

    {TX_SF, DoTransmitSF},
    {TX_FF, DoTransmitFF},
    {RX_FC, DoReceiveFC},
    {TX_CF, DoTransmitCF}
};

#ifdef USED_FIFO
/*Init  queue list*/
void InitQueue(void)
{
    tErroCode eStatus;

    ApplyFifo(RX_CAN_TP_QUEUE_LEN, RX_CAN_TP_QUEUE_ID, &eStatus);

    if (ERRO_NONE != eStatus) {
        UdsAssertPrintf("apply fifo RX erro!\n");

        while (1);
    }

    ApplyFifo(TX_CAN_TP_QUEUE_LEN, TX_CAN_TP_QUEUE_ID, &eStatus);

    if (ERRO_NONE != eStatus) {
#ifdef EN_UDS_DEBUG
        UdsPrintf("apply fifo TX erro code is = %d!\n", eStatus);
#endif

        while (1);
    }

    ApplyFifo(TX_CAN_TP_QUEUE_LEN, RX_CAN_FIFO, &eStatus);

    if (ERRO_NONE != eStatus) {
#ifdef EN_UDS_DEBUG
        UdsPrintf("apply fifo TX erro code is = %d!\n", eStatus);
#endif

        while (1);
    }
}
#endif

/*uds network man function*/
void CanTpMainFun(void)
{
    unsigned char ucIndex = 0u;
    static tCanTpWorkStatus s_eCanTpWorkStatus = IDLE;
    unsigned char ucFindCnt = 0u;

    /*read msg from CAN driver RxFIFO*/
    if (TRUE == g_stUdsNetLayerCfgInfo.pfNetRx((tUdsId *)&gs_stRxCanTpMsg.xMsgId,
                                               (unsigned char *)&gs_stRxCanTpMsg.ucMsgLen,
                                               (unsigned char *)gs_stRxCanTpMsg.aucMsgBuf)) {
        gs_stRxCanTpMsg.ucIsFree = FALSE;
    }

    ucFindCnt = sizeof(gs_astCanTpFunInfo) / sizeof(gs_astCanTpFunInfo[0u]);

    /*can tp time control*/
    CanTpSytstemTickConttol();

    while (ucIndex < ucFindCnt) {
        if (s_eCanTpWorkStatus == gs_astCanTpFunInfo[ucIndex].eCanTpStaus) {
            if (NULL_PTR != gs_astCanTpFunInfo[ucIndex].pfCanTpFun) {
                (void)gs_astCanTpFunInfo[ucIndex].pfCanTpFun(&s_eCanTpWorkStatus);
                //if(TRUE != gs_astCanTpFunInfo[ucIndex].pfCanTpFun(&s_eCanTpWorkStatus))
                {
                    //s_eCanTpWorkStatus = IDLE;
                }

                //break;
            }
        }

        ucIndex++;
    }

    ClearCanTpRxMsgBuf();
}

/*read a frame from UDS TxFIFO. If no data can read return FALSE, else return TRUE*/
unsigned char ReadAFrameDataFromCanTP(tUdsId *o_pxRxCanID,
                                      tLen *o_pxRxDataLen,
                                      unsigned char *o_pucDataBuf)
{
    tErroCode eStatus;
    tLen xCanReadLen = 0u;
    tLen xReadDataLen = 0u;
    unsigned char aucReadBuf[5u] = {0u};

    UdsAssert(NULL_PTR == o_pxRxCanID);
    UdsAssert(NULL_PTR == o_pucDataBuf);
    UdsAssert(NULL_PTR == o_pxRxDataLen);

    /*can read data from buf*/
    GetCanReadLen(RX_CAN_TP_QUEUE_ID, &xReadDataLen, &eStatus);

    if (ERRO_NONE != eStatus || 0u == xReadDataLen) {
        return FALSE;
    }

    /*read receive ID and data len*/
    ReadDataFromFifo(RX_CAN_TP_QUEUE_ID,
                     sizeof(aucReadBuf),
                     aucReadBuf,
                     &xCanReadLen,
                     &eStatus);

    if (ERRO_NONE != eStatus || sizeof(aucReadBuf) != xCanReadLen) {
        UdsAssertPrintf("Read data len erro!\n");
        return FALSE;
    }

    /*save rx can id*/
    *o_pxRxCanID = (((unsigned int)aucReadBuf[0u] << 24u) | ((unsigned int)aucReadBuf[1u] << 16u)
                    | ((unsigned int)aucReadBuf[2u] << 8u) | ((unsigned int)aucReadBuf[3u]));
    xCanReadLen =  aucReadBuf[4u];

    /*read data from fifo*/
    ReadDataFromFifo(RX_CAN_TP_QUEUE_ID,
                     xCanReadLen,
                     o_pucDataBuf,
                     o_pxRxDataLen,
                     &eStatus);

    if (ERRO_NONE != eStatus || (*o_pxRxDataLen != xCanReadLen)) {
        UdsAssertPrintf("Read data erro!\n");
        return FALSE;
    }

    /*check received ID*/
    if (*o_pxRxCanID != g_stUdsNetLayerCfgInfo.xRxFunId &&
            *o_pxRxCanID != g_stUdsNetLayerCfgInfo.xRxPhyId) {
        return FALSE;
    }

    return TRUE;
}

/*write a frame data tp to RxFIFO*/
unsigned char WriteAFrameDataInCanTP(const tUdsId i_xTxCanID,
                                     const tLen i_xTxDataLen,
                                     const unsigned char *i_pucDataBuf)
{
    unsigned char aucDataBuf[5u] = {0u};
    tErroCode eStatus;
    tLen xCanWriteLen = 0u;

    UdsAssert(NULL_PTR == i_pucDataBuf);

    /*check transmit ID*/
    if (i_xTxCanID != g_stUdsNetLayerCfgInfo.xTxId) {
        return FALSE;
    }

    if (0u == i_xTxDataLen) {
        return FALSE;
    }

    aucDataBuf[0u] = i_xTxCanID >> 24u;
    aucDataBuf[1u] = i_xTxCanID >> 16u;
    aucDataBuf[2u] = i_xTxCanID >> 8u;
    aucDataBuf[3u] = (unsigned char)i_xTxCanID;
    aucDataBuf[4u] = (unsigned char)i_xTxDataLen;

    /*check can wirte data len*/
    GetCanWriteLen(TX_CAN_TP_QUEUE_ID, &xCanWriteLen, &eStatus);

    if (ERRO_NONE != eStatus || xCanWriteLen < i_xTxDataLen) {
        return FALSE;
    }

    /*write data uds transmitt ID and data len*/
    WriteDataInFifo(TX_CAN_TP_QUEUE_ID, aucDataBuf, sizeof(aucDataBuf), &eStatus);

    if (ERRO_NONE != eStatus) {
        return FALSE;
    }

    /*write data in fifo*/
    WriteDataInFifo(TX_CAN_TP_QUEUE_ID, (unsigned char *)i_pucDataBuf, i_xTxDataLen, &eStatus);

    if (ERRO_NONE != eStatus) {
        return FALSE;
    }

    return TRUE;
}

/*received a can tp frame, copy these data in UDS RX fifo.*/
static unsigned char CopyAFrameDataInRxFifo(const tUdsId i_xRxCanID,
                                            const tLen i_xRxDataLen,
                                            const unsigned char *i_pucDataBuf)
{
    unsigned char aucDataBuf[5] = {0u};
    tErroCode eStatus;
    tLen xCanWriteLen = 0u;

    UdsAssert(NULL_PTR == i_pucDataBuf);

    if (0u == i_xRxDataLen) {
        return FALSE;
    }

    /* cache Rx can ID and data lenght into aucDataBuf[0-4] */
    aucDataBuf[0u] = i_xRxCanID >> 24u;
    aucDataBuf[1u] = i_xRxCanID >> 16u;
    aucDataBuf[2u] = i_xRxCanID >> 8u;
    aucDataBuf[3u] = (unsigned char)i_xRxCanID;
    aucDataBuf[4u] = (unsigned char)i_xRxDataLen;

    /*check can wirte data len*/
    GetCanWriteLen(RX_CAN_TP_QUEUE_ID, &xCanWriteLen, &eStatus);

    if (ERRO_NONE != eStatus || xCanWriteLen < i_xRxDataLen) {
        return FALSE;
    }

    /*write data uds transmitt ID and data len*/
    WriteDataInFifo(RX_CAN_TP_QUEUE_ID, aucDataBuf, sizeof(aucDataBuf), &eStatus);

    if (ERRO_NONE != eStatus) {
        return FALSE;
    }

    /*write data in fifo*/
    WriteDataInFifo(RX_CAN_TP_QUEUE_ID, (unsigned char *)i_pucDataBuf, i_xRxDataLen, &eStatus);

    if (ERRO_NONE != eStatus) {
        return FALSE;
    }

    return TRUE;
}

/*uds transmitted a application frame data, copy these data in TX fifo.*/
static unsigned char CopyAFrameFromFifoToBuf(tUdsId *o_pxTxCanID,
                                             unsigned char *o_pucTxDataLen,
                                             unsigned char *o_pucDataBuf)
{
    unsigned char aucDataBuf[5] = {0u};
    tErroCode eStatus;
    tLen xCanReadLen = 0u;
    tLen xRealReadLen = 0u;

    UdsAssert(NULL_PTR == o_pxTxCanID);
    UdsAssert(NULL_PTR == o_pucTxDataLen);
    UdsAssert(NULL_PTR == o_pucDataBuf);

    /*can read data from buf*/
    GetCanReadLen(TX_CAN_TP_QUEUE_ID, &xRealReadLen, &eStatus);

    if (ERRO_NONE != eStatus || 0u == xRealReadLen) {
        return FALSE;
    }

    /*read receive ID and data len*/
    ReadDataFromFifo(TX_CAN_TP_QUEUE_ID,
                     sizeof(aucDataBuf),
                     aucDataBuf,
                     &xCanReadLen,
                     &eStatus);

    if (ERRO_NONE != eStatus || sizeof(aucDataBuf) != xCanReadLen) {
        return FALSE;
    }

    /*save rx can id*/
    *o_pxTxCanID = (((unsigned int)aucDataBuf[0u] << 24u) | ((unsigned int)aucDataBuf[1u] << 16u)
                    | ((unsigned int)aucDataBuf[2u] << 8u) | ((unsigned int)aucDataBuf[3u]));
    xCanReadLen = aucDataBuf[4u];

    /*read data from fifo*/
    ReadDataFromFifo(TX_CAN_TP_QUEUE_ID,
                     xCanReadLen,
                     o_pucDataBuf,
                     &xRealReadLen,
                     &eStatus);

    if (ERRO_NONE != eStatus || xRealReadLen != xCanReadLen) {
        return FALSE;
    }

    *o_pucTxDataLen = (unsigned char)xRealReadLen;

    return TRUE;
}


/*can tp IDLE*/
static unsigned char DoCanTpIdle(tCanTpWorkStatus *m_peNextStatus)
{
#ifdef EN_UDS_DEBUG
    unsigned char ucIndex = 0u;
#endif

    unsigned char ucTxDataLen = (unsigned char)gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen;

    UdsAssert(NULL_PTR == m_peNextStatus);

    /*clear can tp data*/
    CanTpMemset(sizeof(tCanTpInfo), 0u, (void *)&gs_stRxDataInfo);
    CanTpMemset(sizeof(tCanTpInfo), 0u, (void *)&gs_stTxDataInfo);

    /*If receive can tp message, judge type. Only received SF or FF message.
    Others frame ignore.*/
    if (FALSE == gs_stRxCanTpMsg.ucIsFree) {
        if (TRUE == IsSF(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
            *m_peNextStatus = RX_SF;
        }

        if (TRUE == IsFF(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
            *m_peNextStatus = RX_FF;
        }
    } else {
        /*Judge have message can will tx.*/
        if (TRUE == CopyAFrameFromFifoToBuf((tUdsId *)&gs_stTxDataInfo.stCanTpDataInfo.xCanTpId,
                                            &ucTxDataLen,
                                            (unsigned char *)gs_stTxDataInfo.stCanTpDataInfo.aucDataBuf)) {
            gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen = ucTxDataLen;

            if (TRUE == IsTxDataLenOverflowSF()) {
                *m_peNextStatus = TX_FF;
            } else {
                *m_peNextStatus = TX_SF;
#ifdef EN_UDS_DEBUG

                for (ucIndex = 0u; ucIndex < ucTxDataLen; ucIndex++) {
                    UdsPrintf("%x ",
                              gs_stTxDataInfo.stCanTpDataInfo.aucDataBuf[ucIndex]);
                }

#endif
            }
        }
    }

    return TRUE;
}

/*do receive signle frame*/
static unsigned char DoReceiveSF(tCanTpWorkStatus *m_peNextStatus)
{
    unsigned char ucSFLen = 0u;
    //    unsigned char ucIndex = 0u;

    UdsAssert(NULL_PTR == m_peNextStatus);

    if (0u == gs_stRxCanTpMsg.ucMsgLen || TRUE == gs_stRxCanTpMsg.ucIsFree) {
        return FALSE;
    }

    if (TRUE != IsSF(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
        return FALSE;
    }

    GetFrameLen(gs_stRxCanTpMsg.aucMsgBuf, &ucSFLen);

    if (ucSFLen > SF_DATA_MAX_LEN) {
        return FALSE;
    }

    /*save received msg ID*/
    SaveRxMsgId(gs_stRxCanTpMsg.xMsgId);

    /*write data to UDS fifo*/
    if (FALSE == CopyAFrameDataInRxFifo(gs_stRxCanTpMsg.xMsgId,
                                        ucSFLen,
                                        (unsigned char *)&gs_stRxCanTpMsg.aucMsgBuf[1u])) {
        UdsAssertPrintf("copy data erro!\n");

        return FALSE;
    }

#ifdef EN_UDS_DEBUG
    UdsAssertPrintf("Received Signle frame: ");

    for (ucIndex = 0u; ucIndex < 8u; ucIndex++) {
        UdsPrintf("%d ", gs_stRxCanTpMsg.aucMsgBuf[ucIndex]);
    }

    UdsAssertPrintf("\n");
#endif

    *m_peNextStatus = IDLE;

    return TRUE;
}

/*do receive first frame*/
static unsigned char DoReceiveFF(tCanTpWorkStatus *m_peNextStatus)
{
    unsigned short usFFDataLen = 0u;

    UdsAssert(NULL_PTR == m_peNextStatus);

    if (0u == gs_stRxCanTpMsg.ucMsgLen || TRUE == gs_stRxCanTpMsg.ucIsFree) {
        return FALSE;
    }

    if (TRUE != IsFF(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
        UdsAssertPrintf("Received not FF\n");

        return FALSE;
    }

    /*get FF Data len*/
    GetFrameLen(gs_stRxCanTpMsg.aucMsgBuf, &usFFDataLen);

    if (usFFDataLen < FF_DATA_MIN_LEN) {
        UdsAssertPrintf("Received not FF data len less than min.\n");

#ifdef EN_UDS_DEBUG
        UdsPrintf("Received FF data len = %d\n", usFFDataLen);
#endif

        return FALSE;
    }

    /*save received msg ID*/
    SaveRxMsgId(gs_stRxCanTpMsg.xMsgId);

    /*write data in global buf. When receive all data, write these data in fifo.*/
    SaveFFDataLen(usFFDataLen);

    /*set wait flow control time*/
    SetWaitFrameTime(g_stUdsNetLayerCfgInfo.xNBr);

    /*copy data in golbal buf*/
    CanTpMemcpy((void *)&gs_stRxCanTpMsg.aucMsgBuf[2u],
                gs_stRxCanTpMsg.ucMsgLen - 2u,
                (void *)gs_stRxDataInfo.stCanTpDataInfo.aucDataBuf);

    AddRevDataLen(gs_stRxCanTpMsg.ucMsgLen - 2u);

    /*jump to next status*/
    *m_peNextStatus = TX_FC;

    ClearCanTpRxMsgBuf();

    return TRUE;
}

#ifdef DebugIo
#if 1
unsigned long g_ulStartTimer = 0u;
unsigned long g_ulEndTimer = 0u;
#endif
#endif

/*do receive conective frame*/
static unsigned char DoReceiveCF(tCanTpWorkStatus *m_peNextStatus)
{
    //    unsigned char ucRevSN = 0u;

#ifdef EN_UDS_DEBUG
    unsigned char ucIndex = 0u;
#endif

    UdsAssert(NULL_PTR == m_peNextStatus);

    //TrigDebugIo();

    /*Is timeout rx wait timeout? If wait time out receive CF over.*/
    if (TRUE == IsWaitCFTimeout()) {
        UdsAssertPrintf("wait conective frame timeout!\n");

        //TrigDebugIo();
#if 0
        g_ulEndTimer = GetSystemTimer();
#endif
        *m_peNextStatus = IDLE;

        return FALSE;
    }

    if (0u == gs_stRxCanTpMsg.ucMsgLen || TRUE == gs_stRxCanTpMsg.ucIsFree) {
        //UdsAssertPrintf("data invalid in CF!\n");

        return FALSE;
    }

    if (gs_stRxDataInfo.stCanTpDataInfo.xCanTpId != gs_stRxCanTpMsg.xMsgId) {
#ifdef EN_UDS_DEBUG
        UdsPrintf("Msg ID invalid in CF! F RX ID = %X, RX ID = %X\n",
                  gs_stRxDataInfo.stCanTpDataInfo.xCanTpId, gs_stRxCanTpMsg.xMsgId);
#endif

        return FALSE;
    }

    if (TRUE != IsCF(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
#ifdef EN_UDS_DEBUG
        UdsPrintf("Msg type invalid in CF %X!\n", gs_stRxCanTpMsg.aucMsgBuf[0u]);
#endif

        return FALSE;
    }

    /*Get rev SN. If SN invalid, return FALSE.*/
    if (TRUE != IsRevSNValid(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
        UdsAssertPrintf("Msg SN invalid in CF!\n");

        return FALSE;
    }

    /*Is timeout rx STmin? If STmin is not timeout ingore receive frame.*/
    if (TRUE != IsSTminTimeOut()) {
        UdsAssertPrintf("STmin timeout in CF!\n");

        return TRUE;
    }

    /*check receive cf all? If receive all, copy data in fifo and clear receive
    buf information. Else count SN and add receive data len.*/
    if (TRUE == IsReciveCFAll(gs_stRxCanTpMsg.ucMsgLen - 1u)) {
        /*copy all data in fifo and receive over. */
        CanTpMemcpy((void *)&gs_stRxCanTpMsg.aucMsgBuf[1u],
                    gs_stRxDataInfo.stCanTpDataInfo.xFFDataLen - gs_stRxDataInfo.stCanTpDataInfo.xPduDataLen,
                    (void *)&gs_stRxDataInfo.stCanTpDataInfo.aucDataBuf[gs_stRxDataInfo.stCanTpDataInfo.xPduDataLen]);

        /*copy all data in FIFO*/
        (void)CopyAFrameDataInRxFifo(gs_stRxDataInfo.stCanTpDataInfo.xCanTpId,
                                     gs_stRxDataInfo.stCanTpDataInfo.xFFDataLen,
                                     (unsigned char *)gs_stRxDataInfo.stCanTpDataInfo.aucDataBuf);

#ifdef EN_UDS_DEBUG
        UdsPrintf("\nCan Tp received all data : \n");

        for (ucIndex = 0u; ucIndex < gs_stRxDataInfo.stCanTpDataInfo.xFFDataLen; ucIndex++) {
            UdsPrintf("%x ", gs_stRxDataInfo.stCanTpDataInfo.aucDataBuf[ucIndex]);
        }

        UdsPrintf("\nCan Tp received data over!\n");
#endif
        *m_peNextStatus = IDLE;
    } else {
        /*If is block size overflow.*/
        if (TRUE == IsRxBlockSizeOverflow()) {
            SetWaitFrameTime(g_stUdsNetLayerCfgInfo.xNBr);

            *m_peNextStatus = TX_FC;
        } else {
            /*count Sn and set STmin, wait timeout time*/
            AddWaitSN();

            /*set wait STmin*/
            SetWaitSTmin();

            /*set wait frame time*/
            SetWaitFrameTime(g_stUdsNetLayerCfgInfo.xNCr);
        }

        /*Copy data in global fifo*/
        CanTpMemcpy((void *)&gs_stRxCanTpMsg.aucMsgBuf[1u],
                    gs_stRxCanTpMsg.ucMsgLen -  1u,
                    (void *)&gs_stRxDataInfo.stCanTpDataInfo.aucDataBuf[gs_stRxDataInfo.stCanTpDataInfo.xPduDataLen]);

        AddRevDataLen(gs_stRxCanTpMsg.ucMsgLen - 1u);
    }

    return TRUE;
}

extern volatile unsigned char g_ucIsCountTime;

/*Transmit flow control frame*/
static unsigned char DoTransmitFC(tCanTpWorkStatus *m_peNextStatus)
{
    unsigned char aucTransDataBuf[DATA_LEN] = {0u};
    unsigned char ucIsCountinueToSend = FALSE;

    /*Is wait FC timeout?*/
    if (TRUE != IsWaitFCTimeout()) {
        return TRUE;
    }

    /*set frame type*/
    (void)SetFrameType(FC, &aucTransDataBuf[0u]);

    /*Check current buf. */
    if (gs_stRxDataInfo.stCanTpDataInfo.xFFDataLen > MAX_CF_DATA_LEN) {
        /*set FS*/
        SetFS(&aucTransDataBuf[1u], OVERFLOW_BUF);
    } else {
        SetFS(&aucTransDataBuf[1u], CONTINUE_TO_SEND);

        ucIsCountinueToSend = TRUE;
    }

    /*set BS*/
    SetBlockSize(&aucTransDataBuf[1u], g_stUdsNetLayerCfgInfo.xBlockSize);

    /*add block size*/
    AddBlockSize();

    /*add wait SN*/
    AddWaitSN();

    /*set STmin*/
    SetSTmin(&aucTransDataBuf[2u], g_stUdsNetLayerCfgInfo.xSTmin);

    /*set wait next frame min time*/
    SetWaitSTmin();

    /*set wait next frame  max time*/
    SetWaitFrameTime(g_stUdsNetLayerCfgInfo.xNCr);

#if 1
    g_ucIsCountTime = 1u;
#endif
    //SetDebugIoHigh();
    //TrigDebugIo();

    /*transmit flow control*/
    if (TRUE == g_stUdsNetLayerCfgInfo.pfNetTx(g_stUdsNetLayerCfgInfo.xTxId,
                                               sizeof(aucTransDataBuf),
                                               aucTransDataBuf)) {
        if (TRUE == ucIsCountinueToSend) {
            *m_peNextStatus = RX_CF;
        } else {
            *m_peNextStatus = IDLE;
        }

        return TRUE;
    }

    return FALSE;
}

/*transmit signle frame*/
static unsigned char DoTransmitSF(tCanTpWorkStatus *m_peNextStatus)
{
    unsigned char aucDataBuf[DATA_LEN] = {0u};
    unsigned char aucTxLen = 0u;

    UdsAssert(NULL_PTR == m_peNextStatus);

    /*Check transmit data len. If data len overflow Max SF, return FALSE.*/
    if (TRUE == IsTxDataLenOverflowSF()) {
        *m_peNextStatus = TX_FF;

        return FALSE;
    }

    if (TRUE == IsTxDataLenLessSF()) {
        *m_peNextStatus = IDLE;

        return FALSE;
    }

    /*set transmitted frame type*/
    (void)SetFrameType(SF, &aucDataBuf[0u]);

    /*set transmitted data len*/
    SetTxSFDataLen(&aucDataBuf[0u], gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen);
    aucTxLen = aucDataBuf[0u] + 1u; //1u: transmitted frame type and data len need 1 byte.

    /*copy data in tx buf*/
    CanTpMemcpy((void *)gs_stTxDataInfo.stCanTpDataInfo.aucDataBuf,
                gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen,
                &aucDataBuf[1u]);

    /*request transmitted application message.*/
    if (TRUE != g_stUdsNetLayerCfgInfo.pfNetTx(gs_stTxDataInfo.stCanTpDataInfo.xCanTpId,
                                               aucTxLen, aucDataBuf))
        //  if(TRUE != g_stUdsNetLayerCfgInfo.pfNetTx(gs_stTxDataInfo.stCanTpDataInfo.xCanTpId,
        //                                              sizeof(aucDataBuf), aucDataBuf))
    {
        /*request transmitted application message failed.*/

        return FALSE;
    }

    /*jump to idle and clear transmitted message.*/
    *m_peNextStatus = IDLE;

    return TRUE;
}

/*transmitt first frame*/
static unsigned char DoTransmitFF(tCanTpWorkStatus *m_peNextStatus)
{
    unsigned char aucDataBuf[DATA_LEN] = {0u};

    UdsAssert(NULL_PTR == m_peNextStatus);

    /*Check transmit data len. If data len overflow less than SF, return FALSE.*/
    if (TRUE != IsTxDataLenOverflowSF()) {
        *m_peNextStatus = TX_SF;

        return FALSE;
    }

    /*set transmitted frame type*/
    (void)SetFrameType(FF, &aucDataBuf[0u]);

    /*set transmitted data len*/
    SetTxFFDataLen(aucDataBuf, gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen);

    /*copy data in tx buf*/
    CanTpMemcpy((void *)gs_stTxDataInfo.stCanTpDataInfo.aucDataBuf, 6u, &aucDataBuf[2u]);//6u:8bytes - frameType(1byte) - datalen(1byte)

    /*request transmitted application message.*/
    if (TRUE != g_stUdsNetLayerCfgInfo.pfNetTx(gs_stTxDataInfo.stCanTpDataInfo.xCanTpId,
                                               sizeof(aucDataBuf), aucDataBuf)) {
        /*request transmitted application message failed.*/

        return FALSE;
    }

    /*add tx data len*/
    AddTxDataLen(6u);

    /*set Tx wait time*/
    SetTxWaitFrameTime(g_stUdsNetLayerCfgInfo.xNBs);

    /*jump to idle and clear transmitted message.*/
    *m_peNextStatus = RX_FC;

    return TRUE;
}

/*wait flow control frame*/
static unsigned char DoReceiveFC(tCanTpWorkStatus *m_peNextStatus)
{
    tFlowStatus eFlowStatus;

    UdsAssert(NULL_PTR == m_peNextStatus);

    /*If tx message wait FC timeout jump to IDLE.*/
    if (TRUE == IsTxWaitFrameTimeout()) {
        UdsAssertPrintf("Wait flow control timeout.\n");

        *m_peNextStatus = IDLE;

        return FALSE;
    }

    if (0u == gs_stRxCanTpMsg.ucMsgLen || TRUE == gs_stRxCanTpMsg.ucIsFree) {

        return FALSE;
    }

    if (TRUE != IsFC(gs_stRxCanTpMsg.aucMsgBuf[0u])) {
        return FALSE;
    }

    /*Get flow status*/
    GetFS(gs_stRxCanTpMsg.aucMsgBuf[0u], &eFlowStatus);

    if (OVERFLOW_BUF == eFlowStatus) {
        *m_peNextStatus = IDLE;

        return FALSE;
    }

    /*wait flow control*/
    if (WAIT_FC == eFlowStatus) {
        /*set Tx wait time*/
        SetTxWaitFrameTime(g_stUdsNetLayerCfgInfo.xNBs);

        return TRUE;
    }

    /*contiune to send */
    if (CONTINUE_TO_SEND == eFlowStatus) {
        SetBlockSize(&gs_stTxDataInfo.ucBlockSize, gs_stRxCanTpMsg.aucMsgBuf[1u]);

        SaveTxSTmin(gs_stRxCanTpMsg.aucMsgBuf[2u]);

        SetTxWaitFrameTime(g_stUdsNetLayerCfgInfo.xNAs + g_stUdsNetLayerCfgInfo.xNCs);

        AddTxSN();
    } else {
        /*received erro Flow control*/

        *m_peNextStatus = IDLE;

        return FALSE;
    }

    *m_peNextStatus = TX_CF;

    return TRUE;
}

/*transmit conective frame*/
static unsigned char DoTransmitCF(tCanTpWorkStatus *m_peNextStatus)
{
    unsigned char aucTxDataBuf[DATA_LEN] = {0u};
    unsigned char aucTxLen = 0u;
    unsigned char aucTxAllLen = 0u;

    UdsAssert(NULL_PTR == m_peNextStatus);

    /*Is Tx STmin timeout?*/
    if (FALSE == IsTxSTminTimeout()) {
        return FALSE;
    }

    /*Is transmitted timeout?*/
    if (TRUE == IsTxWaitFrameTimeout()) {
        *m_peNextStatus = IDLE;

        return FALSE;
    }

    (void)SetFrameType(CF, &aucTxDataBuf[0u]);

    SetTxSN(&aucTxDataBuf[0u]);

    aucTxLen = gs_stTxDataInfo.stCanTpDataInfo.xFFDataLen - gs_stTxDataInfo.stCanTpDataInfo.xPduDataLen;

    if (aucTxLen >= 7u) { //7u: CAN one frame have 8 bytes.
        CanTpMemcpy((void *)&gs_stTxDataInfo.stCanTpDataInfo.aucDataBuf[gs_stTxDataInfo.stCanTpDataInfo.xPduDataLen],
                    7u, &aucTxDataBuf[1u]);

        /*request transmitted application message.*/
        if (TRUE != g_stUdsNetLayerCfgInfo.pfNetTx(gs_stTxDataInfo.stCanTpDataInfo.xCanTpId,
                                                   sizeof(aucTxDataBuf), aucTxDataBuf)) {
            /*request transmitted application message failed.*/
            return FALSE;
        }

        AddTxDataLen(7u);
    }

    else {
        CanTpMemcpy((void *)&gs_stTxDataInfo.stCanTpDataInfo.aucDataBuf[gs_stTxDataInfo.stCanTpDataInfo.xPduDataLen],
                    aucTxLen, &aucTxDataBuf[1u]);

        aucTxAllLen = aucTxLen + 1u;    //1u: transmitted fram etype need 1 byte.

        /*request transmitted application message.*/
        if (TRUE != g_stUdsNetLayerCfgInfo.pfNetTx(gs_stTxDataInfo.stCanTpDataInfo.xCanTpId,
                                                   aucTxAllLen, aucTxDataBuf)) {
            /*request transmitted application message failed.*/
            return FALSE;
        }

        AddTxDataLen(aucTxLen);
    }

    if (TRUE == IsTxAll()) {
        *m_peNextStatus = IDLE;

        return TRUE;
    }

    /*set transmitted next frame min time.*/
    SetTxSTmin();

    /*set tx next frame max time.*/
    SetTxWaitFrameTime(g_stUdsNetLayerCfgInfo.xNAs + g_stUdsNetLayerCfgInfo.xNCs);

    if (gs_stTxDataInfo.ucBlockSize) {
        gs_stTxDataInfo.ucBlockSize--;

        /*block size is equal 0,  waitting  flow control message. if not equal 0, continual send CF message.*/
        if (0u == gs_stTxDataInfo.ucBlockSize) {
            *m_peNextStatus = RX_FC;

            return TRUE;
        }
    }

    AddTxSN();

    return TRUE;
}


/*set transmit frame type*/
static unsigned char SetFrameType(const tNetWorkFrameType i_eFrameType,
                                  unsigned char *o_pucFrameType)
{
    UdsAssert(NULL_PTR == o_pucFrameType);

    if (SF == i_eFrameType ||
            FF == i_eFrameType ||
            FC == i_eFrameType ||
            CF == i_eFrameType) {
        *o_pucFrameType &= 0x0Fu;

        *o_pucFrameType |= ((unsigned char)i_eFrameType << 4u);

        return TRUE;
    }

    return FALSE;
}

/*can tp memcpy*/
static void CanTpMemcpy(const void *i_pvSource,
                        const unsigned short i_usCpyLen,
                        void *o_pvDest)
{
    unsigned short usIndex = 0u;

    UdsAssert(NULL_PTR == i_pvSource);
    UdsAssert(NULL_PTR == o_pvDest);

    if (0u == i_usCpyLen) {
        return;
    }

    for (usIndex = 0u; usIndex < i_usCpyLen; usIndex++) {
        *((unsigned char *)o_pvDest + usIndex) = *((unsigned char *)i_pvSource + usIndex);
    }
}

/*Can tp memset*/
static void CanTpMemset(const unsigned short i_usDataLen,
                        unsigned char i_ucSetValue,
                        void *m_pvSource)
{
    unsigned short usIndex = 0u;

    UdsAssert(NULL_PTR == m_pvSource);

    if (0u == i_usDataLen) {
        return;
    }

    for (usIndex = 0u; usIndex < i_usDataLen; usIndex++) {
        *((unsigned char *)m_pvSource + usIndex) = (unsigned char)i_ucSetValue;
    }
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
