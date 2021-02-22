/*
 * @ 名称: uds_app_cfg.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
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
