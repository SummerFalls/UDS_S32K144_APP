/*
 * @ ����: UDS_alg1.c
 * @ ����: ���27�����õ��ļ����㷨
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "UDS_alg1.h"

#define _MASK_FAW 0x5B798A6C//0x28355728

///////////////////////////////////////////////////////////////////////////////
//���룺
//      *seed, ָ�����ӵı���ط�
//      *key, ָ����Կ��Ҫ����ĵط�
//      KeyLen���������ӵĳ���
//�����
//˵����
void seedToKey(unsigned char *seed, unsigned char *key, const unsigned char KeyLen)
{
    //    unsigned long i;
    //    union
    //    {
    //        unsigned char byte[4];
    //        unsigned long wort;
    //    } seedlokal;
    //    const unsigned long mask = _MASK_FAW;
    //
    //    seedlokal.wort = ((unsigned long)seed[0] << 24) + ((unsigned long)seed[1] << 16)
    //                +((unsigned long)seed[2] << 8) + (unsigned long)seed[3];
    //    for (i = 0; i < 35; i++)
    //    {
    //        if (seedlokal.wort & 0x80000000)
    //        {
    //            seedlokal.wort = seedlokal.wort << 1;
    //            seedlokal.wort = seedlokal.wort ^ mask;
    //        }
    //        else
    //        {
    //            seedlokal.wort = seedlokal.wort << 1;
    //        }
    //    }
    //    for (i = 0; i < KeyLen; i++)
    //    {
    //        //key[3-i] = seedlokal.byte[i];
    //        key[i] = seedlokal.byte[i];
    //    }
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
