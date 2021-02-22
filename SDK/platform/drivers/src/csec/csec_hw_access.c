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

#include "csec_hw_access.h"

/**
 * @page misra_violations MISRA-C:2012 violations
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 11.4, Conversion between a pointer and
 * integer type.
 * The cast is required to initialize a pointer with an unsigned long define,
 * representing an address.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 11.6, Cast from unsigned int to pointer.
 * The cast is required to initialize a pointer with an unsigned long define,
 * representing an address.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * The function is defined for use by application code.
 */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_WriteCommandBytes
 * Description   : This function writes command bytes to CSE_PRAM at 32-bit
 * aligned addresses.
 *
 *END**************************************************************************/
void CSEC_WriteCommandBytes(uint8_t offset, const uint8_t *bytes, uint8_t numBytes)
{
    uint8_t i = 0U;

    DEV_ASSERT(bytes != NULL);

    while ((i + 3U) < numBytes)
    {
        CSE_PRAM->RAMn[(offset + i) >> 2U].DATA_32 =
            CSE_PRAM_RAMn_DATA_32_BYTE_0(bytes[i]) |
            CSE_PRAM_RAMn_DATA_32_BYTE_1(bytes[i + 1U]) |
            CSE_PRAM_RAMn_DATA_32_BYTE_2(bytes[i + 2U]) |
            CSE_PRAM_RAMn_DATA_32_BYTE_3(bytes[i + 3U]);
        i = (uint8_t)(i + 4U);
    }
    while (i < numBytes)
    {
        CSEC_WriteCommandByte(offset + i, bytes[i]);
        i++;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_WriteCommandHalfWord
 * Description   : This function writes a command half word to CSE_PRAM at a
 * 16-bit aligned offset.
 *
 *END**************************************************************************/
void CSEC_WriteCommandHalfWord(uint8_t offset, uint16_t halfWord)
{
    uint32_t tmp;

    tmp = CSE_PRAM->RAMn[(offset >> 2U)].DATA_32;

    if ((offset & 2U) != 0U)
    {
        tmp = tmp & ~CSEC_LOWER_HALF_MASK;
        tmp = tmp | ((((uint32_t) halfWord) << CSEC_LOWER_HALF_SHIFT) & CSEC_LOWER_HALF_MASK);
    }
    else
    {
        tmp = tmp & ~CSEC_UPPER_HALF_MASK;
        tmp = tmp | ((((uint32_t) halfWord) << CSEC_UPPER_HALF_SHIFT) & CSEC_UPPER_HALF_MASK);
    }

    CSE_PRAM->RAMn[(offset >> 2U)].DATA_32 = tmp;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_WriteCommandByte
 * Description   : This function writes a single byte to CSE_PRAM.
 *
 *END**************************************************************************/
void CSEC_WriteCommandByte(uint8_t offset, uint8_t byte)
{
    switch (offset & 0x3U)
    {
    case 0x0U:
        CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8HU = byte;
        break;
    case 0x1U:
        CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8HL = byte;
        break;
    case 0x2U:
        CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8LU = byte;
        break;
    case 0x3U:
        CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8LL = byte;
        break;
    default:
        /* Impossible to get here */
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_WriteCommandWords
 * Description   : This function writes command words to CSE_PRAM at a 32-bit
 * aligned offset.
 *
 *END**************************************************************************/
void CSEC_WriteCommandWords(uint8_t offset, const uint32_t *words, uint8_t numWords)
{
    uint8_t i = 0U;
    uint8_t alignedOffset = (uint8_t)(offset >> 2U);

    DEV_ASSERT(words != NULL);

    while (i < numWords)
    {
        CSE_PRAM->RAMn[alignedOffset + i].DATA_32 = words[i];
        i++;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_ReadCommandBytes
 * Description   : This function reads command bytes from CSE_PRAM from a 32-bit
 * aligned offset.
 *
 *END**************************************************************************/
void CSEC_ReadCommandBytes(uint8_t offset, uint8_t *bytes, uint8_t numBytes)
{
    uint8_t i = 0U;

    DEV_ASSERT(bytes != NULL);

    while ((i + 3U) < numBytes)
    {
        uint32_t tmp = CSE_PRAM->RAMn[(offset + i) >> 2U].DATA_32;

        bytes[i]      = (uint8_t)((tmp & CSE_PRAM_RAMn_DATA_32_BYTE_0_MASK) >> CSE_PRAM_RAMn_DATA_32_BYTE_0_SHIFT);
        bytes[i + 1U] = (uint8_t)((tmp & CSE_PRAM_RAMn_DATA_32_BYTE_1_MASK) >> CSE_PRAM_RAMn_DATA_32_BYTE_1_SHIFT);
        bytes[i + 2U] = (uint8_t)((tmp & CSE_PRAM_RAMn_DATA_32_BYTE_2_MASK) >> CSE_PRAM_RAMn_DATA_32_BYTE_2_SHIFT);
        bytes[i + 3U] = (uint8_t)((tmp & CSE_PRAM_RAMn_DATA_32_BYTE_3_MASK) >> CSE_PRAM_RAMn_DATA_32_BYTE_3_SHIFT);
        i = (uint8_t)(i + 4U);
    }
    while (i < numBytes)
    {
        bytes[i] = CSEC_ReadCommandByte(offset + i);
        i++;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_ReadCommandByte
 * Description   : This function reads a single byte from CSE_PRAM.
 *
 *END**************************************************************************/
uint8_t CSEC_ReadCommandByte(uint8_t offset)
{
    uint8_t byte = 0;

    switch (offset & 0x3U)
    {
    case 0x0U:
        byte = CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8HU;
        break;
    case 0x1U:
        byte = CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8HL;
        break;
    case 0x2U:
        byte = CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8LU;
        break;
    case 0x3U:
        byte = CSE_PRAM->RAMn[offset >> 2U].ACCESS8BIT.DATA_8LL;
        break;
    default:
        /* Impossible to get here */
        break;
    }

    return byte;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CSEC_WriteCmdAndWait
 * Description   : This function writes the header of a command and waits for
 * completion.
 *
 *END**************************************************************************/
START_FUNCTION_DEFINITION_RAMSECTION
void CSEC_WriteCmdAndWait(csec_cmd_t funcId,
        csec_func_format_t funcFormat,
        csec_call_sequence_t callSeq,
        csec_key_id_t keyId)
{
    CSE_PRAM->RAMn[0].DATA_32 =
        CSE_PRAM_RAMn_DATA_32_BYTE_0(funcId) |
        CSE_PRAM_RAMn_DATA_32_BYTE_1(funcFormat) |
        CSE_PRAM_RAMn_DATA_32_BYTE_2(callSeq) |
        CSE_PRAM_RAMn_DATA_32_BYTE_3(keyId);

    while ((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) == 0U)
    {
        /* Wait until the CCIF flag is set */
    }
}
END_FUNCTION_DEFINITION_RAMSECTION

/*******************************************************************************
 * EOF
 ******************************************************************************/
