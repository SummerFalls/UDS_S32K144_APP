/*
 * @ ����: uds_app.h
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
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
