/*
 * @ 名称: can_tp_cfg.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "can_tp_cfg.h"
#include "can_driver.h"
#include "multi_cyc_fifo.h"
#include "can_cfg.h"


extern unsigned char CanTpTxMsg(const tUdsId i_xTxId,
                                const unsigned short i_usDataLen,
                                const unsigned char *i_pucDataBuf);

extern unsigned char CanTpRxMsg(tUdsId *o_pxRxId,
                                unsigned char *o_pucRxDataLen,
                                unsigned char *o_pucRxBuf);

/*uds netwrok layer cfg info */
const tUdsNetLayerCfg g_stUdsNetLayerCfgInfo = {
    1u,       /*called can tp period*/
    RX_FUN_ID,   /*can tp rx function ID*/
    RX_PHY_ID,   /*can tp rx phy ID*/
    TX_ID,   /*can tp tx ID*/
    0u,       /*BS = block size*/
    10u,      /*STmin*/
    30u,      /*N_As*/
    30u,      /*N_Ar*/
    90u,      /*N_Bs*/
    50u,      /*N_Br*/
    100u,     /*N_Cs*/
    150u,     /*N_Cr*/
    CanTpTxMsg, /*can tp tx*/
    CanTpRxMsg, /*can tp rx*/
};

/*can tp tx message: there not use CAN driver TxFIFO, directly invoked CAN send function*/
unsigned char CanTpTxMsg(const tUdsId i_xTxId,
                         const unsigned short i_usDataLen,
                         const unsigned char *i_pucDataBuf)
{
    UdsAssert(NULL_PTR == i_pucDataBuf);

    return TransmiteCANMsg(i_xTxId, (const unsigned char)i_usDataLen, i_pucDataBuf);
}

/*can tp rx message: read rx msg from CAN driver RxFIFO*/
unsigned char CanTpRxMsg(tUdsId *o_pxRxId,
                         unsigned char *o_pucRxDataLen,
                         unsigned char *o_pucRxBuf)
{
    tLen xCanRxDataLen = 0u;
    tUdsLen acuRxDataLen = 0u;
    tErroCode eStatus;
    tTpRxCanMsg stTpRxCanMsg = {0u};
    unsigned char ucIndex = 0u;

    UdsAssert(NULL_PTR == o_pxRxId);
    UdsAssert(NULL_PTR == o_pucRxBuf);
    UdsAssert(NULL_PTR == o_pucRxDataLen);

    GetCanReadLen(RX_CAN_FIFO, &xCanRxDataLen, &eStatus);

    if ((ERRO_NONE == eStatus) && (0u != xCanRxDataLen)) {
        ReadDataFromFifo(RX_CAN_FIFO,
                         TpRxCanMsgHeardLen,
                         (unsigned char *)&stTpRxCanMsg,
                         &acuRxDataLen,
                         &eStatus);

        if ((ERRO_NONE == eStatus) && (TpRxCanMsgHeardLen == acuRxDataLen)) {
            ReadDataFromFifo(RX_CAN_FIFO,
                             stTpRxCanMsg.ucRxDataLen,
                             (unsigned char *)&stTpRxCanMsg.aucDataBuf[0u],
                             &acuRxDataLen,
                             &eStatus);

            if ((ERRO_NONE == eStatus) && (stTpRxCanMsg.ucRxDataLen == acuRxDataLen)) {
                *o_pxRxId = stTpRxCanMsg.usRxDataId;
                *o_pucRxDataLen = stTpRxCanMsg.ucRxDataLen;

                for (ucIndex = 0u; ucIndex < stTpRxCanMsg.ucRxDataLen; ucIndex++) {
                    o_pucRxBuf[ucIndex] = stTpRxCanMsg.aucDataBuf[ucIndex];
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
