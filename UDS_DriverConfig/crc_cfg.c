/*
 * @ ����: crc_cfg.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "crc_cfg.h"

#ifdef USING_HARDWARE_CRC

/*! @brief Configuration structure crc1_UserConfig0 */
crc_user_config_t crc1_UserConfig0 = {
    .crcWidth = CRC_BITS_16,
    .seed = 0xFFFFU,
    .polynomial = POLYNOMIAL_VAL,
    .writeTranspose = CRC_TRANSPOSE_NONE,
    .readTranspose = CRC_TRANSPOSE_NONE,
    .complementChecksum = false
};

#endif  //end of USING_HARDWARE_CRC

/* -------------------------------------------- END OF FILE -------------------------------------------- */
