/*
 * $Id: sbZfSbQe2000ElibMVT.c 1.3.36.4 Broadcom SDK $
 * $Copyright: Copyright 2011 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */


#include "sbTypesGlue.h"
#include "sbZfSbQe2000ElibMVT.hx"
#include "sbWrappers.h"
#include <sal/types.h>



/* Pack from struct into array of bytes */
uint32_t
sbZfSbQe2000ElibMVT_Pack(sbZfSbQe2000ElibMVT_t *pFrom,
                         uint8_t *pToData,
                         uint32_t nMaxToDataIndex) {
  int i;
  int size = SB_ZF_SB_QE2000_ELIB_MVT_ENTRY_SIZE_IN_BYTES;

  for ( i=0; i<size; i++ ) {
    (pToData)[i] = 0;
  }
  i = 0;

  /* Pack operation based on little endian */

  /* Pack Member: m_nReserved */
  (pToData)[10] |= ((pFrom)->m_nReserved & 0x07) <<5;

  /* Pack Member: m_nnPortMap */
  (pToData)[4] |= ((pFrom)->m_nnPortMap & 0x1f) <<3;
  (pToData)[5] |= ((pFrom)->m_nnPortMap >> 5) &0xFF;
  (pToData)[6] |= ((pFrom)->m_nnPortMap >> 13) &0xFF;
  (pToData)[7] |= ((pFrom)->m_nnPortMap >> 21) &0xFF;
  (pToData)[8] |= ((pFrom)->m_nnPortMap >> 29) &0xFF;
  (pToData)[9] |= ((pFrom)->m_nnPortMap >> 37) &0xFF;
  (pToData)[10] |= ((pFrom)->m_nnPortMap >> 45) & 0x1f;

  /* Pack Member: m_nMvtda */
  (pToData)[2] |= ((pFrom)->m_nMvtda & 0x07) <<5;
  (pToData)[3] |= ((pFrom)->m_nMvtda >> 3) &0xFF;
  (pToData)[4] |= ((pFrom)->m_nMvtda >> 11) & 0x07;

  /* Pack Member: m_nMvtdb */
  (pToData)[2] |= ((pFrom)->m_nMvtdb & 0x0f) <<1;

  /* Pack Member: m_nNext */
  (pToData)[0] |= ((pFrom)->m_nNext & 0x7f) <<1;
  (pToData)[1] |= ((pFrom)->m_nNext >> 7) &0xFF;
  (pToData)[2] |= ((pFrom)->m_nNext >> 15) & 0x01;

  /* Pack Member: m_nKnockout */
  (pToData)[0] |= ((pFrom)->m_nKnockout & 0x01);

  return SB_ZF_SB_QE2000_ELIB_MVT_ENTRY_SIZE_IN_BYTES;
}




/* Unpack from array of bytes into struct */
void
sbZfSbQe2000ElibMVT_Unpack(sbZfSbQe2000ElibMVT_t *pToStruct,
                           uint8_t *pFromData,
                           uint32_t nMaxToDataIndex) {
  COMPILER_UINT64 tmp;

  (void) tmp;


  /* Unpack operation based on little endian */

  /* Unpack Member: m_nReserved */
  (pToStruct)->m_nReserved =  (uint32_t)  ((pFromData)[10] >> 5) & 0x07;

  /* Unpack Member: m_nnPortMap */
  COMPILER_64_SET((pToStruct)->m_nnPortMap, 0,  (unsigned int) (pFromData)[4]);
  {
    VOL COMPILER_UINT64 *tmp0 = &(pToStruct)->m_nnPortMap;
    COMPILER_64_SET(tmp, 0,  (unsigned int) (pFromData)[5]);
    COMPILER_64_SHL(tmp, 8);
    COMPILER_64_OR(*tmp0, tmp);
  };
  {
    VOL COMPILER_UINT64 *tmp0 = &(pToStruct)->m_nnPortMap;
    COMPILER_64_SET(tmp, 0,  (unsigned int) (pFromData)[6]);
    COMPILER_64_SHL(tmp, 16);
    COMPILER_64_OR(*tmp0, tmp);
  };
  {
    VOL COMPILER_UINT64 *tmp0 = &(pToStruct)->m_nnPortMap;
    COMPILER_64_SET(tmp, 0,  (unsigned int) (pFromData)[7]);
    COMPILER_64_SHL(tmp, 24);
    COMPILER_64_OR(*tmp0, tmp);
  };
  {
    VOL COMPILER_UINT64 *tmp0 = &(pToStruct)->m_nnPortMap;
    COMPILER_64_SET(tmp, 0,  (unsigned int) (pFromData)[8]);
    COMPILER_64_SHL(tmp, 32);
    COMPILER_64_OR(*tmp0, tmp);
  };
  {
    VOL COMPILER_UINT64 *tmp0 = &(pToStruct)->m_nnPortMap;
    COMPILER_64_SET(tmp, 0,  (unsigned int) (pFromData)[9]);
    COMPILER_64_SHL(tmp, 40);
    COMPILER_64_OR(*tmp0, tmp);
  };
  {
    VOL COMPILER_UINT64 *tmp0 = &(pToStruct)->m_nnPortMap;
    COMPILER_64_SET(tmp, 0,  (unsigned int) (pFromData)[10]);
    COMPILER_64_SHL(tmp, 48);
    COMPILER_64_OR(*tmp0, tmp);
  };

  /* Unpack Member: m_nMvtda */
  (pToStruct)->m_nMvtda =  (uint32_t)  ((pFromData)[2] >> 5) & 0x07;
  (pToStruct)->m_nMvtda |=  (uint32_t)  (pFromData)[3] << 3;
  (pToStruct)->m_nMvtda |=  (uint32_t)  ((pFromData)[4] & 0x07) << 11;

  /* Unpack Member: m_nMvtdb */
  (pToStruct)->m_nMvtdb =  (uint32_t)  ((pFromData)[2] >> 1) & 0x0f;

  /* Unpack Member: m_nNext */
  (pToStruct)->m_nNext =  (uint32_t)  ((pFromData)[0] >> 1) & 0x7f;
  (pToStruct)->m_nNext |=  (uint32_t)  (pFromData)[1] << 7;
  (pToStruct)->m_nNext |=  (uint32_t)  ((pFromData)[2] & 0x01) << 15;

  /* Unpack Member: m_nKnockout */
  (pToStruct)->m_nKnockout =  (uint32_t)  ((pFromData)[0] ) & 0x01;

}



/* initialize an instance of this zframe */
void
sbZfSbQe2000ElibMVT_InitInstance(sbZfSbQe2000ElibMVT_t *pFrame) {

  pFrame->m_nReserved =  (unsigned int)  0;
  pFrame->m_nnPortMap =  (uint64_t)  0;
  pFrame->m_nMvtda =  (unsigned int)  0;
  pFrame->m_nMvtdb =  (unsigned int)  0;
  pFrame->m_nNext =  (unsigned int)  0;
  pFrame->m_nKnockout =  (unsigned int)  0;

}