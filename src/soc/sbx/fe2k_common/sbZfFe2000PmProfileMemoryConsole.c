/*
 * $Id: sbZfFe2000PmProfileMemoryConsole.c 1.1.34.3 Broadcom SDK $
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
#include <soc/sbx/sbWrappers.h>
#include "sbZfFe2000PmProfileMemoryConsole.hx"



/* Print members in struct */
void
sbZfFe2000PmProfileMemory_Print(sbZfFe2000PmProfileMemory_t *pFromStruct) {
  SB_LOG("Fe2000PmProfileMemory:: resv0=0x%01x", (unsigned int)  pFromStruct->uResv0);
  SB_LOG(" proftype=0x%01x", (unsigned int)  pFromStruct->uProfType);
  SB_LOG(" oamproftype=0x%01x", (unsigned int)  pFromStruct->uOamProfType);
  SB_LOG("\n");

  SB_LOG("Fe2000PmProfileMemory:: resv1=0x%07x", (unsigned int)  pFromStruct->uResv1);
  SB_LOG(" resv2=0x%08x", (unsigned int)  pFromStruct->uResv2);
  SB_LOG("\n");

}

/* SPrint members in struct */
char *
sbZfFe2000PmProfileMemory_SPrint(sbZfFe2000PmProfileMemory_t *pFromStruct, char *pcToString, uint32_t lStrSize) {
  uint32_t WrCnt = 0x0;

  WrCnt += SB_SPRINTF(&pcToString[WrCnt],"Fe2000PmProfileMemory:: resv0=0x%01x", (unsigned int)  pFromStruct->uResv0);
  WrCnt += SB_SPRINTF(&pcToString[WrCnt]," proftype=0x%01x", (unsigned int)  pFromStruct->uProfType);
  WrCnt += SB_SPRINTF(&pcToString[WrCnt]," oamproftype=0x%01x", (unsigned int)  pFromStruct->uOamProfType);
  WrCnt += SB_SPRINTF(&pcToString[WrCnt],"\n");

  WrCnt += SB_SPRINTF(&pcToString[WrCnt],"Fe2000PmProfileMemory:: resv1=0x%07x", (unsigned int)  pFromStruct->uResv1);
  WrCnt += SB_SPRINTF(&pcToString[WrCnt]," resv2=0x%08x", (unsigned int)  pFromStruct->uResv2);
  WrCnt += SB_SPRINTF(&pcToString[WrCnt],"\n");

  /* assert if we've overrun the buffer */
  SB_ASSERT(WrCnt < lStrSize);
  return(pcToString);

}

/* validate members in struct (1 = PASS, 0 = FAIL) */
int
sbZfFe2000PmProfileMemory_Validate(sbZfFe2000PmProfileMemory_t *pZf) {

  if (pZf->uResv0 > 0x1) return 0;
  if (pZf->uProfType > 0x1) return 0;
  if (pZf->uOamProfType > 0x3) return 0;
  if (pZf->uResv1 > 0xfffffff) return 0;
  /* pZf->uResv2 implicitly masked by data type */

  return 1; /* success */

}

/* populate field from string and value */
int
sbZfFe2000PmProfileMemory_SetField(sbZfFe2000PmProfileMemory_t *s, char* name, int value) {

  if (SB_STRCMP(name, "") == 0 ) {
    return -1;
  } else if (SB_STRCMP(name, "uresv0") == 0) {
    s->uResv0 = value;
  } else if (SB_STRCMP(name, "uproftype") == 0) {
    s->uProfType = value;
  } else if (SB_STRCMP(name, "uoamproftype") == 0) {
    s->uOamProfType = value;
  } else if (SB_STRCMP(name, "uresv1") == 0) {
    s->uResv1 = value;
  } else if (SB_STRCMP(name, "uresv2") == 0) {
    s->uResv2 = value;
  } else {
    /* string failed to match any field--ignored */
    return -1;
  }

  return(0);

}
