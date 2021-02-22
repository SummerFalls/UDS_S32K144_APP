/*
 * @ 名称: can_tp.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef CAN_TP_H_
#define CAN_TP_H_

#include "can_tp_cfg.h"

#ifdef USED_FIFO
#include "multi_cyc_fifo.h"
#endif

/*uds network man function*/
extern void CanTpMainFun(void);

/*read a frame can tp data  from UDS to Tp TxFIFO. If no data can read return FALSE, else return TRUE*/
extern unsigned char ReadAFrameDataFromCanTP(tUdsId *o_pxRxCanID,
                                             tLen *o_pxRxDataLen,
                                             unsigned char *o_pucDataBuf);

/*write a frame data from CAN Tp to UDS RxFIFO*/
extern unsigned char WriteAFrameDataInCanTP(const tUdsId i_xTxCanID,
                                            const tLen i_xTxDataLen,
                                            const unsigned char *i_pucDataBuf);

#ifdef USED_FIFO

#define TX_CAN_TP_QUEUE_LEN (50u)
#define RX_CAN_TP_QUEUE_LEN (150)

/*Init free queue list*/
extern void InitQueue(void);
#endif

#endif /* CAN_TP_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
