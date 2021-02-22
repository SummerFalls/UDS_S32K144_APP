/*
 * @ 名称: can_driver.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "can_driver.h"
#include "can_cfg.h"
#include "boot.h"
#include "system_S32K144.h"

#include "multi_cyc_fifo.h"
#include "can_tp_cfg.h"

volatile uint8_t g_ucIsCountTime = 0u;

static void CAN_Filiter_RXIndividual(void);
static void Config_Rx_Buffer(void);
static void Config_Tx_Buffer(void);
static uint8_t IsRxCANMsgId(uint32_t i_usRxMsgId);
static void CheckCANTranmittedStatus(void);

void RxCANMsgMainFun(void);
void TransmittedCanMsgCallBack(void);

/* Config CAN filiter is individual */
static void CAN_Filiter_RXIndividual(void)
{
    uint32_t Index = 0;;

    /* Set the ID mask type is individual */
    FLEXCAN_DRV_SetRxMaskType(0u, FLEXCAN_RX_MASK_INDIVIDUAL);

    for (Index = 0u; Index < g_ucRxCANMsgIDNum; Index++) {
        FLEXCAN_DRV_SetRxIndividualMask(0u, g_astRxMsgConfig[Index].RxID_Type, \
                                        g_astRxMsgConfig[Index].usRxMailBox, g_astRxMsgConfig[Index].usRxMask);
    }
}

/* Config rx MB buffer */
static void Config_Rx_Buffer(void)
{
    uint32_t Index = 0u;

    /* Configure RX buffer with index RX_MAILBOX for Function ID and Physical ID*/
    for (Index = 0u; Index < g_ucRxCANMsgIDNum; Index++) {
        /* According to Rx msg ID type to configure MB msg ID type */
        buff_RxTx_Cfg.msg_id_type = g_astRxMsgConfig[Index].RxID_Type;

#ifdef IsUse_CAN_Pal_Driver
        CAN_ConfigRxBuff(&can_pal1_instance, g_astRxMsgConfig[Index].usRxMailBox, \
                         &buff_RxTx_Cfg, g_astRxMsgConfig[Index].usRxID);

#else
        FLEXCAN_DRV_ConfigRxMb(INST_CANCOM1, g_astRxMsgConfig[Index].usRxMailBox, \
                               &buff_RxTx_Cfg, g_astRxMsgConfig[Index].usRxID);
#endif  //end of IsUse_CAN_Pal_Driver
    }
}

/* Config tx MB buffer */
static void Config_Tx_Buffer(void)
{
    /* According to Tx msg ID type to config MB msg ID type */
    buff_RxTx_Cfg.msg_id_type = g_stTxMsgConfig.TxID_Type;

#ifdef IsUse_CAN_Pal_Driver
    /* Config Tx buffer with index Tx mailbox */
    CAN_ConfigTxBuff(&can_pal1_instance, g_stTxMsgConfig.ucTxMailBox, &buff_RxTx_Cfg);
#else
    FLEXCAN_DRV_ConfigTxMb(INST_CANCOM1, g_stTxMsgConfig.ucTxMailBox, &buff_RxTx_Cfg, g_stTxMsgConfig.usTxID);
#endif  //end of IsUse_CAN_Pal_Driver
}

/*Is Rx can message ID? If is rx can message id return TRUE, else return FALSE.*/
static uint8_t IsRxCANMsgId(uint32_t i_usRxMsgId)
{
    uint8_t ucIndex = 0u;

    while (ucIndex < g_ucRxCANMsgIDNum) {
        if (i_usRxMsgId  == g_astRxMsgConfig[ucIndex].usRxID ) {
            return TRUE;
        }

        ucIndex++;
    }

    return FALSE;
}

/*check can transmitted data successfull?*/
static void CheckCANTranmittedStatus(void)
{
    status_t Can_Tx_Statu = STATUS_ERROR;

#ifdef IsUse_CAN_Pal_Driver
    Can_Tx_Statu = CAN_GetTransferStatus(&can_pal1_instance, g_stTxMsgConfig.ucTxMailBox);

#else
    Can_Tx_Statu = FLEXCAN_DRV_GetTransferStatus(INST_CANCOM1, g_stTxMsgConfig.ucTxMailBox);
#endif  //end of IsUse_CAN_Pal_Driver

    if (STATUS_SUCCESS == Can_Tx_Statu) {
        if (NULL != g_stTxMsgConfig.pfCallBack) {
            g_stTxMsgConfig.pfCallBack();
        }
    }
}

/* Can operation completed callback function */
#ifdef IsUse_CAN_Pal_Driver
static void CAN_RxTx_Interrupt_Handle(uint32_t instance,
                                      flexcan_event_type_t eventType,
                                      uint32_t objIdx,
                                      void *driverState)
{
    uint32_t Index = 0;

    DEV_ASSERT(driverState != NULL);

    switch (eventType) {
        case CAN_EVENT_RX_COMPLETE:
            RxCANMsgMainFun();
            break;

        case CAN_EVENT_TX_COMPLETE:
            TxCANMsgMainFun();
            break;

        default:
            break;
    }

    /* Enable MB interrupt*/
    for (Index = 0u; Index < g_ucRxCANMsgIDNum; Index++) {
        CAN_Receive(&can_pal1_instance,  g_astRxMsgConfig[Index].usRxMailBox, &recvMsg);
    }
}

#else
static void CAN_RxTx_Interrupt_Handle(uint32_t instance,
                                      flexcan_event_type_t eventType,
                                      uint32_t objIdx,
                                      flexcan_state_t *flexcanState)
{
    DEV_ASSERT(driverState != NULL);

    switch (eventType) {
        case FLEXCAN_EVENT_RX_COMPLETE:
            RxCANMsgMainFun();

            /* Enable MB interrupt*/
            if (RX_MAILBOX_FUN_ID == objIdx) {
                FLEXCAN_DRV_Receive(INST_CANCOM1, RX_MAILBOX_FUN_ID, &recvMsg);
            }

            else if (RX_MAILBOX_PHY_ID == objIdx) {
                FLEXCAN_DRV_Receive(INST_CANCOM1, RX_MAILBOX_PHY_ID, &recvMsg);
            }

            else {
                ;
            }

            break;

        case FLEXCAN_EVENT_TX_COMPLETE:
            TxCANMsgMainFun();
            break;

        default:
            break;
    }
}
#endif  //end of IsUse_CAN_Pal_Driver

/* Init CAN moudule */
void InitCAN(void)
{
#ifdef IsUse_CAN_Pal_Driver
    /*Init can basical elements */
    CAN_Init(&can_pal1_instance, &can_pal1_Config0);

    /* install can rx and tx interrupt callback function */
    CAN_InstallEventCallback(&can_pal1_instance, (can_callback_t)CAN_RxTx_Interrupt_Handle, NULL);
#else
    /*Init can basical elements */
    FLEXCAN_DRV_Init(INST_CANCOM1, &canCom1_State, &canCom1_InitConfig0);

    /* install can rx and tx interrupt callback function */
    FLEXCAN_DRV_InstallEventCallback(INST_CANCOM1, (flexcan_callback_t)CAN_RxTx_Interrupt_Handle, NULL);
#endif  //end of IsUse_CAN_Pal_Driver

    /* config MBn to Rx buffer */
    Config_Rx_Buffer();

    /* config MBn to Tx buffer */
    Config_Tx_Buffer();

    /* Can Rx individual filiter */
    CAN_Filiter_RXIndividual();

    /* Start receiving data in RX_MAILBOX and Enable MBn of Rx buffer interrupt */
    {
        uint32_t Index = 0;

        for (Index = 0u; Index < g_ucRxCANMsgIDNum; Index++) {
#ifdef IsUse_CAN_Pal_Driver
            CAN_Receive(&can_pal1_instance, g_astRxMsgConfig[Index].usRxMailBox, &recvMsg);

#else
            FLEXCAN_DRV_Receive(INST_CANCOM1, g_astRxMsgConfig[Index].usRxMailBox, &recvMsg);
#endif  //end of IsUse_CAN_Pal_Driver
        }
    }

}

extern volatile uint8_t g_IsReqEnterBootloaderMode;

unsigned char WriteMsgInCanTp(uint32_t *o_pxRxId,
                              uint32_t *o_pucRxDataLen,
                              unsigned char *o_pucRxBuf)
{
    tLen xCanTxDataLen = 0u;
    tUdsLen acuRxDataLen = 0u;
    tErroCode eStatus;
    tTpRxCanMsg stTpRxCanMsg = {0u};
    unsigned char ucIndex = 0u;

    UdsAssert(NULL_PTR == o_pxRxId);
    UdsAssert(NULL_PTR == o_pucRxBuf);
    UdsAssert(NULL_PTR == o_pucRxDataLen);

    stTpRxCanMsg.usRxDataId = *o_pxRxId;
    stTpRxCanMsg.ucRxDataLen = * o_pucRxDataLen;

    for (ucIndex = 0u; ucIndex < stTpRxCanMsg.ucRxDataLen; ucIndex++) {
        stTpRxCanMsg.aucDataBuf[ucIndex] = o_pucRxBuf[ucIndex];
    }

    GetCanWriteLen(RX_CAN_FIFO, &xCanTxDataLen, &eStatus);

    if ((ERRO_NONE == eStatus) && (xCanTxDataLen >= stTpRxCanMsg.ucRxDataLen)) {
        WriteDataInFifo(RX_CAN_FIFO,
                        (unsigned char *)&stTpRxCanMsg,
                        8u,
                        &eStatus);

        if ((ERRO_NONE == eStatus)) {
            WriteDataInFifo(RX_CAN_FIFO,
                            stTpRxCanMsg.aucDataBuf,
                            stTpRxCanMsg.ucRxDataLen,
                            &eStatus);

            if ((ERRO_NONE == eStatus)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}


/*CAN Rx*/
#if USE_CAN_RX == CAN_RX_INTERRUPUT
void RxCANMsgMainFun(void)
{
    tRxCanMsg stRxCANMsg = {0u};
    uint8_t acuCANMsgLen = 0u;

#ifdef IsUse_CAN_Pal_Driver
    /* Read CAN massage data from recieve buffer recvMsg */
    stRxCANMsg.usRxDataId = recvMsg.id;
    stRxCANMsg.ucRxDataLen = recvMsg.length;
    acuCANMsgLen = recvMsg.dataLen + DLEN_ID_LEN;
#else
    stRxCANMsg.usRxDataId = recvMsg.msgId;
    stRxCANMsg.ucRxDataLen = recvMsg.dataLen;
    acuCANMsgLen = recvMsg.dataLen + DLEN_ID_LEN;
#endif  //end of IsUse_CAN_Pal_Driver

    if ((0u != stRxCANMsg.ucRxDataLen) && (TRUE == IsRxCANMsgId(stRxCANMsg.usRxDataId))) {
        /*read can message*/
        for (uint8_t ucIndex = 0u; ucIndex < stRxCANMsg.ucRxDataLen; ucIndex++) {
            stRxCANMsg.aucDataBuf[ucIndex] = recvMsg.data[ucIndex];
        }

        WriteMsgInCanTp(&stRxCANMsg.usRxDataId, &stRxCANMsg.ucRxDataLen, stRxCANMsg.aucDataBuf);
    } else {
        return;
    }

    if ((SERVICE_10 == stRxCANMsg.aucDataBuf[1]) && (SERVICE_10_02 == stRxCANMsg.aucDataBuf[2])) {
        // ClearUpdateAppCode();
        // UpdateBOOTLOADER_REQUEST();
        //SystemSoftwareReset();
        // SystemRest();

        g_IsReqEnterBootloaderMode = TRUE;
    } else {
        return;
    }
}
#else

#endif  //end of USE_CAN_RX == CAN_RX_INTERRUPUT

/* CAN Tx */
#if USE_CAN_TX == CAN_TX_INTERRUPUT
void TxCANMsgMainFun(void)
{
    /*check */
    CheckCANTranmittedStatus();
}
#else

#endif  //end of USE_CAN_TX == CAN_TX_INTERRUPUT

/* CAN communication error  */
#if USE_CAN_ERRO == CAN_ERRO_INTERRUPUT

#else
void CANErroMainFun(void)
{

}
#endif  //end of USE_CAN_ERRO == CAN_ERRO_INTERRUPUT

/*can transmite message*/
uint8_t TransmiteCANMsg(const uint32_t i_usCANMsgID,
                        const uint8_t i_ucDataLen,
                        const uint8_t *i_pucDataBuf)
{
    /* change Tx massage length */
    buff_RxTx_Cfg.data_length = i_ucDataLen;

#ifdef IsUse_CAN_Pal_Driver
    can_message_t message;

    DEV_ASSERT(i_pucDataBuf != NULL);

    if (i_usCANMsgID != g_stTxMsgConfig.usTxID) {
        return FALSE;
    }

    message.cs = 0u;
    message.id = i_usCANMsgID;
    message.length = i_ucDataLen;

    for (uint8_t Index = 0u; Index < i_ucDataLen; Index++) {
        message.data[Index] = i_pucDataBuf[Index];
    }

    CAN_Send(&can_pal1_instance, g_stTxMsgConfig.ucTxMailBox, &message);

#else
    DEV_ASSERT(i_pucDataBuf != NULL);

    if (i_usCANMsgID != g_stTxMsgConfig.usTxID) {
        return FALSE;
    }

    FLEXCAN_DRV_Send(INST_CANCOM1, g_stTxMsgConfig.ucTxMailBox, &buff_RxTx_Cfg, i_usCANMsgID, i_pucDataBuf);
#endif  //end of IsUse_CAN_Pal_Driver

    return TRUE;
}

/*transmitted can message*/
static uint8_t gs_ucIsTransmittedMsg = FALSE;

/*transmitted can message callback*/
void TransmittedCanMsgCallBack(void)
{
    gs_ucIsTransmittedMsg = TRUE;
}

/*set wait transmitted can message*/
void SetWaitTransmittedMsg(void)
{
    gs_ucIsTransmittedMsg = FALSE;
}

/*Is transmitted can messag? If transmitted return TRUE, else return FALSE.*/
uint8_t IsTransmittedMsg(void)
{
    return gs_ucIsTransmittedMsg;
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
