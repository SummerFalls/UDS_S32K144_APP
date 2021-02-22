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
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 1.3, Taking address of near auto variable.
 * The code is not dynamically linked. An absolute stack address is obtained
 * when taking the address of the near auto variable. A source of error in
 * writing dynamic code is that the stack segment may be different from the data
 * segment.
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
 */

#include "device_registers.h"
#if defined(FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL)
#include "crc_hw_access.h"
#elif defined(FEATURE_CRC_DRIVER_HARD_POLYNOMIAL)
#include "crc_c55_hw_access.h"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Table of base addresses for CRC instances. */
static CRC_Type * const s_crcBase[] = CRC_BASE_PTRS;

/*******************************************************************************
 * Code
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_Init
 * Description   : This function initializes CRC driver based on user configuration input.
 * The user must make sure that the clock is enabled.
 *
 * Implements    : CRC_DRV_Init_Activity
 *END**************************************************************************/
status_t CRC_DRV_Init(uint32_t instance,
                      const crc_user_config_t * userConfigPtr)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    DEV_ASSERT(userConfigPtr != NULL);
    CRC_Type * base = s_crcBase[instance];
    status_t retStatus = STATUS_SUCCESS;

    /* Set the default configuration */
    CRC_Init(base);
    /* Set the CRC configuration */
    retStatus = CRC_DRV_Configure(instance, userConfigPtr);

    return retStatus;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_Deinit
 * Description   : This function sets the default configuration.
 *
 * Implements    : CRC_DRV_Deinit_Activity
 *END**************************************************************************/
status_t CRC_DRV_Deinit(uint32_t instance)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    CRC_Type * base = s_crcBase[instance];

    /* Set the default configuration */
    CRC_Init(base);

    return STATUS_SUCCESS;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_GetCrc32
 * Description   : This function appends 32-bit data to the current CRC calculation
 * and returns new result. If the newSeed is true, seed set and result are calculated
 * from the seed new value (new CRC calculation).
 *
 * Implements    : CRC_DRV_GetCrc32_Activity
 *END**************************************************************************/
uint32_t CRC_DRV_GetCrc32(uint32_t instance,
                          uint32_t data,
                          bool newSeed,
                          uint32_t seed)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    CRC_Type * base = s_crcBase[instance];

    /* If newSeed is true then write a seed to initial checksum */
    if (newSeed)
    {
        /* Write a seed - initial checksum */
        CRC_SetSeedReg(base, seed);
    }

    /* Write 32-bit data */
    CRC_SetDataReg(base, data);

    /* Result of the CRC calculation */
    return CRC_GetCrcResult(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_GetCrc16
 * Description   : This function appends 16-bit data to the current CRC calculation
 * and returns new result. If the newSeed is true, seed set and result are calculated
 * from the seed new value (new CRC calculation).
 *
 * Implements    : CRC_DRV_GetCrc16_Activity
 *END**************************************************************************/
uint32_t CRC_DRV_GetCrc16(uint32_t instance,
                          uint16_t data,
                          bool newSeed,
                          uint32_t seed)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    CRC_Type * base = s_crcBase[instance];

    /* If newSeed is true then write a seed to initial checksum */
    if (newSeed)
    {
        /* Write a seed - initial checksum */
        CRC_SetSeedReg(base, seed);
    }
    /* Write 16-bit data */
    CRC_SetDataLReg(base, data);

    /* Result of the CRC calculation */
    return CRC_GetCrcResult(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_GetCrc8
 * Description   : This function appends 8-bit data to the current CRC calculation
 * and returns new result. If the newSeed is true, seed set and result are calculated
 * from the seed new value (new CRC calculation).
 *
 * Implements    : CRC_DRV_GetCrc8_Activity
 *END**************************************************************************/
uint32_t CRC_DRV_GetCrc8(uint32_t instance,
                         uint8_t data,
                         bool newSeed,
                         uint32_t seed)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    CRC_Type * base = s_crcBase[instance];

    /* If newSeed is true then write a seed to initial checksum */
    if (newSeed)
    {
        /* Write a seed - initial checksum */
        CRC_SetSeedReg(base, seed);
    }
    /* Write 8-bit data */
    CRC_SetDataLLReg(base, data);

    /* Result of the CRC calculation */
    return CRC_GetCrcResult(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_WriteData
 * Description   : This function appends a block of bytes to the current CRC calculation.
 *
 * Implements    : CRC_DRV_WriteData_Activity
 *END**************************************************************************/
void CRC_DRV_WriteData(uint32_t instance,
                       const uint8_t * data,
                       uint32_t dataSize)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    DEV_ASSERT(data != NULL);
    uint32_t i;
    CRC_Type * base = s_crcBase[instance];

    /* 8-bit writes till end of data buffer */
    for (i = 0U; i < dataSize; i++)
    {
        CRC_SetDataLLReg(base, data[i]);
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_GetCrcResult
 * Description   : This function returns the current result of the CRC calculation.
 *
 * Implements    : CRC_DRV_GetCrcResult_Activity
 *END**************************************************************************/
uint32_t CRC_DRV_GetCrcResult(uint32_t instance)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    const CRC_Type * base = s_crcBase[instance];

    /* Result of the CRC calculation */
    return CRC_GetCrcResult(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_Configure
 * Description   : This function configures the CRC module from a user configuration structure.
 *
 * Implements    : CRC_DRV_Configure_Activity
 *END**************************************************************************/
status_t CRC_DRV_Configure(uint32_t instance,
                           const crc_user_config_t * userConfigPtr)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    DEV_ASSERT(userConfigPtr != NULL);
    CRC_Type * base = s_crcBase[instance];

#if defined(FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL)
    /* Set CRC mode */
    CRC_SetProtocolWidth(base, userConfigPtr->crcWidth);
#endif /* FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL */
    /* Set transposes options */
    CRC_SetReadTranspose(base, userConfigPtr->readTranspose);
    /* Set CRC polynomial */
    CRC_SetPolyReg(base, userConfigPtr->polynomial);
    /* Set writes transposes */
    CRC_SetWriteTranspose(base, userConfigPtr->writeTranspose);
    /* Sets complement or inversion checksum */
    CRC_SetFXorMode(base, userConfigPtr->complementChecksum);
    /* Write a seed - initial checksum */
    CRC_SetSeedReg(base, userConfigPtr->seed);

    return STATUS_SUCCESS;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_GetConfig
 * Description   : This function Get configures of the CRC module currently
 *
 * Implements    : CRC_DRV_GetConfig_Activity
 *END**************************************************************************/
status_t CRC_DRV_GetConfig(uint32_t instance,
                           crc_user_config_t * const userConfigPtr)
{
    DEV_ASSERT(instance < CRC_INSTANCE_COUNT);
    DEV_ASSERT(userConfigPtr != NULL);
    const CRC_Type * const base = s_crcBase[instance];

#if defined(FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL)
    /* Gets CRC mode */
    userConfigPtr->crcWidth             = CRC_GetProtocolWidth(base);
#endif /* FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL */
    /* Gets transposes and complement options */
    userConfigPtr->readTranspose        = CRC_GetReadTranspose(base);
    /* Get a polynomial */
    userConfigPtr->polynomial           = CRC_GetPolyReg(base);
    /* Gets transposes options */
    userConfigPtr->writeTranspose       = CRC_GetWriteTranspose(base);
    /* Gets complement or inversion checksum */
    userConfigPtr->complementChecksum   = CRC_GetFXorMode(base);
    /* Get a seed - initial checksum */
    userConfigPtr->seed                 = CRC_GetDataReg(base);

    return STATUS_SUCCESS;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CRC_DRV_GetDefaultConfig
 * Description   : This function Get default configures the CRC module for user configuration structure
 *
 * Implements    : CRC_DRV_GetDefaultConfig_Activity
 *END**************************************************************************/
status_t CRC_DRV_GetDefaultConfig(crc_user_config_t * const userConfigPtr)
{
    DEV_ASSERT(userConfigPtr != NULL);

#if defined(FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL)
    /* Gets CRC mode default is 16 bit */
    userConfigPtr->crcWidth             = FEATURE_CRC_DEFAULT_WIDTH;
#endif /* FEATURE_CRC_DRIVER_SOFT_POLYNOMIAL */
    /* Gets default a polynomial default is reset value */
    userConfigPtr->polynomial           = FEATURE_CRC_DEFAULT_POLYNOMIAL;
    /* Gets default read transposes none */
    userConfigPtr->readTranspose        = FEATURE_CRC_DEFAULT_READ_TRANSPOSE;
    /* Gets default write transpose none */
    userConfigPtr->writeTranspose       = FEATURE_CRC_DEFAULT_WRITE_TRANSPOSE;
    /* Gets default no complement or inversion checksum */
    userConfigPtr->complementChecksum   = false;
    /* Gets default a seed - initial checksum */
    userConfigPtr->seed                 = FEATURE_CRC_DEFAULT_SEED;

    return STATUS_SUCCESS;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
