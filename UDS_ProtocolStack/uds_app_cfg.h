/*
 * @ ����: uds_app_cfg.h
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef UDS_APP_CFG_H_
#define UDS_APP_CFG_H_

typedef unsigned short tUdsTime;

typedef struct {
    unsigned char ucCalledPeriod;           /*called uds period*/
    unsigned char ucSecurityRequestCnt;     /*security request count. If over this security request count, locked server some time.*/
    tUdsTime xLockTime;                        /*lock time*/
    tUdsTime xS3Server;                         /*s3 server time. */
} tUdsTimeInfo;

extern const tUdsTimeInfo g_stUdsAppCfg;

/*uds app time to count*/
#define UdsAppTimeToCount(xTime) ((xTime) / g_stUdsAppCfg.ucCalledPeriod)

//#define USE_AES_ALG

#endif /* UDS_APP_CFG_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
