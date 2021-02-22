/*
 * @ 名称: uds_app.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "uds_app.h"
#include "can_tp.h"

#include "uds_app_cfg.h"
#include "fls_app.h"
#include "can_driver.h"
#include "watch_dog.h"
#include "boot.h"
//#include "com.h"

#include "UDS_alg1.h"

#define NEGTIVE_ID (0x7Fu)

/*uds negative value define*/

#define SNS (0x11u)          /*service not support*/
#define SFNS (0x12u)        /*subfunction not support*/
#define IMLOIF (0x13u)       /*incorrect message length or invalid format*/
#define BRR (0x21u)          /*busy repeat request*/
#define CNC (0x22u)          /*conditions not correct*/
#define RSE (0x24u)          /*request  sequence error*/
#define ROOR (0x31u)         /*request out of range*/
#define SAD (0x33u)          /*security access denied*/
#define IK (0x35u)           /*invalid key*/
#define ENOA (0x36u)         /*exceed number of attempts*/
#define RCRRP (0x78u)        /*request correctly received-response pending*/


/*define session mode*/
#define DEFALUT_SESSION (1u << 0u)       /*default session*/
#define PROGRAM_SESSION (1u << 1u)       /*program session*/
#define EXTEND_SESSION (1u << 2u)        /*extend session*/

/*support function/physical ID request*/
#define ERRO_REQUEST_ID (0u)             /*received ID failled*/
#define SUPPORT_PHYSICAL_ADDR (1u << 0u) /*support physical ID request */
#define SUPPORT_FUNCTION_ADDR (1 << 1u)  /*support function ID request*/

/*security request*/
#define NONE_SECURITY (1u << 0u)                          /*none security can request*/
#define SECURITY_LEVEL_1 ((1 << 1u) | NONE_SECURITY)      /*security level 1 request*/
#define SECURITY_LEVEL_2 ((1u << 2u) | SECURITY_LEVEL_1)  /*security level 2 request*/

typedef struct {
    tUdsId xUdsId;
    tLen xDataLen;
    unsigned char aucDataBuf[150u];
} tUdsAppMsgInfo;

//typedef void (*tpfSerNameFun)(tUdsAppMsgInfo *);

typedef struct UDSServiceInfo {
    unsigned char ucSerNum;     /*service num. eg 0x3e/0x87...*/
    unsigned char ucSessionMode;/*default session / program session / extend session*/
    unsigned char ucSupReqMode; /*support physical / function addr*/
    unsigned char ucReqLevel;   /*request level.Lock/unlock*/
    //tpfSerNameFun pfSerNameFun; /*service name fun*/
    void (*pfSerNameFun)(struct UDSServiceInfo *, tUdsAppMsgInfo *);
} tUDSService;

typedef struct {
    unsigned char ucCurSessionMode;  /*current session mode. default/program/extend mode*/
    unsigned char ucRequsetIdMode;   /*SUPPORT_PHYSICAL_ADDR/SUPPORT_FUNCTION_ADDR*/
    unsigned char ucSecurityLevel;   /*current security level*/
    tUdsTime xUdsS3ServerTime;       /*uds s3 server time*/
    tUdsTime xSecurityReqLockTime; /*security request lock time*/
} tUdsInfo;

/*define write data subfunction*/
typedef struct {
    unsigned char ucSubfunction;     /*subfunction*/
    unsigned char ucRequestSession;  /*request session*/
    unsigned char ucRequestIdMode;   /*request id mode*/
    unsigned char ucRequestLevel;    /*request level*/
    void (*pfRoutine)(void);         /*routine*/
} tWriteDataByIdentifierInfo;

/*define security access info*/
typedef struct {
    unsigned char ucSubfunctionNumber;   /*subfunction number*/
    unsigned char ucRequestSession;      /*request session*/
    unsigned char ucRequestIDMode;       /*request id mode*/
    unsigned char ucRequestSecurityLevel;/*request security level*/
    void (*pfRoutine)(void);             /*routine*/
} tSecurityAccessInfo;

typedef struct {
    unsigned long ulStartAddr;         /*data start address*/
    unsigned long ulDataLen;           /*data len*/
} tDowloadDataInfo;

#define DOWLOAD_DATA_ADDR_LEN (4u)      /*dowload data addr len*/
#define DOWLOAD_DATA_LEN (4u)           /*dowload data len*/

typedef enum {
    ERASE_MEMORY_ROUTINE_CONTROL,   /*check erase memory routine control*/
    CHECK_SUM_ROUTINE_CONTROL,      /*check sum routine control*/
    CHECK_DEPENDENCY_ROUTINE_CONTROL /*check dependency routine control*/
} tCheckRoutineCtlInfo;

typedef void (*tpfPending)(void);

/***********************Global value************************/
static tpfPending gs_pfPendingFun = NULL_PTR;

static tUdsInfo gs_stUdsInfo = {
    DEFALUT_SESSION,
    ERRO_REQUEST_ID,
    NONE_SECURITY,
    0u,
    0u
};

/*security access info*/
static const tSecurityAccessInfo g_stSecrityAccessInfo = {
    0x01u,
    DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
    SUPPORT_PHYSICAL_ADDR,
    NONE_SECURITY,
    NULL_PTR
};

/*download data info*/
static tDowloadDataInfo gs_stDowloadDataInfo = {0u, 0u};

/*received block number*/
static unsigned char gs_ucRxBlockNum = 0u;
/*********************************************************/
/*set currrent session mode. DEFAULT_SESSION/PROGRAM_SESSION/EXTEND_SESSION */
#define SetCurrentSession(xSession) (gs_stUdsInfo.ucCurSessionMode = (xSession))

/*Is current session DEFAULT return TRUE, else return FALSE.*/
#define IsCurDefaultSession() ((DEFALUT_SESSION == gs_stUdsInfo.ucCurSessionMode) ? TRUE : FALSE)

/*Is S3server timeout?*/
#define IsS3ServerTimeout() (((0u == gs_stUdsInfo.xUdsS3ServerTime) /*&& (TRUE != IsCurDefaultSession())*/) ? TRUE : FALSE)

/*restart s3server time*/
#define RestartS3Server() (gs_stUdsInfo.xUdsS3ServerTime = UdsAppTimeToCount(g_stUdsAppCfg.xS3Server))

/*Is current session can request?*/
#define IsCurSeesionCanRequest(xServiceSession) (((gs_stUdsInfo.ucCurSessionMode) == \
                                                  ((xServiceSession) & gs_stUdsInfo.ucCurSessionMode))\
                                                 ? TRUE : FALSE)

/*set current request id  SUPPORT_PHYSICAL_ADDR/SUPPORT_FUNCTION_ADDR */
#define SetRequestIdType(xRequestIDType) (gs_stUdsInfo.ucRequsetIdMode = (xRequestIDType))

/*save received request id. If receved physical/function/none phy and function ID set rceived physicali/function/erro ID.*/
#define SaveRequestId(xRequestID)\
    do{\
        if((xRequestID) == g_stUdsNetLayerCfgInfo.xRxPhyId)\
        {\
            SetRequestIdType(SUPPORT_PHYSICAL_ADDR);\
        }\
        else if((xRequestID) == g_stUdsNetLayerCfgInfo.xRxFunId)\
        {\
            SetRequestIdType(SUPPORT_FUNCTION_ADDR);\
        }\
        else\
        {\
            SetRequestIdType(ERRO_REQUEST_ID);\
        }\
    }while(0u)


/*Is current received id can request?*/
#define IsCurRxIdCanRequest(xServiceRequestIdType) ((gs_stUdsInfo.ucRequsetIdMode == \
                                                     ((xServiceRequestIdType) & gs_stUdsInfo.ucRequsetIdMode))\
                                                    ? TRUE : FALSE)

/*set security level*/
#define SetSecurityLevel(xSecurityLevel) (gs_stUdsInfo.ucSecurityLevel = (xSecurityLevel))

/*Is current security level can request?*/
#define IsCurSecurityLevelRequet(xServiceSurityLevel) ((xServiceSurityLevel == \
                                                        ((xServiceSurityLevel) & gs_stUdsInfo.ucSecurityLevel))\
                                                       ? TRUE : FALSE)
/*Is security request lock timeout?*/
#define IsSecurityRequestLockTimeout() ((gs_stUdsInfo.xSecurityReqLockTime) ? FALSE : TRUE)

/*uds time control*/
#define UdsTimeControl()\
    do{\
        if(gs_stUdsInfo.xUdsS3ServerTime)\
        {\
            gs_stUdsInfo.xUdsS3ServerTime--;\
        }\
        if(gs_stUdsInfo.xSecurityReqLockTime)\
        {\
            gs_stUdsInfo.xSecurityReqLockTime--;\
        }\
    }while(0u)

/*********************************************************/

/**********************Static function************************/
/*set negative erro code*/
static void SetNegativeErroCode(unsigned char i_ucUDSServiceNum,
                                unsigned char i_ucErroCode,
                                tUdsAppMsgInfo *m_pstPDUMsg);

/*check routine control right?*/
static unsigned char IsCheckRoutineControlRight(const tCheckRoutineCtlInfo i_eCheckRoutineCtlId,
                                                const tUdsAppMsgInfo *m_pstPDUMsg);

/*Is erase memory routine control?*/
static unsigned char IsEraseMemoryRoutineControl(const tUdsAppMsgInfo *m_pstPDUMsg);

/*Is check sum routine control?*/
static unsigned char IsCheckSumRoutineControl(const tUdsAppMsgInfo *m_pstPDUMsg);

/*Is check programming dependency?*/
static unsigned char IsCheckProgrammingDependency(const tUdsAppMsgInfo *m_pstPDUMsg);

/*Is write fingerprint right?*/
static unsigned char IsWriteFingerprintRight(const tUdsAppMsgInfo *m_pstPDUMsg);

/*Is download data address valid?*/
static unsigned char IsDownloadDataAddrValid(const unsigned long i_ulDataAddr);

/*Is dowload data len valid?*/
static unsigned char IsDownloadDataLenValid(const unsigned long i_ulDataLen);

/*get random*/
static void GetRandom(const unsigned char i_ucNeedGetRandomLen, unsigned char *o_pucRandomBuf);

/*check random is right?*/
static unsigned char IsReceivedKeyRight(const unsigned char *i_pucReceivedKey,
                                        const unsigned char *i_pucTxSeed, const unsigned char KeyLen);

/*do check sum. If check sum right return TRUE, else return FALSE.*/
static void DoCheckSum(void);

///*do programming*/
//static void DoProgrammingData(void);

/*do erase flash*/
static void DoEraseFlash(void);

/*do check programming dependency*/
static unsigned char DoCheckProgrammingDependency(void);

/*do response checksum*/
static void DoResponseChecksum(unsigned char i_ucStatus);

/*do erase flash response*/
static void DoEraseFlashResponse(unsigned char i_ucStatus);

/*check application jump to bootloader mode*/
static void DoAppJumpToBootloader(tUdsAppMsgInfo *m_pstMsg);

/*do reset mcu*/
static void DoResetMCU(void);

/******************************UDS service function***************************************/
/*dig session*/
static void DigSession(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*control DTC setting*/
static void ControlDTCSetting(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*communication control*/
static void CommunicationControl(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*security access*/
static void SecurityAccess(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*write data by identifier*/
static void WriteDataByIdentifier(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*request download*/
static void RequestDownload(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*transfer data*/
static void TransferData(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*request transfer exit*/
static void RequestTransferExit(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*routine control*/
static void RoutineControl(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*reset ECU*/
static void ResetECU(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*Tester present service*/
static void TesterPresent(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);

/*********************************************************/

/*dig serverice config table*/
static const tUDSService gs_astUDSService[] = {
    /*diagnose mode control*/
    {
        0x10u,
        DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
        SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
        NONE_SECURITY,
        DigSession
    },

    /*communication control*/
    {
        0x28u,
        DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
        SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
        NONE_SECURITY,
        CommunicationControl
    },

#if 0
    /*control DTC setting*/
    {
        0x85u,
        DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
        SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
        NONE_SECURITY,
        ControlDTCSetting
    },

    /*security access*/
    {
        0x27u,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR,
        NONE_SECURITY,
        SecurityAccess
    },

    /*write data by identifier*/
    {
        0x2Eu,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR,
        SECURITY_LEVEL_1,
        WriteDataByIdentifier
    },

    /*request download data */
    {
        0x34u,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR,
        SECURITY_LEVEL_1,
        RequestDownload
    },

    /*transter data*/
    {
        0x36u,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR,
        SECURITY_LEVEL_1,
        TransferData
    },

    /*request exit transfer data*/
    {
        0x37u,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR,
        SECURITY_LEVEL_1,
        RequestTransferExit
    },

    /*routine control*/
    {
        0x31u,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR,
        SECURITY_LEVEL_1,
        RoutineControl
    },

    /*reset ECU*/
    {
        0x11u,
        PROGRAM_SESSION,
        SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
        SECURITY_LEVEL_1,
        ResetECU
    },

    /*diagnose mode control*/
    {
        0x3Eu,
        DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
        SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
        NONE_SECURITY,
        TesterPresent
    },
#endif
};

/*uds servie sub function config table*/
/*erase memory routine cotnrol ID*/
static const unsigned char gs_aucEraseMemoryRoutineControlId[] = {0x31u, 0x01u, 0xFFu, 0x00u};

/*check sum routine control ID*/
static const unsigned char gs_aucCheckSumRoutineControlId[] = {0x31u, 0x01u, 0x02u, 0x02u};

/*check programming dependency*/
static const unsigned char gs_aucCheckProgrammingDependencyId[] = {0x31u, 0x01u, 0xFFu, 0x01u};

/*write fingerprint id*/
static const unsigned char gs_aucWriteFingerprintId[] = {0x2Eu, 0xF1u, 0x5Au};

extern unsigned char g_ucIsRxUdsMsg;

/*********************************************************/

/*uds main function. ISO14229*/
void UDSMainFun(void)
{
    unsigned char ucIndex = 0u;
    unsigned char ucSupServNum = sizeof(gs_astUDSService) / sizeof(gs_astUDSService[0u]);
    tUdsAppMsgInfo stUdsAppMsg = {0u, 0u, {0u}};
    unsigned char ucIsFindService = FALSE;

    /*uds time control*/
    UdsTimeControl();

    if (TRUE == IsS3ServerTimeout()) {
        /*If s3 server timeout, back default session mode.*/
        SetCurrentSession(DEFALUT_SESSION);

        /*set security level. If S3server timeout, clear current security.*/
        SetSecurityLevel(NONE_SECURITY);

        gs_pfPendingFun = NULL_PTR;

        InitFlashDowloadInfo();
    }

    if ((TRUE == IsTransmittedMsg()) && (NULL_PTR != gs_pfPendingFun)) {
        (*gs_pfPendingFun)();

        gs_pfPendingFun = NULL_PTR;
    }

    /*read data from can tp*/
    if (TRUE == ReadAFrameDataFromCanTP(&stUdsAppMsg.xUdsId,
                                        &stUdsAppMsg.xDataLen,
                                        stUdsAppMsg.aucDataBuf)) {
        g_ucIsRxUdsMsg = TRUE;

        if (TRUE != IsCurDefaultSession()) {
            /*restart s3server time*/
            RestartS3Server();
        }

        /*save request id.*/
        SaveRequestId(stUdsAppMsg.xUdsId);
    } else {
        if (TRUE == IsRequestBootloader()) {
            //DoAppJumpToBootloader(&stUdsAppMsg); /*0x27 service application reset mcu*/

            SetCurrentSession(PROGRAM_SESSION);
            SetSecurityLevel(SECURITY_LEVEL_1);

            /*restart s3server time*/
            RestartS3Server();

            ClearRequestBootloader();

            return; /*tomlin add for application do 0x27u*/
        } else {
            /*don't received any uds app message, return.*/
            return;
        }
    }

    while (ucIndex < ucSupServNum) {
        if (stUdsAppMsg.aucDataBuf[0u] == gs_astUDSService[ucIndex].ucSerNum) {
            ucIsFindService = TRUE;

            if (TRUE != IsCurRxIdCanRequest(gs_astUDSService[ucIndex].ucSupReqMode)) {
                /*received ID cann't request this service.*/
                SetNegativeErroCode(stUdsAppMsg.aucDataBuf[0u], SNS, &stUdsAppMsg);

                break;
            }

            if (TRUE != IsCurSeesionCanRequest(gs_astUDSService[ucIndex].ucSessionMode)) {
                /*currnet session mode cann't request ths service.*/
                SetNegativeErroCode(stUdsAppMsg.aucDataBuf[0u], SNS, &stUdsAppMsg);

                break;
            }

            if (TRUE != IsCurSecurityLevelRequet(gs_astUDSService[ucIndex].ucReqLevel)) {
                /*current security level cann't request this service.*/
                SetNegativeErroCode(stUdsAppMsg.aucDataBuf[0u], SNS, &stUdsAppMsg);

                break;
            }

            /*find service, and do it.*/
            if (NULL_PTR != gs_astUDSService[ucIndex].pfSerNameFun) {
                gs_astUDSService[ucIndex].pfSerNameFun((tUDSService *)&gs_astUDSService[ucIndex], &stUdsAppMsg);
            }

            break;
        }

        ucIndex++;
    }

    if (TRUE != ucIsFindService) {
        /*response not support service.*/
        SetNegativeErroCode(stUdsAppMsg.aucDataBuf[0u], SNS, &stUdsAppMsg);
    }

    if (0u != stUdsAppMsg.xDataLen) {
        (void)WriteAFrameDataInCanTP(g_stUdsNetLayerCfgInfo.xTxId, stUdsAppMsg.xDataLen, stUdsAppMsg.aucDataBuf);
    }
}

extern void ReqEnterBootloaderMode(void);

/*dig session*/
static void DigSession(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucRequestSubfunction = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    ucRequestSubfunction = m_pstPDUMsg->aucDataBuf[1u];

    /*set send postive message*/
    m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
    m_pstPDUMsg->aucDataBuf[1u] = ucRequestSubfunction;
    m_pstPDUMsg->xDataLen = 2u;


    /*sub function*/
    switch (ucRequestSubfunction) {
        case 0x01u :  /*default mode*/
        case 0x81u :

            SetCurrentSession(DEFALUT_SESSION);

            if (0x81u == ucRequestSubfunction) {
                m_pstPDUMsg->xDataLen = 0u;
            }


            break;

        case 0x02u :  /*program mode*/
        case 0x82u :

            SetCurrentSession(PROGRAM_SESSION);

            if (0x82u == ucRequestSubfunction) {
                m_pstPDUMsg->xDataLen = 0u;
            }

            /*set slave mcu in bootloader*/
            //SetSlaveMcuInBootloader();

            /*restart s3server time*/
            RestartS3Server();

            ReqEnterBootloaderMode();

            SetWaitTransmittedMsg();

            /*request client timeout time*/
            SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RCRRP, m_pstPDUMsg);

            DebugPrintf("\n APP --> Request Enter bootloader mode!\n");

            gs_pfPendingFun = &SystemRest;


            break;

        case 0x03u :  /*extend mode*/
        case 0x83u :

            SetCurrentSession(EXTEND_SESSION);

            if (0x83u == ucRequestSubfunction) {
                m_pstPDUMsg->xDataLen = 0u;
            }

            /*restart s3server time*/
            RestartS3Server();

            break;

        default :

            SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);

            break;
    }
}

/*control DTC setting*/
static void ControlDTCSetting(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucRequestSubfunction = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    ucRequestSubfunction = m_pstPDUMsg->aucDataBuf[1u];

    switch (ucRequestSubfunction) {
        case 0x01u :
        case 0x02u :

            m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
            m_pstPDUMsg->aucDataBuf[1u] = ucRequestSubfunction;
            m_pstPDUMsg->xDataLen = 2u;

            break;

        case 0x81u :
        case 0x82u :

            m_pstPDUMsg->xDataLen = 0u;

            break;

        default :

            SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);

            break;
    }
}

/*communication control*/
static void CommunicationControl(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucRequestSubfunction = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    ucRequestSubfunction = m_pstPDUMsg->aucDataBuf[1u];

    switch (ucRequestSubfunction) {
        case 0x0u :
        case 0x03u :

            m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
            m_pstPDUMsg->aucDataBuf[1u] = ucRequestSubfunction;
            m_pstPDUMsg->xDataLen = 2u;

            break;

        case 0x80u :
        case 0x83u :
            /*don't transmit uds message.*/
            m_pstPDUMsg->aucDataBuf[0u] = 0u;
            m_pstPDUMsg->xDataLen = 0u;

            break;

        default :

            SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);

            break;
    }
}

/*security access*/
static void SecurityAccess(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucRequestSubfunction = 0u;
    //tSecurityAccessInfo *pstSecurityAccessInfo = (tSecurityAccessInfo *)&g_stSecrityAccessInfo;
    static unsigned char s_aucSeedBuf[16u] = {0u};
    //unsigned char ucKeyBuf[16u] = {0u};

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    /*get subfunction*/
    ucRequestSubfunction = m_pstPDUMsg->aucDataBuf[1u];

    switch (ucRequestSubfunction) {
        case 0x01u :

            m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;

            /*get random and put in m_pstPDUMsg->aucDataBuf[2u] ~ 17u byte*/
            GetRandom(sizeof(s_aucSeedBuf), s_aucSeedBuf);
            AppMemcopy(s_aucSeedBuf, sizeof(s_aucSeedBuf), &m_pstPDUMsg->aucDataBuf[2u]);

            m_pstPDUMsg->xDataLen = 2u + sizeof(s_aucSeedBuf);//2u:len+service ID

            break;

        case 0x02u :

            /*if seed is init, then not transmitted any seed.*/

            /*count random to key and check received key right?*/
            if (TRUE == IsReceivedKeyRight(&m_pstPDUMsg->aucDataBuf[2u], s_aucSeedBuf, sizeof(s_aucSeedBuf))) {
                m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;

                m_pstPDUMsg->xDataLen = 2u;

                AppMemset(0x1u, sizeof(s_aucSeedBuf), s_aucSeedBuf);

                SetSecurityLevel(SECURITY_LEVEL_1);
            } else {
                SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, IK, m_pstPDUMsg);
            }

            break;

        default :

            break;
    }
}

/*write data by identifier*/
static void WriteDataByIdentifier(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    //unsigned char ucRequestSubfunction = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    /*Is write fingerprint id right?*/
    if (TRUE == IsWriteFingerprintRight(m_pstPDUMsg)) {
        /*do write fingerprint*/
        SavePrintfigner(&m_pstPDUMsg->aucDataBuf[3u], (m_pstPDUMsg->xDataLen - 3u));

        m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
        m_pstPDUMsg->aucDataBuf[1u] = 0xF1u;
        m_pstPDUMsg->aucDataBuf[2u] = 0x5Au;
        m_pstPDUMsg->xDataLen = 3u;
    } else {
        /*don't have this routine control ID*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);
    }
}

/*request download*/
static void RequestDownload(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    //unsigned char ucRequestSubfunction = 0u;
    unsigned char ucIndex = 0u;
    unsigned char ucRet = TRUE;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    ucRet = TRUE;

    if (m_pstPDUMsg->xDataLen < (DOWLOAD_DATA_ADDR_LEN + DOWLOAD_DATA_LEN + 1u + 2u)) {
        ucRet = FALSE;

        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, IMLOIF, m_pstPDUMsg);
    }

    if (TRUE == ucRet) {
        /*get data addr */
        gs_stDowloadDataInfo.ulStartAddr = 0u;

        for (ucIndex = 0u; ucIndex < DOWLOAD_DATA_ADDR_LEN; ucIndex++) {
            gs_stDowloadDataInfo.ulStartAddr <<= 8u;
            gs_stDowloadDataInfo.ulStartAddr |= m_pstPDUMsg->aucDataBuf[ucIndex + 3u];//3u = N_PCI(1) + SID34(1) + dataFormatldentifier(1)
        }

        /*get data len*/
        gs_stDowloadDataInfo.ulDataLen = 0u;

        for (ucIndex = 0u; ucIndex < DOWLOAD_DATA_LEN; ucIndex++) {
            gs_stDowloadDataInfo.ulDataLen <<= 8u;
            gs_stDowloadDataInfo.ulDataLen |= m_pstPDUMsg->aucDataBuf[ucIndex + 7u];
        }
    }

    /*Is download data  addr  and len valid?*/
    if (((TRUE != IsDownloadDataAddrValid(gs_stDowloadDataInfo.ulStartAddr)) ||
            (TRUE != IsDownloadDataLenValid(gs_stDowloadDataInfo.ulDataLen))) && (TRUE == ucRet)) {
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, ROOR, m_pstPDUMsg);

        ucRet = FALSE;
    }

    if (TRUE == ucRet) {
        /*save received program addr and data len*/
        (void)SaveReceivedProgramInfo(gs_stDowloadDataInfo.ulStartAddr, gs_stDowloadDataInfo.ulDataLen);

        /*set wait transfer data step(0x34 service)*/
        g_stFlashDownloadInfo.eDownloadStep = FL_TRANSFER_STEP;
        g_stFlashDownloadInfo.ulStartAddr = gs_stDowloadDataInfo.ulStartAddr;
        g_stFlashDownloadInfo.ulLength = gs_stDowloadDataInfo.ulDataLen;

        /*fill pastive message*/
        m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
        m_pstPDUMsg->aucDataBuf[1u] = 0x10u;
        m_pstPDUMsg->aucDataBuf[2u] = 0x80u;
        m_pstPDUMsg->xDataLen = 3u;

        /*set wait received block number*/
        gs_ucRxBlockNum = 1u;
    } else {
        InitFlashDowloadInfo();

        /*set request transfer data step(0x34 service)*/
        g_stFlashDownloadInfo.eDownloadStep = FL_REQUEST_STEP;
    }
}

/*transfer data*/
static void TransferData(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    //unsigned char ucRequestSubfunction = 0u;
    //static unsigned char s_ucBlockNum = 0u;
    static unsigned long s_ulRevDatalen = 0u;
    unsigned char ucRet = TRUE;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    ucRet = TRUE;

    /*request sequence erro*/
    if ((FL_TRANSFER_STEP != g_stFlashDownloadInfo.eDownloadStep) && (TRUE == ucRet)) {
        ucRet = FALSE;

        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RSE, m_pstPDUMsg);
    }

    s_ulRevDatalen = m_pstPDUMsg->xDataLen - 2u;

    if ((gs_ucRxBlockNum != m_pstPDUMsg->aucDataBuf[1u]) && (TRUE == ucRet)) {
        ucRet = FALSE;

        /*received data is not wait block number*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RSE, m_pstPDUMsg);
    }

    gs_ucRxBlockNum++;

    /*copy flash data in flash area*/
    if ((TRUE != FlashProgramRegion(gs_stDowloadDataInfo.ulStartAddr, &m_pstPDUMsg->aucDataBuf[2u], (m_pstPDUMsg->xDataLen - 2u))) && (TRUE == ucRet)) {
        ucRet = FALSE;

        /*saved data and information failled!*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, CNC, m_pstPDUMsg);
    } else {
        gs_stDowloadDataInfo.ulStartAddr += (m_pstPDUMsg->xDataLen - 2u);
        gs_stDowloadDataInfo.ulDataLen -= (m_pstPDUMsg->xDataLen - 2u);
    }

    /*received all data*/
    if ((0u == gs_stDowloadDataInfo.ulDataLen) && (TRUE == ucRet)) {
        s_ulRevDatalen = 0u;

        gs_ucRxBlockNum = 0u;

        /*set wait exit transfer step(0x37 service)*/
        g_stFlashDownloadInfo.eDownloadStep = FL_EXIT_TRANSFER_STEP;
    }


    if (TRUE == ucRet) {
        /*tranmitted postive message.*/
        m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
        m_pstPDUMsg->xDataLen = 4u;
    } else {
        InitFlashDowloadInfo();

        /*set request transfer data step(0x34 service)*/
        g_stFlashDownloadInfo.eDownloadStep = FL_REQUEST_STEP;

        gs_ucRxBlockNum = 0u;
    }
}

/*request transfer exit*/
static void RequestTransferExit(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    //unsigned char ucRequestSubfunction = 0u;
    unsigned char ucRet = TRUE;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    if (FL_EXIT_TRANSFER_STEP != g_stFlashDownloadInfo.eDownloadStep) {
        ucRet = FALSE;

        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RSE, m_pstPDUMsg);
    }

    if (TRUE == ucRet) {
        g_stFlashDownloadInfo.eDownloadStep = FL_CHECKSUM_STEP;

        /*tranmitted postive message.*/
        m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
        m_pstPDUMsg->xDataLen = 1u;
    } else {
        InitFlashDowloadInfo();
    }

}

/*routine control*/
static void RoutineControl(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    //unsigned char ucRequestSubfunction = 0u;
    unsigned char ucRet = FALSE;
    unsigned long ulReceivedCrc = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    RestartS3Server();

    /*Is erase memory routine control?*/
    if (TRUE == IsEraseMemoryRoutineControl(m_pstPDUMsg)) {
        /*do erase memeory*/
        SetWaitTransmittedMsg();

        /*request client timeout time*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RCRRP, m_pstPDUMsg);

        gs_pfPendingFun = &DoEraseFlash;
    }

    /*Is check sum routine control?*/
    else if (TRUE == IsCheckSumRoutineControl(m_pstPDUMsg)) {
        ulReceivedCrc = m_pstPDUMsg->aucDataBuf[4u];
        ulReceivedCrc = (ulReceivedCrc << 8u) | m_pstPDUMsg->aucDataBuf[5u];
        ulReceivedCrc = (ulReceivedCrc << 8u) | m_pstPDUMsg->aucDataBuf[6u];
        ulReceivedCrc = (ulReceivedCrc << 8u) | m_pstPDUMsg->aucDataBuf[7u];
        SavedReceivedCheckSumCrc(ulReceivedCrc);

        /*do check sum routine control*/
        SetWaitTransmittedMsg();

        /*request client timeout time*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RCRRP, m_pstPDUMsg);

        gs_pfPendingFun = &DoCheckSum;
    }

    /*Is check programming dependency?*/
    else if (TRUE == IsCheckProgrammingDependency(m_pstPDUMsg)) {
        /*write application information in flash.*/
        (void)WriteFlashAppInfo();

        /*do check programming dependency*/
        ucRet = DoCheckProgrammingDependency();

        if (TRUE == ucRet) {
            m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
            m_pstPDUMsg->xDataLen = 4u;
        } else {
            /*don't have this routine control ID*/
            SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);
        }
    } else {
        /*don't have this routine control ID*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);
    }

}

/*reset ECU*/
static void ResetECU(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    //unsigned char ucRequestSubfunction = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

#if 0
    /*reset ECU*/
    SystemRest();

    while (1) {
        /*wait watch dog reset mcu*/
    }

#endif

    /*do check sum routine control*/
    SetWaitTransmittedMsg();

#ifdef EnSlaveBootloader

    if ((TRUE == GoExecute(SLAVE_APP_START_ADDR)))
#endif
    {
        /*If program data in flash successfull, set Bootloader will jump to application flag*/
        EraseFlashDriverInRAM();

        /*If invalid application software in flash, then this step set application jump to bootloader flag*/
        UpdateAppSuccessfull();

        /*request client timeout time*/
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, RCRRP, m_pstPDUMsg);

        gs_pfPendingFun = &DoResetMCU;
    }

#ifdef EnSlaveBootloader
    else {
        /*reset slave mcu erro, transmitted */
        SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, CNC, m_pstPDUMsg);

        InitFlashDowloadInfo();
    }

#endif
}

/*Tester present service*/
static void TesterPresent(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucRequestSubfunction = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);
    UdsAssert(NULL_PTR == i_pstUDSServiceInfo);

    ucRequestSubfunction = m_pstPDUMsg->aucDataBuf[1u];

    /*sub function*/
    switch (ucRequestSubfunction) {
        case 0x00u :  /*zero subFunction*/
            /*set send postive message*/
            m_pstPDUMsg->aucDataBuf[0u] = i_pstUDSServiceInfo->ucSerNum + 0x40u;
            m_pstPDUMsg->aucDataBuf[1u] = ucRequestSubfunction;
            m_pstPDUMsg->xDataLen = 2u;
            break;

        case 0x80u :  /*program mode*/
            m_pstPDUMsg->xDataLen = 0u;
            break;

        default :
            SetNegativeErroCode(i_pstUDSServiceInfo->ucSerNum, SFNS, m_pstPDUMsg);
            break;
    }
}


/*do reset mcu*/
static void DoResetMCU(void)
{
    /*reset ECU*/
    SystemRest();

    while (1) {
        /*wait watch dog reset mcu*/
    }
}

/*set negative erro code*/
static void SetNegativeErroCode(unsigned char i_ucUDSServiceNum,
                                unsigned char i_ucErroCode,
                                tUdsAppMsgInfo *m_pstPDUMsg)
{
    UdsAssert(NULL_PTR == m_pstPDUMsg);

    m_pstPDUMsg->aucDataBuf[0u] = NEGTIVE_ID;
    m_pstPDUMsg->aucDataBuf[1u] = i_ucUDSServiceNum;
    m_pstPDUMsg->aucDataBuf[2u] = i_ucErroCode;
    m_pstPDUMsg->xDataLen = 3u;
}

/*get random*/
static void GetRandom(const unsigned char i_ucNeedGetRandomLen, unsigned char *o_pucRandomBuf)
{
    unsigned short usStartRandomAddr = 0x30;
    unsigned char ucIndex = 0u;

    UdsAssert(NULL_PTR == o_pucRandomBuf);

    for (ucIndex = 0u; ucIndex < i_ucNeedGetRandomLen; ucIndex++) {
        o_pucRandomBuf[ucIndex] = *((unsigned char *)(usStartRandomAddr + (unsigned short)ucIndex));
    }
}

/*check random is right?*/
static unsigned char IsReceivedKeyRight(const unsigned char *i_pucReceivedKey,
                                        const unsigned char *i_pucTxSeed, const unsigned char KeyLen)
{
    UdsAssert(NULL_PTR == i_pucReceivedKey);
    UdsAssert(NULL_PTR == i_pucTxSeed);

#ifdef USE_AES_ALG
    unsigned char *aucKey = NULL;
    unsigned char ucIndex = 0u;

    seedToKey((unsigned char *)i_pucTxSeed, aucKey, KeyLen);

    while (ucIndex < sizeof(aucKey)) {
        if (i_pucReceivedKey[ucIndex] != aucKey[ucIndex]) {
            return FALSE;
        }

        ucIndex++;
    }

#endif  //end of USE_AES_ALG

    return TRUE;
}

/*app memcopy*/
void AppMemcopy(const void *i_pvSource, const unsigned char i_ucCopyLen, void *o_pvDest)
{
    unsigned char ucIndex = 0u;

    UdsAssert(NULL_PTR == i_pvSource);
    UdsAssert(NULL_PTR == o_pvDest);

    for (ucIndex = 0u; ucIndex < i_ucCopyLen; ucIndex++) {
        ((unsigned char *)o_pvDest)[ucIndex] = (unsigned char)((unsigned char *)i_pvSource)[ucIndex];
    }
}

/*app memset*/
void AppMemset(const unsigned char i_ucSetValue, const unsigned short i_usLen, void *m_pvSource)
{
    unsigned short usIndex = 0u;

    UdsAssert(NULL_PTR == m_pvSource);

    for (usIndex = 0u; usIndex < i_usLen; usIndex++) {
        ((unsigned char *)m_pvSource)[usIndex] = (unsigned char)i_ucSetValue;
    }
}

/*check routine control right?*/
static unsigned char IsCheckRoutineControlRight(const tCheckRoutineCtlInfo i_eCheckRoutineCtlId,
                                                const tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucIndex = 0u;
    unsigned char ucFindCnt = 0u;
    unsigned char *pucDestRoutineCltId = NULL_PTR;

    UdsAssert(NULL_PTR == m_pstPDUMsg);

    switch (i_eCheckRoutineCtlId) {
        case ERASE_MEMORY_ROUTINE_CONTROL :

            pucDestRoutineCltId = (unsigned char *)&gs_aucEraseMemoryRoutineControlId[0u];

            ucFindCnt = sizeof(gs_aucEraseMemoryRoutineControlId);

            break;

        case CHECK_SUM_ROUTINE_CONTROL :

            pucDestRoutineCltId = (unsigned char *)&gs_aucCheckSumRoutineControlId[0u];

            ucFindCnt = sizeof(gs_aucCheckSumRoutineControlId);

            break;

        case CHECK_DEPENDENCY_ROUTINE_CONTROL :

            pucDestRoutineCltId = (unsigned char *)&gs_aucCheckProgrammingDependencyId[0u];

            ucFindCnt = sizeof(gs_aucCheckProgrammingDependencyId);

            break;

        default :

            return FALSE;

            /*This is not have break*/
    }

    if ((NULL_PTR == pucDestRoutineCltId) || (m_pstPDUMsg->xDataLen < ucFindCnt)) {
        return FALSE;
    }

    while (ucIndex < ucFindCnt) {
        if (m_pstPDUMsg->aucDataBuf[ucIndex] != pucDestRoutineCltId[ucIndex]) {
            return FALSE;
        }

        ucIndex++;
    }

    return TRUE;

}

/*Is erase memory routine control?*/
static unsigned char IsEraseMemoryRoutineControl(const tUdsAppMsgInfo *m_pstPDUMsg)
{
    return IsCheckRoutineControlRight(ERASE_MEMORY_ROUTINE_CONTROL, m_pstPDUMsg);
}

/*Is check sum routine control?*/
static unsigned char IsCheckSumRoutineControl(const tUdsAppMsgInfo *m_pstPDUMsg)
{
    return IsCheckRoutineControlRight(CHECK_SUM_ROUTINE_CONTROL, m_pstPDUMsg);
}

/*Is check programming dependency?*/
static unsigned char IsCheckProgrammingDependency(const tUdsAppMsgInfo *m_pstPDUMsg)
{
    return IsCheckRoutineControlRight(CHECK_DEPENDENCY_ROUTINE_CONTROL, m_pstPDUMsg);
}

/*Is write fingerprint right?*/
static unsigned char IsWriteFingerprintRight(const tUdsAppMsgInfo *m_pstPDUMsg)
{
    unsigned char ucIndex = 0u;
    unsigned char ucWriteFingerprintIdLen = 0u;

    UdsAssert(NULL_PTR == m_pstPDUMsg);

    ucWriteFingerprintIdLen = sizeof(gs_aucWriteFingerprintId);

    if (m_pstPDUMsg->xDataLen < ucWriteFingerprintIdLen) {
        return FALSE;
    }

    while (ucIndex < ucWriteFingerprintIdLen) {
        if (m_pstPDUMsg->aucDataBuf[ucIndex] != gs_aucWriteFingerprintId[ucIndex]) {
            return FALSE;
        }

        ucIndex++;
    }

    return TRUE;

}

/*Is download data address valid?*/
static unsigned char IsDownloadDataAddrValid(const unsigned long i_ulDataAddr)
{

    return TRUE;
}

/*Is dowload data len valid?*/
static unsigned char IsDownloadDataLenValid(const unsigned long i_ulDataLen)
{

    return TRUE;
}

/*do check sum. If check sum right return TRUE, else return FALSE.*/
static void DoCheckSum(void)
{
    /*need request client delay time for flash checking flash data*/
    g_stFlashDownloadInfo.eActiveJob = FALSH_CHECKING;

    SetWaitDoResponse(&DoResponseChecksum);
}

/*do response checksum*/
static void DoResponseChecksum(unsigned char i_ucStatus)
{
    unsigned char ucIndex = 0u;
    unsigned char aucResponseBuf[8u] = {0u};
    unsigned char ucTxDataLen = 0u;

    //if(STATUS_SUCCESS == i_ucStatus)
    {
        ucTxDataLen = sizeof(gs_aucCheckSumRoutineControlId) / sizeof(gs_aucCheckSumRoutineControlId[0u]);
        aucResponseBuf[0u] = gs_aucCheckSumRoutineControlId[0u] + 0x40u;

        for (ucIndex = 0u; ucIndex < ucTxDataLen - 1u; ucIndex++) {
            aucResponseBuf[ucIndex + 1u] = gs_aucCheckSumRoutineControlId[ucIndex + 1u];
        }
    }

    if (STATUS_SUCCESS == i_ucStatus) {
        aucResponseBuf[ucTxDataLen] = 0u;
    } else {
        aucResponseBuf[ucTxDataLen] = 1u;
    }

    ucTxDataLen++;

#if 0
    else {
        ucTxDataLen = 3u;
        aucResponseBuf[0u] = NEGTIVE_ID;
        aucResponseBuf[1u] = 0x31u;
        aucResponseBuf[2u] = CNC;
    }

#endif

    (void)WriteAFrameDataInCanTP(g_stUdsNetLayerCfgInfo.xTxId, ucTxDataLen, aucResponseBuf);
}

///*do programming*/
//static void DoProgrammingData(void)
//{
//  /*need request client delay time for flash programming flash*/
//  g_stFlashDownloadInfo.eActiveJob = FALSH_PROGRAMMING;
//
//  SetWaitDoResponse(&DoEraseFlashResponse);
//}

/*do erase flash response*/
static void DoEraseFlashResponse(unsigned char i_ucStatus)
{
    unsigned char ucIndex = 0u;
    unsigned char aucResponseBuf[8u] = {0u};
    unsigned char ucTxDataLen = 0u;

    //if(TRUE == i_ucStatus)
    {
        ucTxDataLen = sizeof(gs_aucEraseMemoryRoutineControlId) / sizeof(gs_aucEraseMemoryRoutineControlId[0u]);
        aucResponseBuf[0u] = gs_aucEraseMemoryRoutineControlId[0u] + 0x40u;

        for (ucIndex = 0u; ucIndex < ucTxDataLen - 1u; ucIndex++) {
            aucResponseBuf[ucIndex + 1u] = gs_aucEraseMemoryRoutineControlId[ucIndex + 1u];
        }
    }
#if 0
    else {
        aucResponseBuf[0u] = NEGTIVE_ID;
        aucResponseBuf[1u] = 0x31u;
        aucResponseBuf[2u] = CNC;
    }

#endif

    if (STATUS_SUCCESS == i_ucStatus) {
        aucResponseBuf[ucTxDataLen] = 0u;
    } else {
        aucResponseBuf[ucTxDataLen] = 1u;
    }

    ucTxDataLen++;

    (void)WriteAFrameDataInCanTP(g_stUdsNetLayerCfgInfo.xTxId, ucTxDataLen, aucResponseBuf);
}

/*do erase flash*/
static void DoEraseFlash(void)
{
    /*do erase flash need request client delay timeout*/
    g_stFlashDownloadInfo.eActiveJob = FALSH_ERASING;

    SetWaitDoResponse(&DoEraseFlashResponse);
}

/*do check programming dependency*/
static unsigned char DoCheckProgrammingDependency(void)
{
#ifdef DebugBootloader
    return TRUE;
#endif

    if (TRUE == IsReadAppInfoFromFlashValid()) {
        if (TRUE == IsAppInFlashValid()) {
            return TRUE;
        }
    }

    return FALSE;
}

/*check application jump to bootloader mode*/
static void DoAppJumpToBootloader(tUdsAppMsgInfo *m_pstMsg)
{
    m_pstMsg->xUdsId = g_stUdsNetLayerCfgInfo.xRxPhyId;
    m_pstMsg->xDataLen = 2u;
    m_pstMsg->aucDataBuf[0u] = 0x10u;
    m_pstMsg->aucDataBuf[1u] = 0x02u;

    SaveRequestId(m_pstMsg->xUdsId);
    SetCurrentSession(EXTEND_SESSION);
    SetSecurityLevel(NONE_SECURITY);
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
