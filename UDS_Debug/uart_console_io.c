/*
 * @ 名称: uart_console_io.c
 * @ 描述:
 * @ 作者: Tomy
 * @ 日期: 2021年2月20日
 * @ 版本: V1.0
 * @ 历史: V1.0 2021年2月20日 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

/*
 * Routines
 * --------
 *     __read_console
 *     __write_console
 *     __close_console
 *
 * Implementation
 * --------------
 *
 *     These routines implement low-level console IO routines using
 *     any UART library which conforms to the MetroTRK UART library
 *     interface.  They are intended to provide basic console IO
 *     services in an embedded environment which lacks OS console
 *     support.
 */
////#pragma ANSI_strict off /* _No_Console will be empty file */

#include <ansi_parms.h>

#if _EWL_CONSOLE_SUPPORT

#include "console_io.h"
#include "sys/uart_console_config.h"

#include <ewl_misra_types.h>

#include "cpu.h"

#ifndef __GCN_1_3_COMPATIBILITY__
#define __GCN_1_3_COMPATIBILITY__   0
#endif

/*
 * This is defined in the TRK library for GAMECUBE
 * It is used to check if this message should be
 * sent to TRK. If not connected, the message would
 * not be handled, and hence a exception would occur
 */

#if __GCN_1_3_COMPATIBILITY__
int_t       __TRK_write_console   (__file_handle handle, uchar_t *buffer,
                                   size_t *count, __idle_proc idle_proc);
#endif

static UARTError __init_uart_console(void);


/************************************************************************/
#if  !__GCN_1_3_COMPATIBILITY__

/*//#pragma overload int_t __read_console(__file_handle handle, uchar_t * buffer, size_t * count, __idle_proc idle_proc);*/

MISRA_EXCEPTION_RULE_16_7()
#if __dest_os == __starcore
int_t __read_console(__file_handle handle, uchar_t *buffer, size_t *count, __ref_con ref_con)
#else
int_t __read_console(__file_handle handle, uchar_t *buffer, size_t *count)
#endif
{
#if __dest_os == __starcore
    //#pragma unused(handle,ref_con)
#else
    //#pragma unused(handle)
#endif
    MISRA_QUIET_UNUSED_ARGS()

    size_t limit;
    UARTError err;

    if (__init_uart_console() != kUARTNoError) {
        MISRA_EXCEPTION_RULE_14_7()
        return (int_t) __io_error;
    }

    /*
     ** A console read should return when it encounters a line-ending
     ** or after it has read the requested number of characters.
     */

    limit = *count;
    *count = 0u;
    err = kUARTNoError;

    while ((*count < limit) && (err == kUARTNoError)) {
        /* Changed this to ReadUARTN and set the count to 1
         * since it is the same functionality as ReadUART1,
         * but ReadUART1 broke end user's implementations
         */
        err = ReadUARTN(buffer, 1u );
#if __PPC_EABI__

        /* eppc wants backspace handled here */
        if (*buffer == (uchar_t)'\b') {
            if (*count != 0u) {
                --(*count);
                --buffer;
            }

            MISRA_EXCEPTION_RULE_14_5()
            continue;
        }

#endif
        ++(*count);

        if (*buffer == (uchar_t)'\n') {
            MISRA_EXCEPTION_RULE_14_6()
            break;
        }

        if (*buffer == (uchar_t)'\r') {
            *buffer = (uchar_t)'\n';
            MISRA_EXCEPTION_RULE_14_6()
            break;
        }

        ++buffer;
    }

    return ( (err == kUARTNoError) ? (int_t)__no_io_error : (int_t)__io_error );
}

#endif

/************************************************************************/

MISRA_EXCEPTION_RULE_16_7()
#if __dest_os == __starcore
int_t __write_console(__file_handle handle, uchar_t *buffer, size_t *count, __ref_con ref_con)
#else
int_t __write_console(__file_handle handle, uchar_t *buffer, size_t *count)
#endif
{
#if __dest_os == __starcore
    //#pragma unused(handle,ref_con)
#else
    //#pragma unused(handle)
#endif
    MISRA_QUIET_UNUSED_ARGS()

    /* skip the initialization if this is a TDEV
     ** Note there is no OS mask for TDEV or
     ** no console IO currently.
     */

    if (__init_uart_console() != kUARTNoError) {
        MISRA_EXCEPTION_RULE_14_7()
        return (int_t)__io_error;
    }

    if (WriteUARTN(buffer, (uint32_t)*count) != kUARTNoError) {
        *count = 0u;
        MISRA_EXCEPTION_RULE_14_7()
        return (int_t)__io_error;
    }


#if __GCN_1_3_COMPATIBILITY__
    /*
     * If TRK is connected, send a message out to it as well
     * as to the UART. This uses the mslsupp.c file supplied
     * with TRK that knows how to interface with TRK in order
     * to do serial and host i/o.
     *
     * NOTE this requires a TRK with a) framing enabled
     * and b) this function implemented that only starts up
     * when a connect message is received. Otherwise TRK
     * will get a trap (twi) instruction before it has even
     * been brought up, and hence the app will crash.
     */

    __TRK_write_console (handle, buffer, count, idle_proc);
#endif

    return (int_t)__no_io_error;

}


/************************************************************************/
//#pragma overload int_t __close_console(__file_handle handle);

int_t __close_console(__file_handle handle)
{
    //#pragma unused(handle)
    MISRA_QUIET_UNUSED_ARGS()

    return (int_t)__no_io_error;
}



/************************************************************************/
static UARTError __init_uart_console(void)
{
    UARTError err = kUARTNoError;
    static int_t initialized = 0;

    if (initialized == 0) {
        err = InitializeUART( UART_CONSOLE_BAUD_RATE );

        if (err == kUARTNoError) {
            initialized = 1;
        }
    }

    return ( err );
}

#endif /* _EWL_CONSOLE_SUPPORT */

#if __GCN_1_3_COMPATIBILITY__ && !_EWL_OS_DISK_FILE_SUPPORT

/*
 * Usually unimplemented
 */

//#pragma overload int_t __delete_file (const char * name);

int_t __delete_file (const char *name)
{
    return kUARTNoError;
}

/*
 * Usually unimplemented
 */

//#pragma overload int_t __rename_file (const char * old_name, const char * new_name);

int_t __rename_file (const char *old_name, const char *new_name)
{
    return kUARTNoError;
}

/*
 * Usually unimplemented
 */
//#pragma overload void __temp_file_name    (char * name_str, void * ptr);

void __temp_file_name   (char *name_str, void *ptr)
{
    if ( name_str ) {
        name_str[0] = 0;
    }
}

#endif

/* -------------------------------------------- END OF FILE -------------------------------------------- */
