/*
 * @ ����: can_cfg.h
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef CAN_CFG_H_
#define CAN_CFG_H_

#include "includes.h"
#include "Cpu.h"
#include "can_driver.h"

#define CAN_TX_INTERRUPUT (0u)
#define CAN_TX_POLLING (1u)

#define CAN_RX_INTERRUPUT (0u)
#define CAN_RX_POLLING (1u)

#define CAN_ERRO_INTERRUPUT (0u)
#define CAN_ERRO_POLLING (1u)

#define CAN_WAKE_UP_INTERRUPT (0u)
#define CAN_WAKE_UP_POLLING (1u)

#define USE_CAN_TX (CAN_TX_INTERRUPUT)
#define USE_CAN_RX (CAN_RX_INTERRUPUT)

#define USE_CAN_ERRO (CAN_ERRO_POLLING)
#define USE_CAN_WAKE_UP (CAN_WAKE_UP_POLLING)

/* TODO APP: #00 CAN Rx and Tx message ID, ID type, ID mask configure */
#define RX_FUN_ID       0x7DFu
#define RX_FUN_ID_TYPE  FLEXCAN_MSG_ID_STD
#define RX_FUN_ID_MASK  0xFFFFFFFFu

#define RX_PHY_ID       0x74Cu
#define RX_PHY_ID_TYPE  FLEXCAN_MSG_ID_STD
#define RX_PHY_ID_MASK  0xFFFFFFFFu

#define TX_ID           0x75Cu
#define TX_ID_TYPE      FLEXCAN_MSG_ID_STD

/* Rx and Tx mailbox number configure */
#define RX_MAILBOX_FUN_ID  (1u)
#define RX_MAILBOX_PHY_ID  (2u)
#define TX_MAILBOX_0 (3u)

#define SERVICE_10          (0x10u)
#define SERVICE_10_02       (0x02u)


typedef void (*tpfTxSuccesfullCallBack)(void);

#if 0
/*can hardware config*/
typedef struct {
    unsigned long ulCANClock;    /*can clock unit MHz*/
    unsigned short usCANBaudRate; /*can baud rate unit K*/
} tCANHardwareConfig;
#endif

typedef struct {
    uint32_t usTxID;        /*tx can id*/
    uint8_t ucTxMailBox;   /*used tx mail box*/
    flexcan_msgbuff_id_type_t TxID_Type;    /* rx mask ID type:  Standard ID or Extended ID*/
    tpfTxSuccesfullCallBack pfCallBack;
} tTxMsgConfig;

typedef struct {
    uint8_t usRxMailBox;    /*used rx mail box*/
    uint32_t usRxID;        /*rx can id*/
    uint32_t usRxMask;      /*rx mask*/
    flexcan_msgbuff_id_type_t RxID_Type;    /* rx mask ID type:  Standard ID or Extended ID*/
} tRxMsgConfig;

#ifdef NXF47391
/* use SDK2.0 can pal or flexcan driver */
//#define   IsUse_CAN_Pal_Driver

#ifdef IsUse_CAN_Pal_Driver
/* Set information about the data to be receive and sent
     *  - Standard message ID
     *  - Bit rate switch enabled to use a different bitrate for the data segment
     *  - Flexible data rate enabled
     *  - Use zeros for FD padding
     */
extern can_buff_config_t buff_RxTx_Cfg;

/* Define receive buffer */
extern can_message_t recvMsg;

#else
/* Set information about the data to be receive and sent
     *  - Standard message ID
     *  - Bit rate switch enabled to use a different bitrate for the data segment
     *  - Flexible data rate enabled
     *  - Use zeros for FD padding
     */
extern flexcan_data_info_t buff_RxTx_Cfg;

/* Define receive buffer */
extern flexcan_msgbuff_t recvMsg;
#endif  //end of IsUse_CAN_Pal_Driver

#endif  //end of NXF47391

#if 0
/*can hardware config*/
extern const tCANHardwareConfig g_stCANHardWareConfig;
#endif

/*RX can message config*/
extern const tRxMsgConfig g_astRxMsgConfig[];

/*rx can message num*/
extern const uint8_t g_ucRxCANMsgIDNum;

/*TX can message config*/
extern const tTxMsgConfig g_stTxMsgConfig;

#endif /* CAN_CFG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
