/*
 * @ ����: flash_cfg.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "flash_cfg.h"

/*application can used space*/
const tBlockInfo g_astBlockNum[] = {
    {0x14000u, 0x80000u},    /*block logical 0*/
};

/*logical num*/
const unsigned char g_ucBlockNum = sizeof(g_astBlockNum) / sizeof(g_astBlockNum[0u]);

#ifdef NXF47391

#endif  //end of NXF47391

/* -------------------------------------------- END OF FILE -------------------------------------------- */
