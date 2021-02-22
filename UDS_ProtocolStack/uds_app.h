/*
 * @ 名称: uds_app.h
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef UDS_APP_H_
#define UDS_APP_H_

/*app memcopy*/
extern void AppMemcopy(const void *i_pvSource, const unsigned char i_ucCopyLen, void *o_pvDest);

/*app memset*/
extern void AppMemset(const unsigned char i_ucSetValue, const unsigned short i_usLen, void *m_pvSource);

/*uds main function. ISO14229*/
extern void UDSMainFun(void);

#endif /* UDS_APP_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
