/*
 * @ 名称: multi_cyc_fifo.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef MULTI_CYC_FIFO_H_
#define MULTI_CYC_FIFO_H_

//#include "cstdint"

#include <stdint.h>

//typedef unsigned char uint8_t;

#ifndef TRUE
#define TRUE (1u)
#endif

#ifndef FALSE
#define FALSE (!TRUE)
#endif

/************************************************************
**  compile level.Use close some check.Theses marco reserved.
**  SAFE_LEVEL_O0        Highest
**  SAFE_LEVEL_O1
**  SAFE_LEVEL_O2
**  SAFE_LEVEL_O3        Lowest

**  SAFE_LEVEL_O0     @
**  SAFE_LEVEL_O1   @
**  SAFE_LEVEL_O2   @    Check input parameter over max or less than min?
**  SAFE_LEVEL_O3   @   Check pointer is safe?
************************************************************/
#define SAFE_LEVEL_O0    /**/
#define SAFE_LEVEL_O1    /**/
#define SAFE_LEVEL_O2    /*Check input parameter over max or less than min? */
#define SAFE_LEVEL_O3    /*Check pointer is safe?*/
/***********************************************************/


/*define erro code */
typedef enum {
    ERRO_NONE = 0u,         /*no erro*/
    ERRO_LEES_MIN,          /*less than min*/
    ERRO_NO_NODE,
    ERRO_OVER_MAX,          /*over max*/
    ERRO_POINTER_NULL,      /*pointer null*/
    ERRO_REGISTERED_SECOND, /*timer registered*/
    ERRO_TIME_TYPE_ERRO,    /*time type erro*/
    ERRO_TIME_USEING,
    ERRO_TIMEOUT,           /*timeout*/
    ERRO_WRITE_ERRO,
    ERRO_READ_ERRO
} tErroCode;

typedef unsigned int tId;
typedef unsigned int tLen;

#define FIFO_NUM (3u)           /*Fifo num*/
#define TOTAL_FIFO_BYTES (500u + 99u) /*config total bytes*/

/**********************************************************
**  Function Name   :   ApplyFifo
**  Description     :   Apply a fifo
**  Input Parameter :   i_xApplyFifoLen need apply fifo len
                        i_xFifoId fifo id. Use find this fifo.
**  Modify Parameter    :   none
**  Output Parameter    :   o_peApplyStatus apply status. If apply success ERRO_NONE, else ERRO_XXX
**  Return Value        :   none
**  Version         :   v00.00.01
**  Author          :   Tomlin
**  Created Date        :   2013-3-27
**********************************************************/
extern void ApplyFifo(tLen i_xApplyFifoLen, tLen i_xFifoId, tErroCode *o_peApplyStatus);

/**********************************************************
**  Function Name   :   WriteDataInFifo
**  Description     :   write data in fifo.
**  Input Parameter :   i_xFifoId   fifo id
                        i_pucWriteDataBuf Need write data buf
                        i_xWriteDatalen  write data len
**  Modify Parameter    :   none
**  Output Parameter    :   o_peWriteStatus write data status. If successfull ERRO_NONE, else ERRO_XX
**  Return Value        :   none
**  Version         :   v00.00.01
**  Author          :   Tomlin
**  Created Date        :   2013-3-27
**********************************************************/
void WriteDataInFifo(const tId i_xFifoId,
                     const uint8_t *i_pucWriteDataBuf,
                     const tLen i_xWriteDatalen,
                     tErroCode *o_peWriteStatus);

/**********************************************************
**  Function Name   :   ReadDataFromFifo
**  Description     :   Read data from fifo.
**  Input Parameter :   i_xFifoId need read fifo
                        i_xNeedReadDataLen read data len
**  Modify Parameter    :   none
**  Output Parameter    :   o_pucReadDataBuf need read data buf.
                        o_pxReadLen need read data len
                        o_peReadStatus read status. If read successfull ERRO_NONE, else ERRO_XXX
**  Return Value        :   none
**  Version         :   v00.00.01
**  Author          :   Tomlin
**  Created Date        :   2013-3-27
**********************************************************/
extern void ReadDataFromFifo(tId i_xFifoId, tLen i_xNeedReadDataLen,
                             unsigned char *o_pucReadDataBuf,
                             tLen *o_pxReadLen,
                             tErroCode *o_peReadStatus);

/**********************************************************
**  Function Name   :   GetCanReadLen
**  Description     :   Get fifo have data.
**  Input Parameter :   i_xFifoId fifo id
**  Modify Parameter    :   none
**  Output Parameter    :   o_pxCanReadLen how much data can read.
                        o_peGetStatus get status. If get successfull ERRO_NONE, else ERRO_XXX
**  Return Value        :   none
**  Version         :   v00.00.01
**  Author          :   Tomlin
**  Created Date        :   2013-3-27
**********************************************************/
extern void GetCanReadLen(tId i_xFifoId, tLen *o_pxCanReadLen, tErroCode *o_peGetStatus);

/**********************************************************
**  Function Name   :   GetCanWriteLen
**  Description     :   Get can write data.
**  Input Parameter :   i_xFifoId fifo id
**  Modify Parameter    :   none
**  Output Parameter    :   o_pxCanWriteLen how much data can write.
                        o_peGetStatus get data status. If get successfull ERRO_NONE, esle ERRO_XX
**  Return Value        :   none
**  Version         :   v00.00.01
**  Author          :   Tomlin
**  Created Date        :   2013-3-27
**********************************************************/
extern void GetCanWriteLen(tId i_xFifoId, tLen *o_pxCanWriteLen, tErroCode *o_peGetStatus);

#endif /* MULTI_CYC_FIFO_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
