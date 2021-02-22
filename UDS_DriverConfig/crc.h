/*
 * @ ����: crc.h
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef CRC_H_
#define CRC_H_

#include "crc_cfg.h"
#include "Cpu.h"

extern void Crc_Init(void);

/************************************************************
**  Description :   creat crc. Input data in @ i_pucDataBuf
**  and data len in @ i_ulDataLen. When creat crc successfull
**  return CRC.
************************************************************/
extern void CreatCrc(const uint8_t *i_pucDataBuf, const uint32_t i_ulDataLen, uint32_t *m_pCurCrc);

extern void Crc_Test(void);

#endif /* CRC_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
