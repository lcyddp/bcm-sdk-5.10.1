/*  -*-  Mode:C; c-basic-offset:4 -*- */
/*
 * $Id: sbZfFe2000PpInitialQueueStateEntry.hx 1.3.36.4 Broadcom SDK $
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


#ifndef SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_H
#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_H

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_SIZE_IN_BYTES 9
#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_SIZE 9
#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_M_UHEADERSHIFT_BITS "65:60"
#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_M_UHEADERTYPE_BITS "59:56"
#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_M_USTATE_BITS "55:32"
#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_M_UVARIABLE_BITS "31:0"


typedef struct _sbZfFe2000PpInitialQueueStateEntry {
  uint32_t m_uHeaderShift;
  uint32_t m_uHeaderType;
  uint32_t m_uState;
  uint32_t m_uVariable;
} sbZfFe2000PpInitialQueueStateEntry_t;

uint32_t
sbZfFe2000PpInitialQueueStateEntry_Pack(sbZfFe2000PpInitialQueueStateEntry_t *pFrom,
                                        uint8_t *pToData,
                                        uint32_t nMaxToDataIndex);
void
sbZfFe2000PpInitialQueueStateEntry_Unpack(sbZfFe2000PpInitialQueueStateEntry_t *pToStruct,
                                          uint8_t *pFromData,
                                          uint32_t nMaxToDataIndex);
void
sbZfFe2000PpInitialQueueStateEntry_InitInstance(sbZfFe2000PpInitialQueueStateEntry_t *pFrame);

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_SET_HEADERSHIFT(nFromData,pToData,nMaxToDataIndex) \
          do { \
           (pToData)[7] = ((pToData)[7] & ~(0x0f << 4)) | (((nFromData) & 0x0f) << 4); \
           (pToData)[8] = ((pToData)[8] & ~ 0x03) | (((nFromData) >> 4) & 0x03); \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_SET_HEADERTYPE(nFromData,pToData,nMaxToDataIndex) \
          do { \
           (pToData)[7] = ((pToData)[7] & ~0x0f) | ((nFromData) & 0x0f); \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_SET_STATE(nFromData,pToData,nMaxToDataIndex) \
          do { \
           (pToData)[4] = ((nFromData)) & 0xFF; \
           (pToData)[5] = ((pToData)[5] & ~0xFF) | (((nFromData) >> 8) & 0xFF); \
           (pToData)[6] = ((pToData)[6] & ~0xFF) | (((nFromData) >> 16) & 0xFF); \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_SET_VARIABLE(nFromData,pToData,nMaxToDataIndex) \
          do { \
           (pToData)[0] = ((nFromData)) & 0xFF; \
           (pToData)[1] = ((pToData)[1] & ~0xFF) | (((nFromData) >> 8) & 0xFF); \
           (pToData)[2] = ((pToData)[2] & ~0xFF) | (((nFromData) >> 16) & 0xFF); \
           (pToData)[3] = ((pToData)[3] & ~0xFF) | (((nFromData) >> 24) & 0xFF); \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_GET_HEADERSHIFT(nToData,pFromData,nMaxFromDataIndex) \
          do { \
           (nToData) = (uint32_t) ((pFromData)[7] >> 4) & 0x0f; \
           (nToData) |= (uint32_t) ((pFromData)[8] & 0x03) << 4; \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_GET_HEADERTYPE(nToData,pFromData,nMaxFromDataIndex) \
          do { \
           (nToData) = (uint32_t) ((pFromData)[7]) & 0x0f; \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_GET_STATE(nToData,pFromData,nMaxFromDataIndex) \
          do { \
           (nToData) = (uint32_t) (pFromData)[4] ; \
           (nToData) |= (uint32_t) (pFromData)[5] << 8; \
           (nToData) |= (uint32_t) (pFromData)[6] << 16; \
          } while(0)

#define SB_ZF_FE2000PPINITIALQUEUESTATEENTRY_GET_VARIABLE(nToData,pFromData,nMaxFromDataIndex) \
          do { \
           (nToData) = (uint32_t) (pFromData)[0] ; \
           (nToData) |= (uint32_t) (pFromData)[1] << 8; \
           (nToData) |= (uint32_t) (pFromData)[2] << 16; \
           (nToData) |= (uint32_t) (pFromData)[3] << 24; \
          } while(0)

#endif
