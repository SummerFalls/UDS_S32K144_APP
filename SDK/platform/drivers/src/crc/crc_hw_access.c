/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @page misra_violations MISRA-C:2012 violations
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * The function is defined for use by application code.
 */

#include "crc_hw_access.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Initial checksum */
#define CRC_INITIAL_SEED        (0U)

/*******************************************************************************
 * Code
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_Init
 * Description   : This function initializes the module to default configuration
 * (Initial checksum: 0U,
 * Default polynomial: 0x1021U,
 * Type of read transpose: CRC_TRANSPOSE_NONE,
 * Type of write transpose: CRC_TRANSPOSE_NONE,
 * No complement of checksum read,
 * 32-bit CRC).
 *
 *END**************************************************************************/
void CRC_Init(CRC_Type * const base)
{
    /* Set CRC mode to 32-bit */
    CRC_SetProtocolWidth(base, CRC_BITS_32);

    /* Set read/write transpose and complement checksum to none */
    CRC_SetWriteTranspose(base, CRC_TRANSPOSE_NONE);
    CRC_SetReadTranspose(base, CRC_TRANSPOSE_NONE);
    CRC_SetFXorMode(base, false);

    /* Write polynomial to 0x1021U */
    CRC_SetPolyReg(base, FEATURE_CRC_DEFAULT_POLYNOMIAL);

    /* Write seed to zero */
    CRC_SetSeedOrDataMode(base, true);
    CRC_SetDataReg(base, CRC_INITIAL_SEED);
    CRC_SetSeedOrDataMode(base, false);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_SetSeedReg
 * Description   : This function set seed value for new of CRC module computation
 *
 *END**************************************************************************/
void CRC_SetSeedReg(CRC_Type * const base,
		            uint32_t seedValue)
{
    CRC_SetSeedOrDataMode(base, true);
    /* Write a seed - initial checksum */
    CRC_SetDataReg(base, seedValue);
    CRC_SetSeedOrDataMode(base, false);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_GetCrcResult
 * Description   : This function returns the current result of the CRC calculation.
 *
 *END**************************************************************************/
uint32_t CRC_GetCrcResult(const CRC_Type * const base)
{
    crc_bit_width_t width = CRC_GetProtocolWidth(base);
    crc_transpose_t transpose;
    uint32_t result;

    if (width == CRC_BITS_16)
    {
        transpose = CRC_GetReadTranspose(base);
        if ((transpose == CRC_TRANSPOSE_BITS_AND_BYTES) || (transpose == CRC_TRANSPOSE_BYTES))
        {
            /* Returns upper 16 bits of CRC because of transposition in 16-bit mode */
            result = (uint32_t)CRC_GetDataHReg(base);
        }
        else
        {
            result = (uint32_t)CRC_GetDataLReg(base);
        }
    }
    else
    {
        result = CRC_GetDataReg(base);
    }

    return result;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
