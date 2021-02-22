/*
 * @ 名称: crc_cfg.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef CRC_CFG_H_
#define CRC_CFG_H_

//#define USING_HARDWARE_CRC
#define   USING_SOFTWARE_LOOKUP
//#define   USING_SOFTWARE_CALCULATE

//#define   MCU_USE_PAGING

//#define   UNUSE_CRC_CHKSUM

typedef unsigned short tCrc;

#ifdef USING_HARDWARE_CRC
#include "cpu.h"

extern crc_user_config_t crc1_UserConfig0;
#endif  //end of USING_HARDWARE_CRC

#define POLYNOMIAL_VAL  0x1021u
#define CRC_SEED_INIT_VALUE 0xffff

#endif /* CRC_CFG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
