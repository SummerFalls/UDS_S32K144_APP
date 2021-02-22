/*
 * @ ����: boot.c
 * @ ����:
 * @ ����: Tomy
 * @ ����: 2021��2��20��
 * @ �汾: V1.0
 * @ ��ʷ: V1.0 2021��2��20�� Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include "boot.h"
#include "fls_app.h"

JumpToPtr pJumpTo;


/*jump to app code address*/
void JumpToAppCode(void)
{
    //asm("JSR 0x4000");
    JumpToApp();
}

/*Is can jump to app code? If need update app code reutrn TRUE, else return FALSE.*/
unsigned char IsCanJumpToAppCode(void)
{
    /*application code update*/
    if (TRUE != IsAppCodeUpdated()) {
        return FALSE;
    }

    if (TRUE == IsRequestBootloader()) {
        return FALSE;
    }

    //ClearUpdateAppCode();

    return TRUE;
}

/*Is app code updated?*/
unsigned char IsAppCodeUpdated(void)
{
    /*check app code flash status. If app code update successfull, this g_stFlashOptInfo return TRUE, else return FALSE.*/
    if (TRUE != IsReadAppInfoFromFlashValid()) {
        return FALSE;
    }

    return IsAppInFlashValid();
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
