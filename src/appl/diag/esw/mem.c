/*
 * $Id: mem.c 1.52.2.2 Broadcom SDK $
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
 *
 * socdiag memory commands
 */

#include <sal/core/libc.h>
#include <soc/mem.h>
#include <soc/debug.h>

#include <soc/error.h>
#include <soc/l2x.h>
#include <soc/l3x.h>
#ifdef BCM_EASYRIDER_SUPPORT
#include <soc/easyrider.h>
#endif
#include <appl/diag/system.h>

#ifdef __KERNEL__
#define atoi _shr_ctoi
#endif

/*
 * Utility routine to concatenate the first argument ("first"), with
 * the remaining arguments, with commas separating them.
 */

static void
collect_comma_args(args_t *a, char *valstr, char *first)
{
    char *s;

    sal_strcpy(valstr, first);

    while ((s = ARG_GET(a)) != 0) {
	strcat(valstr, ",");
	strcat(valstr, s);
    }
}

static void
check_global(int unit, soc_mem_t mem, char *s, int *is_global)
{
    soc_field_info_t	*fld;
    soc_mem_info_t	*m = &SOC_MEM_INFO(unit, mem);
    char                *eqpos;
    
    eqpos = strchr(s, '=');
    if (eqpos != NULL) {
        *eqpos++ = 0;
    }
    for (fld = &m->fields[0]; fld < &m->fields[m->nFields]; fld++) {
        if (!sal_strcasecmp(s, SOC_FIELD_NAME(unit, fld->field)) &&
            (fld->flags & SOCF_GLOBAL)) {
    	    break;
        }
    }
    if (fld == &m->fields[m->nFields]) {
        *is_global = 0;
    } else {
        *is_global = 1;
    }
}

static int
collect_comma_args_with_view(args_t *a, char *valstr, char *first, 
                             char *view, int unit, soc_mem_t mem)
{
    char *s, *s_copy = NULL, *f_copy = NULL;
    int is_global, rv = 0;

    if ((f_copy = sal_alloc(strlen(first) + 1, "first")) == NULL) {
        printk("cmd_esw_mem_write : Out of memory\n");
        rv = -1;
        goto done;
    }
    memset(f_copy, 0, strlen(first) + 1);
    sal_strcpy(f_copy, first);

    /* Check if field is global before applying view prefix */
    check_global(unit, mem, f_copy, &is_global);
    if (!is_global) {
        sal_strcpy(valstr, view);
        strcat(valstr, first);
    } else {
        sal_strcpy(valstr, first);
    }

    while ((s = ARG_GET(a)) != 0) {
        if ((s_copy = sal_alloc(strlen(s) + 1, "s_copy")) == NULL) {
            printk("cmd_esw_mem_write : Out of memory\n");
            rv = -1;
            goto done;
        }
        memset(s_copy, 0, strlen(s) + 1);
        sal_strcpy(s_copy, s);
        check_global(unit, mem, s_copy, &is_global);
        sal_free(s_copy);
        strcat(valstr, ",");
        if (!is_global) {
            strcat(valstr, view);
            strcat(valstr, s);
        } else {
            strcat(valstr, s);
        }
    }
done:
    if (f_copy != NULL) {
        sal_free(f_copy);
    }
    return rv;
}

/*
 * modify_mem_fields
 *
 *   Verify similar to modify_reg_fields (see reg.c) but works on
 *   memory table entries instead of register values.  Handles fields
 *   of any length.
 *
 *   If mask is non-NULL, it receives an entry which is a mask of all
 *   fields modified.
 *
 *   Values may be specified with optional increment or decrement
 *   amounts; for example, a MAC address could be 0x1234+2 or 0x1234-1
 *   to specify an increment of +2 or -1, respectively.
 *
 *   If incr is FALSE, the increment is ignored and the plain value is
 *   stored in the field (e.g. 0x1234).
 *
 *   If incr is TRUE, the value portion is ignored.  Instead, the
 *   increment value is added to the existing value of the field.  The
 *   field value wraps around on overflow.
 *
 *   Returns -1 on failure, 0 on success.
 */

static int
modify_mem_fields(int unit, soc_mem_t mem, uint32 *entry,
		  uint32 *mask, char *mod, int incr)
{
    soc_field_info_t	*fld;
    char		*fmod, *fval, *s;
    char		*modstr = NULL;
    uint32		fvalue[SOC_MAX_MEM_FIELD_WORDS];
    uint32		fincr[SOC_MAX_MEM_FIELD_WORDS];
    int			i, entry_dw;
    soc_mem_info_t	*m = &SOC_MEM_INFO(unit, mem);

    entry_dw = BYTES2WORDS(m->bytes);

    if ((modstr = sal_alloc(ARGS_BUFFER, "modify_mem")) == NULL) {
        printk("modify_mem_fields: Out of memory\n");
        return CMD_FAIL;
    }

    strncpy(modstr, mod, ARGS_BUFFER);/* Don't destroy input string */
    modstr[ARGS_BUFFER - 1] = 0;
    mod = modstr;

    if (mask) {
	memset(mask, 0, entry_dw * 4);
    }

    while ((fmod = strtok(mod, ",")) != 0) {
	mod = NULL;			/* Pass strtok NULL next time */
	fval = strchr(fmod, '=');
	if (fval != NULL) {		/* Point fval to arg, NULL if none */
	    *fval++ = 0;		/* Now fmod holds only field name. */
	}
	if (fmod[0] == 0) {
	    printk("Null field name\n");
            sal_free(modstr);
	    return -1;
	}
	if (!sal_strcasecmp(fmod, "clear")) {
	    memset(entry, 0, entry_dw * sizeof (*entry));
	    if (mask) {
		memset(mask, 0xff, entry_dw * sizeof (*entry));
	    }
	    continue;
	}
	for (fld = &m->fields[0]; fld < &m->fields[m->nFields]; fld++) {
	    if (!sal_strcasecmp(fmod, SOC_FIELD_NAME(unit, fld->field))) {
		break;
	    }
	}
	if (fld == &m->fields[m->nFields]) {
	    printk("No such field \"%s\" in memory \"%s\".\n",
		   fmod, SOC_MEM_UFNAME(unit, mem));
            sal_free(modstr);
	    return -1;
	}
	if (!fval) {
	    printk("Missing %d-bit value to assign to \"%s\" field \"%s\".\n",
		   fld->len,
		   SOC_MEM_UFNAME(unit, mem), SOC_FIELD_NAME(unit, fld->field));
            sal_free(modstr);
	    return -1;
	}
	s = strchr(fval, '+');
	if (s == NULL) {
	    s = strchr(fval, '-');
	}
	if (s == fval) {
	    s = NULL;
	}
	if (incr) {
	    if (s != NULL) {
		parse_long_integer(fincr, SOC_MAX_MEM_FIELD_WORDS,
				   s[1] ? &s[1] : "1");
		if (*s == '-') {
		    neg_long_integer(fincr, SOC_MAX_MEM_FIELD_WORDS);
		}
		if (fld->len & 31) {
		    /* Proper treatment of sign extension */
		    fincr[fld->len / 32] &= ~(0xffffffff << (fld->len & 31));
		}
		soc_mem_field_get(unit, mem, entry, fld->field, fvalue);
		add_long_integer(fvalue, fincr, SOC_MAX_MEM_FIELD_WORDS);
		if (fld->len & 31) {
		    /* Proper treatment of sign extension */
		    fvalue[fld->len / 32] &= ~(0xffffffff << (fld->len & 31));
		}
		soc_mem_field_set(unit, mem, entry, fld->field, fvalue);
	    }
	} else {
	    if (s != NULL) {
		*s = 0;
	    }
	    parse_long_integer(fvalue, SOC_MAX_MEM_FIELD_WORDS, fval);
	    for (i = fld->len; i < SOC_MAX_MEM_FIELD_BITS; i++) {
		if (fvalue[i / 32] & 1 << (i & 31)) {
		    printk("Value \"%s\" too large for "
			   "%d-bit field \"%s\".\n",
			   fval, fld->len, SOC_FIELD_NAME(unit, fld->field));
                    sal_free(modstr);
		    return -1;
		}
	    }
	    soc_mem_field_set(unit, mem, entry, fld->field, fvalue);
	}
	if (mask) {
	    memset(fvalue, 0, sizeof (fvalue));
	    for (i = 0; i < fld->len; i++) {
		fvalue[i / 32] |= 1 << (i & 31);
	    }
	    soc_mem_field_set(unit, mem, mask, fld->field, fvalue);
	}
    }

    sal_free(modstr);
    return 0;
}

/*
 * mmudebug command
 *    No argument: print current state
 *    Argument is 1: enable
 *    Argument is 0: disable
 */

char mmudebug_usage[] =
    "Parameters: [on | off]\n\t"
    "Puts the MMU in debug mode (on) or normal mode (off).\n\t"
    "With no parameter, displays the current mode.\n\t"
    "MMU must be in debug mode to access CBP memories.\n";

cmd_result_t
mem_mmudebug(int unit, args_t *a)
{
    char *s = ARG_GET(a);

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (s) {
	if (!sal_strcasecmp(s, "on")) {
	    printk("Entering debug mode ...\n");
	    soc_mem_debug_set(unit, 1);
	} else if (!sal_strcasecmp(s, "off")) {
	    printk("Leaving debug mode ...\n");
	    soc_mem_debug_set(unit, 0);
	} else
	    return CMD_USAGE;
    } else {
	int enable = 0;
	soc_mem_debug_get(unit, &enable);
	printk("MMU debug mode is %s\n", enable ? "on" : "off");
    }

    return CMD_OK;
}

/*
 * Initialize Cell Free Address pool.
 */

char cfapinit_usage[] =
    "Parameters: none\n\t"
    "Run diags on CFAP pool and initialize CFAP with good entries only\n";

cmd_result_t
mem_cfapinit(int unit, args_t *a)
{
    int r;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((r = soc_mem_cfap_init(unit)) < 0) {
	printk("NOTICE: error initializing CFAP: %s\n", soc_errmsg(r));
	return CMD_FAIL;
    }

    return CMD_OK;
}

#ifdef BCM_HERCULES_SUPPORT
/*
 * Initialize Cell Free Address pool.
 */

char llainit_usage[] =
    "Parameters: [force]\n\t"
    "Run diags on LLA and PP and initialize LLA with good entries only\n";

cmd_result_t
mem_llainit(int unit, args_t *a)
{
    char *argforce;
    int r, force = FALSE;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((argforce = ARG_GET(a)) != NULL) {
        if (!sal_strcasecmp(argforce, "force")) {
            force = TRUE;
        } else {
            return CMD_USAGE;
        }
    }

    if (force) {
        if (SOC_CONTROL(unit)->lla_map != NULL) {
            int i;
	    PBMP_PORT_ITER(unit, i) {
                if (SOC_CONTROL(unit)->lla_map[i] != NULL) {
                    sal_free(SOC_CONTROL(unit)->lla_map[i]);
                    SOC_CONTROL(unit)->lla_map[i] = NULL;
                }
            }
        }
    }

    if ((r = soc_mem_lla_init(unit)) < 0) {
        printk("NOTICE: error initializing LLA: %s\n", soc_errmsg(r));
        return CMD_FAIL;
    }

    return CMD_OK;
}
#endif

static int
parse_dwords(int count, uint32 *dw, args_t *a)
{
    char	*s;
    int		i;

    for (i = 0; i < count; i++) {
	if ((s = ARG_GET(a)) == NULL) {
	    printk("Not enough data values (have %d, need %d)\n",
		   i, count);
	    return -1;
	}

	dw[i] = parse_integer(s);
    }

    if (ARG_CNT(a) > 0) {
	printk("Ignoring extra data on command line "
	       "(only %d words needed)\n",
	       count);
    }

    return 0;
}

#ifdef BCM_EASYRIDER_SUPPORT

STATIC cmd_result_t
_mem_write_external_cells(int unit, args_t *a, soc_mem_t mem,
                          int copyno, int index, int count)
{
    soc_reg_t          cmd_reg;
    uint32             cmd_data;
    soc_er_cd_chunk_t    chunk_data = NULL;
    soc_er_cd_slice_t    slice_data = NULL;
    soc_er_cd_columns_t  columns_data = NULL;
    int                idx, chunk, slice;
    int		       i, r, hex, column, mem_size, rv;
    char               *s, *chunk_str[SOC_ER_CELLDATA_CHUNKS],
                       *slice_str[SOC_ER_CELLDATA_SLICES];

    char               *column_str[SOC_ER_CELLDATA_SLICE_COLUMNS][SOC_ER_CELLDATA_SLICES];
    char               parse_name[SOC_ER_CELLDATA_SLICES][10];
    /*char               col_parse_name[SOC_ER_CELLDATA_SLICE_COLUMNS][SOC_ER_CELLDATA_SLICES][15];*/
    char                ***col_parse_name = NULL;
    soc_field_info_t   *fld;
    parse_table_t      pt;

    fld = &(SOC_MEM_INFO(unit, C0_CPU_WQm).fields[0]); /* Only one field */
    cmd_reg = (mem == C0_CELLm) ? MCU_CHN0_REQ_CMDr : MCU_CHN1_REQ_CMDr;

    rv = CMD_OK;

    mem_size = sizeof(char **) * SOC_ER_CELLDATA_SLICE_COLUMNS;
    col_parse_name = sal_alloc(mem_size, "col_parse_name");
    if (NULL == col_parse_name) {
        rv = CMD_FAIL;
        goto exit_cleanup;
    }
    sal_memset(col_parse_name, 0, mem_size);
    for (i = 0; i < SOC_ER_CELLDATA_SLICE_COLUMNS; i++) {
        mem_size = sizeof(char *) * SOC_ER_CELLDATA_SLICES;
        col_parse_name[i] = sal_alloc(mem_size, "col_parse_name");
        if (NULL == col_parse_name[i]) {
            rv = CMD_FAIL;
            goto exit_cleanup;
        }
        sal_memset(col_parse_name[i], 0, mem_size);
        for (r = 0; r < SOC_ER_CELLDATA_SLICES; r++) {
            mem_size = sizeof(char) * 15;
            col_parse_name[i][r] = sal_alloc(mem_size, "col_parse_name");
            if (NULL == col_parse_name[i][r]) {
                rv = CMD_FAIL;
                goto exit_cleanup;
            }
            sal_memset(col_parse_name[i][r], 0, mem_size);
        }
    }

    if (soc_er_dual_dimensional_array_alloc(&chunk_data, SOC_ER_CELLDATA_CHUNKS,
                                            SOC_MAX_MEM_WORDS) < 0) {
        printk ("Unable to allocate memory for chunk_data \n");
        rv = CMD_FAIL;
        goto exit_cleanup;
    }

    if (soc_er_dual_dimensional_array_alloc(&slice_data, SOC_ER_CELLDATA_SLICES,
                                            SOC_ER_CELLDATA_SLICE_WORDS) < 0) {
        printk ("Unable to allocate memory for slice_data \n");
        rv = CMD_FAIL;
        goto exit_cleanup;
    }

    if (soc_er_dual_dimensional_array_alloc(&columns_data, SOC_ER_CELLDATA_SLICES,
                                            SOC_ER_CELLDATA_SLICE_COLUMNS) < 0) {
        printk ("Unable to allocate memory for columns_data \n");
        rv = CMD_FAIL;
        goto exit_cleanup;
    }


    s = ARG_GET(a);
    if (s != NULL &&
        ((sal_strcasecmp(s, "raw") == 0) || (sal_strcasecmp(s, "hex") == 0))) {
        hex = (sal_strcasecmp(s, "hex") == 0);
        parse_table_init(unit, &pt);

        for (i = 0; i < SOC_ER_CELLDATA_SLICES; i++) {
            if (hex) {
                for (column = 0; column < SOC_ER_CELLDATA_SLICE_COLUMNS;
                     column++) {
                    sal_sprintf(col_parse_name[column][i],
                            "Column%d.%d", column, i);
                    parse_table_add(&pt, col_parse_name[column][i],
                                    PQ_STRING, 0, &column_str[column][i], 0);
                }
            } else {
                sal_sprintf(parse_name[i], "Slice%d", i);
                parse_table_add(&pt, parse_name[i], PQ_STRING, 0,
                                &(slice_str[i]), 0);
            }
        }

        if (parse_arg_eq(a, &pt) < 0) {
            printk("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            rv = (CMD_FAIL);
            goto exit_cleanup;
        }

        for (slice = 0; slice < SOC_ER_CELLDATA_SLICES; slice++) {
            if (hex) {
                for (column = 0; column < SOC_ER_CELLDATA_SLICE_COLUMNS;
                     column++) {
                    if (*column_str[column][slice]) {
                        columns_data[slice][column] =
                            parse_integer(column_str[column][slice]);
                        if ((columns_data[slice][column] & 0xfffc0000) != 0) {
                            printk("Value \"%s\" greater than 18 bits\n",
                                   column_str[column][slice]);
                            parse_arg_eq_done(&pt);
                            rv = (CMD_FAIL);
                            goto exit_cleanup;
                        }
                    } else {
                        columns_data[slice][column] =0;
                    }
                }
            } else {
                if (*slice_str[slice]) {
                    parse_long_integer(slice_data[slice],
                                       SOC_MAX_MEM_FIELD_WORDS,
                                       slice_str[slice]);
                    for (i = 72; i < SOC_MAX_MEM_FIELD_BITS; i++) {
                        int _index =  i / 32;
                        
                        if (_index >= SOC_ER_CELLDATA_SLICE_WORDS) {
                            break;
                        }
                        if (slice_data[slice][_index] & 1 << (i & 31)) {
                            printk("Value \"%s\" greater than 72 bits\n",
                                   slice_str[slice]);
                            parse_arg_eq_done(&pt);
                            rv = (CMD_FAIL);
                            goto exit_cleanup;
                        }
                    }
                } else {
                    sal_memset(slice_data[slice], 0,
                               WORDS2BYTES(SOC_ER_CELLDATA_SLICE_WORDS));
                }
            }

        }
        
        if (hex) {
            soc_er_celldata_columns_to_slice(columns_data, slice_data);
        }
        soc_er_celldata_slice_to_chunk(slice_data, chunk_data);
    } else {
        ARG_PREV(a); /* Back up one argument */
        parse_table_init(unit, &pt);
        for (i = 0; i < SOC_ER_CELLDATA_CHUNKS; i++) {
            sal_sprintf(parse_name[i], "Chunk%d", i);
            parse_table_add(&pt, parse_name[i], PQ_STRING, 0,
                            &(chunk_str[i]), 0);
        }

        if (parse_arg_eq(a, &pt) < 0) {
            printk("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            rv = (CMD_FAIL);
            goto exit_cleanup;
        }

        for (chunk = 0; chunk < SOC_ER_CELLDATA_CHUNKS; chunk++) {
            if (*chunk_str[chunk]) {
                parse_long_integer(chunk_data[chunk], SOC_MAX_MEM_FIELD_WORDS,
                                   chunk_str[chunk]);
                for (i = fld->len; i < SOC_MAX_MEM_FIELD_BITS; i++) {
                    if (chunk_data[chunk][i / 32] & 1 << (i & 31)) {
                        printk("Value \"%s\" too large for "
                               "%d-bit field \"%s\".\n",
                               chunk_str[chunk], fld->len,
                               SOC_FIELD_NAME(unit, fld->field));
                        parse_arg_eq_done(&pt);
                        rv = (CMD_FAIL);
                        goto exit_cleanup;
                    }
                }
            } else {
                sal_memset(chunk_data[chunk], 0,
                           WORDS2BYTES(SOC_ER_CELLDATA_CHUNK_WORDS));
            }
        }
    }

    /* Don't need strings anymore */
    parse_arg_eq_done(&pt);

    /* Load up write buffer once */
    /* Write mem */
    if ((r = soc_er_celldata_chunk_write(unit, mem, 
                                         index, chunk_data)) < 0) {
        printk("Failed to write memory %s[%d]: %s\n",
               SOC_MEM_NAME(unit, mem), index, soc_errmsg(r));
        rv = (CMD_FAIL);
        goto exit_cleanup;
    }

    cmd_data = 0;
    /* Write data code */
    soc_reg_field_set(unit, cmd_reg, &cmd_data, COMMANDf, 6);
    
    /*
     * Now just write the same data repeatedly
     * Buffer still filled from write above
     */
    for (idx = index + 1; idx < index + count; idx++) {
        soc_reg_field_set(unit, cmd_reg, &cmd_data, DDR_ADDRf, idx);
        if ((r = soc_reg32_set(unit, cmd_reg,
                                 REG_PORT_ANY, 0, cmd_data)) < 0) {
	    printk("Failed to write command register %s: %s\n",
                   SOC_REG_NAME(unit, cmd_reg), soc_errmsg(r));
            rv = (CMD_FAIL);
            goto exit_cleanup;
        }
    }


exit_cleanup:
    
    if (NULL != col_parse_name) {
        for (i = 0; i < SOC_ER_CELLDATA_SLICE_COLUMNS; i++) {
            if (NULL != col_parse_name[i]) {
                for (r = 0; r < SOC_ER_CELLDATA_SLICES; r++) {
                    if (NULL != col_parse_name[i][r]) {
                        sal_free(col_parse_name[i][r]);
                    }
                }
                sal_free(col_parse_name[i]);
            }
        }
        sal_free(col_parse_name);
    }
    if (NULL != chunk_data) {
        soc_er_dual_dimensional_array_free(chunk_data, SOC_ER_CELLDATA_CHUNKS); 
    }
    if (NULL != slice_data) {
        soc_er_dual_dimensional_array_free(slice_data, SOC_ER_CELLDATA_SLICES); 
    }
    if (NULL != columns_data) {
        soc_er_dual_dimensional_array_free(columns_data, SOC_ER_CELLDATA_SLICES);
    }
    return rv;
}
#endif

cmd_result_t
cmd_esw_mem_write(int unit, args_t *a)
{
    int			i, index, start, count, copyno;
    char		*tab, *idx, *cnt, *s, *memname, *slam_buf = NULL;
    soc_mem_t		mem;
    uint32		entry[SOC_MAX_MEM_WORDS];
    int			entry_dw, view_len, entry_bytes;
    char		copyno_str[8];
    int			r, update;
    int			rv = CMD_FAIL;
    char		*valstr = NULL, *view = NULL;
    int			no_cache = 0;
    int                 use_slam = 0;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    tab = ARG_GET(a);
    if (tab != NULL && sal_strcasecmp(tab, "nocache") == 0) {
	no_cache = 1;
	tab = ARG_GET(a);
    }
    idx = ARG_GET(a);
    cnt = ARG_GET(a);
    s = ARG_GET(a);

    /* you will need at least one value and all the args .. */
    if (!tab || !idx || !cnt || !s || !isint(cnt)) {
	return CMD_USAGE;
    }

    /* Deal with VIEW:MEMORY if applicable */
    memname = strstr(tab, ":");
    view_len = 0;
    if (memname != NULL) {
        memname++;
        view_len = memname - tab;
    } else {
        memname = tab;
    }

    if (parse_memory_name(unit, &mem, memname, &copyno) < 0) {
	printk("ERROR: unknown table \"%s\"\n",tab);
	goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }

#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (soc_mem_is_readonly(unit, mem)) {
	debugk(DK_ERR, "ERROR: Table %s is read-only\n",
               SOC_MEM_UFNAME(unit, mem));
	goto done;
    }

    start = parse_memory_index(unit, mem, idx);
    count = parse_integer(cnt);

    if (copyno == COPYNO_ALL) {
	copyno_str[0] = 0;
    } else {
	sal_sprintf(copyno_str, ".%d", copyno);
    }

#ifdef BCM_EASYRIDER_SUPPORT
    if (SOC_IS_EASYRIDER(unit) && ((mem == C0_CELLm) || (mem == C1_CELLm))) {
        ARG_PREV(a); /* Back up one argument for next call */
        return _mem_write_external_cells(unit, a, mem, copyno, start, count);
    }
#endif
    entry_dw = soc_mem_entry_words(unit, mem);

    if ((valstr = sal_alloc(ARGS_BUFFER, "reg_set")) == NULL) {
        printk("cmd_esw_mem_write : Out of memory\n");
        goto done;
    }

    /*
     * If a list of fields were specified, generate the entry from them.
     * Otherwise, generate it by reading raw dwords from command line.
     */

    if (!isint(s)) {
	/* List of fields */

	if (view_len == 0) {
            collect_comma_args(a, valstr, s);
        } else {
            if ((view = sal_alloc(view_len + 1, "view_name")) == NULL) {
                printk("cmd_esw_mem_write : Out of memory\n");
                goto done;
            }
            memset(view, 0, view_len + 1);
            memcpy(view, tab, view_len);
            if (collect_comma_args_with_view(a, valstr, s, view, unit, mem) < 0) {
                printk("Out of memory: aborted\n");
                goto done;
            }
        }

	memset(entry, 0, sizeof (entry));

	if (modify_mem_fields(unit, mem, entry, NULL, valstr, FALSE) < 0) {
	    printk("Syntax error: aborted\n");
	    goto done;
	}

	update = TRUE;
    } else {
	/* List of numeric values */

	ARG_PREV(a);

	if (parse_dwords(entry_dw, entry, a) < 0) {
	    goto done;
	}

	update = FALSE;
    }

    if (debugk_check(DK_SOCMEM)) {
	printk("WRITE[%s%s], DATA:", SOC_MEM_UFNAME(unit, mem), copyno_str);
	for (i = 0; i < entry_dw; i++) {
	    printk(" 0x%x", entry[i]);
	}
	printk("\n");
    }

    /*
     * Created entry, now write it
     */

#ifdef BCM_EASYRIDER_SUPPORT
    if (SOC_IS_EASYRIDER(unit) && (mem == FP_EXTERNALm)) {
        for (index = start; index < start + count; index++) {
            if ((r = soc_er_fp_ext_write(unit, index,
                                         (void *)entry)) < 0) {
                printk("Write ERROR: table %s.%s[%d]: %s\n",
                       SOC_MEM_UFNAME(unit, mem), copyno_str,
                       index, soc_errmsg(r));
                goto done;
            }
        }
        rv = CMD_OK;
        goto done;
    } else if (SOC_IS_EASYRIDER(unit) && (mem == FP_TCAM_EXTERNALm)) {
        for (index = start; index < start + count; index++) {
            if ((r = soc_er_fp_tcam_ext_write(unit, index,
                                              (void *)entry)) < 0) {
                printk("Write ERROR: table %s.%s[%d]: %s\n",
                       SOC_MEM_UFNAME(unit, mem), copyno_str,
                       index, soc_errmsg(r));
                goto done;
            }
        }
        rv = CMD_OK;
        goto done;
    }
#endif

    use_slam = soc_property_get(unit, spn_DIAG_SHELL_USE_SLAM, 0);
    if (use_slam && count > 1) {
        entry_bytes = soc_mem_entry_bytes(unit, mem);
        slam_buf = soc_cm_salloc(unit, count * entry_bytes, "slam_entry");
        if (slam_buf == NULL) {
            printk("cmd_esw_mem_write : Out of memory\n");
            goto done;
        }
        for (i = 0; i < count; i++) {
            sal_memcpy(slam_buf + i * entry_bytes, entry, entry_bytes);
        }
        r = soc_mem_write_range(unit, mem, copyno, start, start + count - 1, slam_buf);
        soc_cm_sfree(unit, slam_buf);
        if (r < 0) {
            printk("Slam ERROR: table %s.%s: %s\n",
                   SOC_MEM_UFNAME(unit, mem), copyno_str, soc_errmsg(r));
            goto done;
        }
        for (index = start; index < start + count; index++) {
	    if (update) {
	        modify_mem_fields(unit, mem, entry, NULL, valstr, TRUE);
	    }
        }
    } else {
        for (index = start; index < start + count; index++) {
       	    if ((r = soc_mem_write(unit, mem, copyno,
        	                   no_cache ? -index : index, entry)) < 0) {
	        printk("Write ERROR: table %s.%s[%d]: %s\n",
		       SOC_MEM_UFNAME(unit, mem), copyno_str,
		       index, soc_errmsg(r));
	        goto done;
	    }

	    if (update) {
	        modify_mem_fields(unit, mem, entry, NULL, valstr, TRUE);
	    }
        }
    }
    rv = CMD_OK;

 done:
    if (valstr != NULL) {
       sal_free(valstr);
    }
    if (view != NULL) {
       sal_free(view);
    }
    return rv;
}


/*
 * Modify the fields of a table entry
 */
cmd_result_t
cmd_esw_mem_modify(int unit, args_t *a)
{
    int			index, start, count, copyno, i, view_len;
    char		*tab, *idx, *cnt, *s, *memname;
    soc_mem_t		mem;
    uint32		entry[SOC_MAX_MEM_WORDS], mask[SOC_MAX_MEM_WORDS];
    uint32		changed[SOC_MAX_MEM_WORDS];
    char		*valstr = NULL, *view = NULL;
    int			r, rv = CMD_FAIL;
    int			blk;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    tab = ARG_GET(a);
    idx = ARG_GET(a);
    cnt = ARG_GET(a);
    s = ARG_GET(a);

    /* you will need at least one dword and all the args .. */
    if (!tab || !idx || !cnt || !s || !isint(cnt)) {
	return CMD_USAGE;
    }

    if ((valstr = sal_alloc(ARGS_BUFFER, "mem_modify")) == NULL) {
        printk("cmd_esw_mem_modify : Out of memory\n");
        goto done;
    }

    memname = strstr(tab, ":"); 
    view_len = 0; 
    if (memname != NULL) { 
        memname++; 
        view_len = memname - tab; 
    } else { 
        memname = tab; 
    } 

    if (parse_memory_name(unit, &mem, memname, &copyno) < 0) {
	printk("ERROR: unknown table \"%s\"\n",tab);
    goto done;
    }

    if (view_len == 0) {
        collect_comma_args(a, valstr, s);
    } else {
        if ((view = sal_alloc(view_len + 1, "view_name")) == NULL) {
            printk("cmd_esw_mem_modify : Out of memory\n");
            goto done;
        }

        memcpy(view, tab, view_len);
        view[view_len] = 0;

        if (collect_comma_args_with_view(a, valstr, s, view, unit, mem) < 0) {
            printk("Out of memory: aborted\n");
            goto done;
        }
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }

#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (soc_mem_is_readonly(unit, mem)) {
	printk("ERROR: Table %s is read-only\n", SOC_MEM_UFNAME(unit, mem));
        goto done;
    }

    memset(changed, 0, sizeof (changed));

    if (modify_mem_fields(unit, mem, changed, mask, valstr, FALSE) < 0) {
	printk("Syntax error: aborted\n");
        goto done;
    }

    start = parse_memory_index(unit, mem, idx);
    count = parse_integer(cnt);

    /*
     * Take lock to ensure atomic modification of memory.
     */

    soc_mem_lock(unit, mem);

    rv = CMD_OK;

    for (index = start; index < start + count; index++) {
	SOC_MEM_BLOCK_ITER(unit, mem, blk) {
	    if (copyno != COPYNO_ALL && copyno != blk) {
		continue;
	    }

	    /*
	     * Retrieve the current entry, set masked fields to changed
	     * values, and write back.
	     */

#ifdef BCM_EASYRIDER_SUPPORT
            if (SOC_IS_EASYRIDER(unit) && (mem == FP_EXTERNALm)) {
                /* Access in normalized format */
                r = soc_er_fp_ext_read(unit, index,
                                       (void *)entry);
            } else if (SOC_IS_EASYRIDER(unit) && (mem == FP_TCAM_EXTERNALm)) {
                /* Access in normalized format */
                r = soc_er_fp_tcam_ext_read(unit, index,
                                            (void *)entry);
            } else
#endif
            {
                r = soc_mem_read(unit, mem, blk, index, entry);
            }

	    if (r < 0) {
		printk("ERROR: read from %s table copy %d failed: %s\n",
		       SOC_MEM_UFNAME(unit, mem), blk, soc_errmsg(r));
		rv = CMD_FAIL;
		break;
	    }

	    for (i = 0; i < SOC_MAX_MEM_WORDS; i++) {
		entry[i] = (entry[i] & ~mask[i]) | changed[i];
	    }

#ifdef BCM_EASYRIDER_SUPPORT
            if (SOC_IS_EASYRIDER(unit) && (mem == FP_EXTERNALm)) {
                /* Access in normalized format */
                r = soc_er_fp_ext_write(unit, index,
                                        (void *)entry);
            } else if (SOC_IS_EASYRIDER(unit) && (mem == FP_TCAM_EXTERNALm)) {
                /* Access in normalized format */
                r = soc_er_fp_tcam_ext_write(unit, index,
                                             (void *)entry);
            } else
#endif
            {
                r = soc_mem_write(unit, mem, blk, index, entry);
            }

	    if (r < 0) {
		printk("ERROR: write to %s table copy %d failed: %s\n",
		       SOC_MEM_UFNAME(unit, mem), blk, soc_errmsg(r));
		rv = CMD_FAIL;
		break;
	    }
	}

	if (rv != CMD_OK) {
	    break;
	}

	modify_mem_fields(unit, mem, changed, NULL, valstr, TRUE);
    }

    soc_mem_unlock(unit, mem);

 done:
    if (view != NULL) {
        sal_free(view);
    }
    sal_free(valstr);
    return rv;
}

/*
 * Auxiliary routine to perform either insert or delete
 */

#define TBL_OP_INSERT		0
#define TBL_OP_DELETE		1
#define TBL_OP_LOOKUP		2

static cmd_result_t
do_ins_del_lkup(int unit, args_t *a, int tbl_op)
{
    soc_control_t	*soc = SOC_CONTROL(unit);
    uint32		entry[SOC_MAX_MEM_WORDS];
    uint32		result[SOC_MAX_MEM_WORDS];
    int			entry_dw = 0, index;
    char		*ufname, *s;
    int			rv = CMD_FAIL;
    char		valstr[1024];
    soc_mem_t		mem;
    int			copyno, update, view_len = 0;
    int			swarl = 0;	/* Indicates operating on
					   software ARL/L2 table */
    char		*tab, *memname, *view = NULL;
    int			count = 1;
    soc_mem_t		arl_mem = INVALIDm;
    shr_avl_compare_fn	arl_cmp = NULL;
    int			quiet = 0;
    uint8               banks = 0;
    int			i=0;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    for (;;) {
	if ((tab = ARG_GET(a)) == NULL) {
	    return CMD_USAGE;
	}

	if (isint(tab)) {
	    count = parse_integer(tab);
	    continue;
	}

	if (sal_strcasecmp(tab, "quiet") == 0) {
	    quiet = 1;
	    continue;
	}
        
	if (sal_strcasecmp(tab, "bank0") == 0) {
	    banks = SOC_MEM_HASH_BANK0_ONLY;
	    continue;
	}

	if (sal_strcasecmp(tab, "bank1") == 0) {
	    banks = SOC_MEM_HASH_BANK1_ONLY;
	    continue;
	}

	break;
    }

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_arl_hashed)) {
	arl_mem = L2Xm;
	arl_cmp = soc_l2x_entry_compare_key;
    }
#endif

    if (!sal_strcasecmp(tab, "sa") || !sal_strcasecmp(tab, "sarl")) {
	if (soc->arlShadow == NULL) {
	    printk("ERROR: No software ARL table\n");
	    goto done;
	}
	mem = arl_mem;
        if (mem != INVALIDm) {
            copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        }
	swarl = 1;
    } else {
        memname = strstr(tab, ":");
  	if (memname != NULL) {
  	    memname++;
  	    view_len = memname - tab;
  	} else {
  	    memname = tab;
  	}
        if (parse_memory_name(unit, &mem, memname, &copyno) < 0) {
	    printk("ERROR: unknown table \"%s\"\n", tab);
	    goto done;
        }
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               (mem != INVALIDm) ? SOC_MEM_UFNAME(unit, mem) : "INVALID",
               SOC_UNIT_NAME(unit));
        goto done;
    }

    if (!soc_mem_is_sorted(unit, mem) &&
	!soc_mem_is_hashed(unit, mem) &&
        !soc_mem_is_cam(unit, mem) &&
        !soc_mem_is_cmd(unit, mem)) {
	printk("ERROR: %s table is not sorted, hashed, CAM, or command\n",
	       SOC_MEM_UFNAME(unit, mem));
	goto done;
    }

    /* All hash tables have multiple banks from TRX onwards only */
#ifndef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_dual_hash) && !SOC_IS_TRX(unit)) {
        if (banks != 0) {
            switch (mem) {
            case L2Xm:
            case MPLS_ENTRYm:
            case VLAN_XLATEm:
            case EGR_VLAN_XLATEm: 
            case L3_ENTRY_IPV4_UNICASTm:
            case L3_ENTRY_IPV4_MULTICASTm:
            case L3_ENTRY_IPV6_UNICASTm:
            case L3_ENTRY_IPV6_MULTICASTm:
                 break;
            default:
                printk("ERROR: %s table does not have multiple banks\n",
                       SOC_MEM_UFNAME(unit, mem));
                goto done;
            }
        }
    }
#endif

    entry_dw = soc_mem_entry_words(unit, mem);
    ufname = (swarl ? "SARL" : SOC_MEM_UFNAME(unit, mem));

    if (tbl_op == TBL_OP_LOOKUP && copyno == COPYNO_ALL) {
	copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    /*
     * If a list of fields were specified, generate the entry from them.
     * Otherwise, generate it by reading raw dwords from command line.
     */

    if ((s = ARG_GET(a)) == 0) {
	printk("ERROR: missing data for entry to %s\n",
	       tbl_op == TBL_OP_INSERT ? "insert" :
	       tbl_op == TBL_OP_DELETE ? "delete" : "lookup");
	goto done;
    }

    if (!isint(s)) {
	/* List of fields */
        memset(valstr, 0, 1024);
        if (view_len == 0) {
            collect_comma_args(a, valstr, s);
	} else {
  	    if ((view = sal_alloc(view_len + 1, "view_name")) == NULL) {
  	        printk("cmd_esw_mem_modify : Out of memory\n");
  	        goto done;
  	    }
  	    memset(view, 0, view_len + 1);
  	    memcpy(view, tab, view_len);
  	    if (collect_comma_args_with_view(a, valstr, s, view, unit, mem) < 0) {
  	        printk("Out of memory: aborted\n");
  	        goto done;
  	    }
  	}

	memset(entry, 0, sizeof (entry));

	if (modify_mem_fields(unit, mem, entry, NULL, valstr, FALSE) < 0) {
	    printk("Syntax error in field specification\n");
	    goto done;
	}

	update = TRUE;
    } else {
	/* List of numeric values */

	ARG_PREV(a);

	if (parse_dwords(entry_dw, entry, a) < 0) {
	    goto done;
	}

	update = FALSE;
    }

    if (debugk_check(DK_SOCMEM)) {
	printk("%s[%s], DATA:",
	       tbl_op == TBL_OP_INSERT ? "INSERT" :
	       tbl_op == TBL_OP_DELETE ? "DELETE" : "LOOKUP", ufname);
	for (i = 0; i < entry_dw; i++) {
	    printk(" 0x%x", entry[i]);
	}
	printk("\n");
    }

    /*
     * Have entry data, now insert, delete, or lookup.
     * For delete and lookup, all fields except the key are ignored.
     * Software ARL/L2 table requires special processing.
     */

    while (count--) {
	switch (tbl_op) {
	case TBL_OP_INSERT:
	    if (swarl) {
		if (soc->arlShadow != NULL) {
		    sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
		    i = shr_avl_insert(soc->arlShadow,
				       arl_cmp,
				       (shr_avl_datum_t *) entry);
		    sal_mutex_give(soc->arlShadowMutex);
		} else {
		    i = SOC_E_NONE;
		}
#ifdef BCM_TRIUMPH_SUPPORT
            } else if (mem == EXT_L2_ENTRYm) {
                i = soc_mem_generic_insert(unit, mem, MEM_BLOCK_ANY, 0,
                                           entry, NULL, 0);
                if (i == SOC_E_EXISTS) {
                    i = SOC_E_NONE;
                }
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            } else if (banks != 0){
                switch (mem) {
                case L2Xm:
                    i = soc_fb_l2x_bank_insert(unit, banks,
                                               (void *)entry);
                    break;
#if defined(INCLUDE_L3)
                case L3_ENTRY_IPV4_UNICASTm:
                case L3_ENTRY_IPV4_MULTICASTm:
                case L3_ENTRY_IPV6_UNICASTm:
                case L3_ENTRY_IPV6_MULTICASTm:
                    i = soc_fb_l3x_bank_insert(unit, banks,
                               (void *)entry);
                    break;
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
                case MPLS_ENTRYm:
#endif
                case VLAN_XLATEm:
                case VLAN_MACm:
                    i = soc_mem_bank_insert(unit, mem, banks,
                                     copyno, (void *)entry, NULL);
                    break;
                default:
                    i = SOC_E_INTERNAL;
                    goto done;
                }
#endif /* BCM_FIREBOLT2_SUPPORT */
	    } else {
		i = soc_mem_insert(unit, mem, copyno, entry);
                if (i == SOC_E_EXISTS) {
                    i = SOC_E_NONE;
                }
	    }

	    if (quiet && i == SOC_E_FULL) {
		i = SOC_E_NONE;
	    }

	    if (i < 0) {
		printk("Insert ERROR: %s table insert failed: %s\n",
		       ufname, soc_errmsg(i));
		goto done;
	    }

	    break;

	case TBL_OP_DELETE:
	    if (swarl) {
		if (soc->arlShadow != NULL) {
		    sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
		    i = shr_avl_delete(soc->arlShadow,
				       arl_cmp,
				       (shr_avl_datum_t *) entry);
		    sal_mutex_give(soc->arlShadowMutex);
		} else {
		    i = SOC_E_NONE;
		}
#ifdef BCM_TRX_SUPPORT
            } else if (soc_feature(unit, soc_feature_generic_table_ops)) {
                i = soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY,
                                           banks, entry, NULL, 0);
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            } else if (banks != 0) {
                switch (mem) {
                case L2Xm:
                    i = soc_fb_l2x_bank_delete(unit, banks,
                                               (void *)entry);
                    break;
#if defined(INCLUDE_L3)
                case L3_ENTRY_IPV4_UNICASTm:
                case L3_ENTRY_IPV4_MULTICASTm:
                case L3_ENTRY_IPV6_UNICASTm:
                case L3_ENTRY_IPV6_MULTICASTm:
                    i = soc_fb_l3x_bank_delete(unit, banks,
                               (void *)entry);
                    break;
#endif
                default:
                    i = SOC_E_INTERNAL;
                    goto done;
                }
#endif
	    } else {
		i = soc_mem_delete(unit, mem, copyno, entry);
	    }

	    if (quiet && i == SOC_E_NOT_FOUND) {
		i = SOC_E_NONE;
	    }

	    if (i < 0) {
		printk("Delete ERROR: %s table delete failed: %s\n",
		       ufname, soc_errmsg(i));
		goto done;
	    }

	    break;

	case TBL_OP_LOOKUP:
	    if (swarl) {
		if (soc->arlShadow != NULL) {
		    sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
		    i = shr_avl_lookup(soc->arlShadow,
				       arl_cmp,
				       (shr_avl_datum_t *) entry);
		    sal_mutex_give(soc->arlShadowMutex);
                    if (i) {
                        i = SOC_E_NONE;
                        index = -1;
                        memcpy(result, entry,
                               soc_mem_entry_words(unit, mem) * 4);
                    } else {
                        i = SOC_E_NOT_FOUND;
                    }
		} else {
		    i = SOC_E_NONE;
                    goto done;
		}
#ifdef BCM_TRX_SUPPORT
            } else if (soc_feature(unit, soc_feature_generic_table_ops)) {
                i = soc_mem_generic_lookup(unit, mem, MEM_BLOCK_ANY,
                                           banks, entry, result, &index);
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
            } else if (banks != 0) {
                switch (mem) {
                case L2Xm:
                    i = soc_fb_l2x_bank_lookup(unit, banks,
                                               (void *)entry,
                                               (void *)result,
                                               &index);
                    break;
#if defined(INCLUDE_L3)
                case L3_ENTRY_IPV4_UNICASTm:
                case L3_ENTRY_IPV4_MULTICASTm:
                case L3_ENTRY_IPV6_UNICASTm:
                case L3_ENTRY_IPV6_MULTICASTm:
                    i = soc_fb_l3x_bank_lookup(unit, banks,
                                               (void *)entry,
                                               (void *)result,
                                               &index);
                    break;
#endif
                default:
                    i = SOC_E_INTERNAL;
                    goto done;
                }
#endif /* BCM_FIREBOLT2_SUPPORT */
	    } else {
		i = soc_mem_search(unit, mem, copyno,
                                   &index, entry, result, 0);
	    }

	    if (i < 0) {		/* Error not fatal for lookup */
                if (i == SOC_E_NOT_FOUND) {
                    printk("Lookup: No matching entry found\n");
                } else {
                    printk("Lookup ERROR: read error during search: %s\n",
                           soc_errmsg(i));
                }
	    } else { /* Entry found */
		if (index < 0) {
		    printk("Found in %s.%s: ",
			   ufname, SOC_BLOCK_NAME(unit, copyno));
		} else {
		    printk("Found %s.%s[%d]: ",
			   ufname, SOC_BLOCK_NAME(unit, copyno), index);
		}
		soc_mem_entry_dump(unit, mem, result);
		printk("\n");
            }

	    break;

	default:
	    assert(0);
	    break;
	}

	if (update) {
	    modify_mem_fields(unit, mem, entry, NULL, valstr, TRUE);
	}
    }

    rv = CMD_OK;

 done:
    if (view != NULL) {
        sal_free(view);
    }
    return rv;
}

/*
 * Insert an entry by key into a sorted table
 */

char insert_usage[] =
    "\nParameters 1: [quiet] [<COUNT>] [bank#] <TABLE> <DW0> .. <DWN>\n"
    "Parameters 2: [quiet] [<COUNT>] <TABLE> <FIELD>=<VALUE> ...\n\t"
    "Number of <DW> must be appropriate to table entry size.\n\t"
    "Entry is inserted in ascending sorted order by key field.\n\t"
    "The quiet option indicates not to complain on table/bucket full.\n\t"
    "Some tables allow restricting the insert to a single bank.\n";

cmd_result_t
mem_insert(int unit, args_t *a)
{
    return do_ins_del_lkup(unit, a, TBL_OP_INSERT);
}


/*
 * Delete entries by key from a sorted table
 */

char delete_usage[] =
    "\nParameters 1: [quiet] [<COUNT>] [bank#] <TABLE> <DW0> .. <DWN>\n"
    "Parameters 2: [quiet] [<COUNT>] <TABLE> <FIELD>=<VALUE> ...\n\t"
    "Number of <DW> must be appropriate to table entry size.\n\t"
    "The entry is deleted by key.\n\t"
    "All fields other than the key field(s) are ignored.\n\t"
    "The quiet option indicates not to complain on entry not found.\n\t"
    "Some tables allow restricting the delete to a single bank.\n";

cmd_result_t
mem_delete(int unit, args_t *a)
{
    return do_ins_del_lkup(unit, a, TBL_OP_DELETE);
}

/*
 * Lookup entry an key in a sorted table
 */

char lookup_usage[] =
    "\nParameters 1: [<COUNT>] [bank#] <TABLE> <DW0> .. <DWN>\n"
    "Parameters 2: [<COUNT>] <TABLE> <FIELD>=<VALUE> ...\n\t"
    "Number of <DW> must be appropriate to table entry size.\n\t"
    "The entry is looked up by key.\n\t"
    "All fields other than the key field(s) are ignored.\n\t"
    "Some tables allow restricting the lookup to a single bank.\n";

cmd_result_t
mem_lookup(int unit, args_t *a)
{
    return do_ins_del_lkup(unit, a, TBL_OP_LOOKUP);
}

/*
 * Remove (delete) entries by index from a sorted table
 */

char remove_usage[] =
    "Parameters: <TABLE> <INDEX> [COUNT]\n\t"
    "Deletes an entry from a table by index.\n\t"
    "(For the ARL, reads the table entry and deletes by key).\n";

cmd_result_t
mem_remove(int unit, args_t *a)
{
    char		*tab, *ind, *cnt, *ufname;
    soc_mem_t		mem;
    int			copyno;
    int			r, rv = CMD_FAIL;
    int			index, count;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    tab = ARG_GET(a);
    ind = ARG_GET(a);
    cnt = ARG_GET(a);

    if (!tab || !ind) {
	return CMD_USAGE;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno) < 0) {
	printk("ERROR: unknown table \"%s\"\n", tab);
	goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }

    ufname = SOC_MEM_UFNAME(unit, mem);

    if (!soc_mem_is_sorted(unit, mem) &&
	!soc_mem_is_hashed(unit, mem) &&
	!soc_mem_is_cam(unit, mem)) {
	printk("ERROR: %s table is not sorted, hashed, or CAM\n",
	       ufname);
	goto done;
    }

    count = cnt ? parse_integer(cnt) : 1;

    index = parse_memory_index(unit, mem, ind);

    while (count-- > 0)
	if ((r = soc_mem_delete_index(unit, mem, copyno, index)) < 0) {
	    printk("ERROR: delete %s table index 0x%x failed: %s\n",
		   ufname, index, soc_errmsg(r));
	    goto done;
	}

    rv = CMD_OK;

 done:
    return rv;
}

/*  
 * Pop an entry from a FIFO
 */

char pop_usage[] =
    "\nParameters: [quiet] [<COUNT>] <TABLE>\n"
    "The quiet option indicates not to complain on FIFO empty.\n";

cmd_result_t
mem_pop(int unit, args_t *a)
{   
    uint32              result[SOC_MAX_MEM_WORDS];
    char                *ufname;
    int                 rv = CMD_FAIL;
    soc_mem_t           mem;
    int                 copyno;
    char                *tab; 
    int                 count = 1;
    int                 quiet = 0;
    int                 i;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (!soc_feature(unit, soc_feature_mem_push_pop)) {
        /* feature not supported */
        return CMD_FAIL;
    }

    for (;;) {
        if ((tab = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }

        if (isint(tab)) {
            count = parse_integer(tab);
            continue;
        }

        if (sal_strcasecmp(tab, "quiet") == 0) {
            quiet = 1;
            continue;
        }
        break;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno) < 0) {
        printk("ERROR: unknown table \"%s\"\n", tab);
        goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }

    switch (mem) {
#if defined (BCM_TRIUMPH_SUPPORT)
        case ING_IPFIX_EXPORT_FIFOm:
        case EGR_IPFIX_EXPORT_FIFOm:
        case EXT_L2_MOD_FIFOm:
#endif /* BCM_TRIUMPH_SUPPORT */
        case L2_MOD_FIFOm:
            break;
        default:
            printk("ERROR: %s table does not support FIFO push/pop\n",
                   SOC_MEM_UFNAME(unit, mem));
            goto done;
    }

    ufname = SOC_MEM_UFNAME(unit, mem);

    if (copyno == COPYNO_ALL) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    if (debugk_check(DK_SOCMEM)) {
        printk("POP[%s]", ufname);
        printk("\n");
    }

    while (count--) {
        i = soc_mem_pop(unit, mem, copyno, result);

        if (i < 0) {            /* Error not fatal for lookup */
            if (i != SOC_E_NOT_FOUND) {
                printk("Pop ERROR: read error during pop: %s\n",
                       soc_errmsg(i));
            } else if (!quiet) {
                printk("Pop: Fifo empty\n");
            }
        } else { /* Entry popped */
            printk("Popped in %s.%s: ",
                   ufname, SOC_BLOCK_NAME(unit, copyno));
            soc_mem_entry_dump(unit, mem, result);
            printk("\n");
        }
    }

    rv = CMD_OK;

 done:
    return rv;
}

/*
 * Push an entry onto a FIFO
 */

char push_usage[] =
    "\nParameters: [quiet] [<COUNT>] <TABLE>\n"
    "The quiet option indicates not to complain on FIFO full.\n";

cmd_result_t
mem_push(int unit, args_t *a)
{
    uint32              entry[SOC_MAX_MEM_WORDS];
    int                 entry_dw;
    char                *ufname, *s;
    int                 rv = CMD_FAIL;
    char                valstr[1024];
    soc_mem_t           mem;
    int                 copyno;
    char                *tab;
    int                 count = 1;
    int                 quiet = 0;
    int                 i;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (!soc_feature(unit, soc_feature_mem_push_pop)) {
        /* feature not supported */
        return CMD_FAIL;
    }

    for (;;) {
        if ((tab = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }

        if (isint(tab)) {
            count = parse_integer(tab);
            continue;
        }

        if (sal_strcasecmp(tab, "quiet") == 0) {
            quiet = 1;
            continue;
        }
        break;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno) < 0) {
        printk("ERROR: unknown table \"%s\"\n", tab);
        goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }

    switch (mem) { 
#if defined (BCM_TRIUMPH_SUPPORT)
        case ING_IPFIX_EXPORT_FIFOm:
        case EGR_IPFIX_EXPORT_FIFOm:
        case EXT_L2_MOD_FIFOm:
#endif /* BCM_TRIUMPH_SUPPORT */
        case L2_MOD_FIFOm:
            break;
        default:
            printk("ERROR: %s table does not support FIFO push/pop\n",
                   SOC_MEM_UFNAME(unit, mem));
            goto done;
    }
    
    entry_dw = soc_mem_entry_words(unit, mem);
    ufname = SOC_MEM_UFNAME(unit, mem);
    
    if (copyno == COPYNO_ALL) { 
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    /* 
     * If a list of fields were specified, generate the entry from them.
     * Otherwise, generate it by reading raw dwords from command line.
     */

    if ((s = ARG_GET(a)) == 0) { 
        printk("ERROR: missing data for entry to push\n");
        goto done;
    }

    if (!isint(s)) {
        /* List of fields */

        collect_comma_args(a, valstr, s);

        memset(entry, 0, sizeof (entry));

        if (modify_mem_fields(unit, mem, entry, NULL, valstr, FALSE) < 0) {
            printk("Syntax error in field specification\n");
            goto done;
        }
    } else {
        /* List of numeric values */

        ARG_PREV(a);

        if (parse_dwords(entry_dw, entry, a) < 0) {
            goto done;
        }
    }

    if (debugk_check(DK_SOCMEM)) {
        printk("PUSH[%s], DATA:", ufname);
        for (i = 0; i < entry_dw; i++) {
            printk(" 0x%x", entry[i]);
        }
        printk("\n");
    }

    /*
     * Have entry data, push entry.
     */

    while (count--) {
        i = soc_mem_push(unit, mem, copyno, entry);
        if (quiet && i == SOC_E_FULL) {
            i = SOC_E_NONE;
        }

        if (i < 0) {
            printk("Push ERROR: %s table push failed: %s\n",
                   ufname, soc_errmsg(i));
            goto done;
        }
    }

    rv = CMD_OK;

 done:
    return rv;
}

/*
 * do_search_entry
 *
 *  Auxiliary routine for command_search()
 *  Returns byte position within entry where pattern is found.
 *  Returns -1 if not found.
 *  Finds pattern in either big- or little-endian order.
 */

static int
do_search_entry(uint8 *entry, int entry_len,
		uint8 *pat, int pat_len)
{
    int start, i;

    for (start = 0; start <= entry_len - pat_len; start++) {
	for (i = 0; i < pat_len; i++) {
	    if (entry[start + i] != pat[i]) {
		break;
	    }
	}

	if (i == pat_len) {
	    return start;
	}

	for (i = 0; i < pat_len; i++) {
	    if (entry[start + i] != pat[pat_len - 1 - i]) {
		break;
	    }
	}

	if (i == pat_len) {
	    return start;
	}
    }

    return -1;
}

/*
 * Search a table for a byte pattern
 */

#define SRCH_MAX_PAT		16

char search_usage[] =
    "1. Parameters: [ALL | BIN | FBIN] <TABLE>[.<COPY>] 0x<PATTERN>\n"
#ifndef COMPILER_STRING_CONST_LIMIT
    "\tDumps table entries containing the specified byte pattern\n\t"
    "anywhere in the entry in big- or little-endian order.\n\t"
    "Example: search arl 0x112233445566\n"
#endif
    "2. Parameters: [ALL | BIN | FBIN] <TABLE>[.<COPY>]"
         "<FIELD>=<VALUE>[,...]\n"
#ifndef COMPILER_STRING_CONST_LIMIT
    "\tDumps table entries where the specified fields are equal\n\t"
    "to the specified values.\n\t"
    "Example: search l3 ip_addr=0xf4000001,pnum=10\n\t"
    "The ALL flag indicates to search sorted tables in their\n\t"
    "entirety instead of only entries known to contain data.\n\t"
    "The BIN flag does a binary search of sorted tables in\n\t"
    "the portion known to contain data.\n\t"
    "The FBIN flag does a binary search, and guarantees if there\n\t"
    "are duplicates the lowest matching index will be returned.\n"
#endif
    ;

cmd_result_t
mem_search(int unit, args_t *a)
{
    int			t, rv = CMD_FAIL;
    char		*tab, *patstr;
    soc_mem_t		mem;
    uint32		pat_entry[SOC_MAX_MEM_WORDS];
    uint32		pat_mask[SOC_MAX_MEM_WORDS];
    uint32		entry[SOC_MAX_MEM_WORDS];
    char		valstr[1024];
    uint8		pat[SRCH_MAX_PAT];
    char		*ufname;
    int			patsize = 0, found_count = 0, entry_bytes, entry_dw;
    int			copyno, index, min, max;
    int			byte_search;
    int			all_flag = 0, bin_flag = 0, lowest_match = 0;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    tab = ARG_GET(a);

    while (tab) {
	if (!sal_strcasecmp(tab, "all")) {
	    all_flag = 1;
	} else if (!sal_strcasecmp(tab, "bin")) {
	    bin_flag = 1;
	} else if (!sal_strcasecmp(tab, "fbin")) {
	    bin_flag = 1;
	    lowest_match = 1;
	} else {
	    break;
	}
	tab = ARG_GET(a);
    }

    patstr = ARG_GET(a);

    if (!tab || !patstr) {
	return CMD_USAGE;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno) < 0) {
	printk("ERROR: unknown table \"%s\"\n", tab);
	goto done;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
               SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
        goto done;
    }

    if (copyno == COPYNO_ALL) {
	copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    entry_bytes = soc_mem_entry_bytes(unit, mem);
    entry_dw = BYTES2WORDS(entry_bytes);
    min = soc_mem_index_min(unit, mem);
    max = soc_mem_index_max(unit, mem);
    ufname = SOC_MEM_UFNAME(unit, mem);

    byte_search = isint(patstr);

    if (byte_search) {
	char *s;

	/*
	 * Search by pattern string.
	 * Parse pattern string (long hex number) into array of bytes.
	 */

	if (patstr[0] != '0' ||
	    (patstr[1] != 'x' && patstr[1] != 'X') || patstr[2] == 0) {
	    printk("ERROR: illegal search pattern, need hex constant\n");
	    goto done;
	}

	patstr += 2;

	for (s = patstr; *s; s++) {
	    if (!isxdigit((unsigned) *s)) {
		printk("ERROR: invalid hex digit in search pattern\n");
		goto done;
	    }
	}

	while (s > patstr + 1) {
	    s -= 2;
	    pat[patsize++] = xdigit2i(s[0]) << 4 | xdigit2i(s[1]);
	}

	if (s > patstr) {
	    s--;
	    pat[patsize++] = xdigit2i(s[0]);
	}

#if 0
	printk("PATTERN: ");
	for (index = 0; index < patsize; index++) {
	    printk("%02x ", pat[index]);
	}
	printk("\n");
#endif
    } else {
	/*
	 * Search by pat_entry and pat_mask.
	 * Collect list of fields
	 */

	collect_comma_args(a, valstr, patstr);

	memset(pat_entry, 0, sizeof (pat_entry));

	if (modify_mem_fields(unit, mem,
			      pat_entry, pat_mask,
			      valstr, FALSE) < 0) {
	    printk("Syntax error: aborted\n");
	    goto done;
	}
    }

    if (bin_flag) {
	if (!soc_mem_is_sorted(unit, mem)) {
	    printk("ERROR: can only binary search sorted tables\n");
	    goto done;
	}

	if (byte_search) {
	    printk("ERROR: can only binary search for "
		   "<FIELD>=<VALUE>[,...]\n");
	    goto done;
	}

	printk("Searching %s...\n", ufname);

	t = soc_mem_search(unit, mem, copyno, &index,
				pat_entry, entry, lowest_match);
        if ((t < 0) && (t != SOC_E_NOT_FOUND)) {
	    printk("Read ERROR: table[%s]: %s\n", ufname, soc_errmsg(t));
	    goto done;
	}

	if (t == SOC_E_NONE) {
	    printk("%s[%d]: ", ufname, index);
	    soc_mem_entry_dump(unit, mem, entry);
	    printk("\n");
	} else {
	    printk("Nothing found\n");
        }

	rv = CMD_OK;
	goto done;
    }

    if (!all_flag && soc_mem_is_sorted(unit, mem)) {
	max = soc_mem_index_last(unit, mem, copyno);
    }

    printk("Searching %s table indexes 0x%x through 0x%x...\n",
	   ufname, min, max);

    for (index = min; index <= max; index++) {
	int match;

	if ((t = soc_mem_read(unit, mem, copyno, index, entry)) < 0) {
	    printk("Read ERROR: table %s.%s[%d]: %s\n",
		   ufname, SOC_BLOCK_NAME(unit, copyno), index, soc_errmsg(t));
	    goto done;
	}

#ifdef BCM_XGS_SWITCH_SUPPORT
	if (!all_flag) {
#ifdef BCM_XGS12_SWITCH_SUPPORT
            if SOC_IS_XGS12_SWITCH(unit) {
	        if (mem == L2Xm &&
		    !soc_L2Xm_field32_get(unit, entry, VALID_BITf)) {
		    continue;
	        }

	        if (mem == L3Xm &&
		    !soc_L3Xm_field32_get(unit, entry, L3_VALIDf)) {
		    continue;
	        }
            }
#endif /* BCM_XGS12_SWITCH_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
#ifdef BCM_EASYRIDER_SUPPORT
            if SOC_IS_EASYRIDER(unit) {
                switch (mem) {
                    case L2_ENTRY_INTERNALm:
                         if (!soc_L2_ENTRY_INTERNALm_field32_get(unit, 
                              entry, VALID_BITf)) {
                             continue;
                         }
                         break;
                    case L2_ENTRY_EXTERNALm:
                         if (!soc_L2_ENTRY_EXTERNALm_field32_get(unit, 
                              entry, VALID_BITf)) {
                             continue;
                         }
                         break;
                    case L2_USER_ENTRYm:
                         if (!soc_L2_USER_ENTRYm_field32_get(unit, 
                              entry, VALIDf)) {
                             continue;
                         }
                         break;
                    case L3_ENTRY_V4m: 
                         if (!soc_L3_ENTRY_V4m_field32_get(unit,
                              entry, VALIDf)) {
                             continue;
                         }
                         break;
                    case L3_ENTRY_V6m: 
                         if (!soc_L3_ENTRY_V6m_field32_get(unit,
                              entry, VALIDf)) {
                             continue;
                         }
                         break;
                    default:
                         break; 
                } 
            }
#endif /* BCM_EASYRIDER_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
            if (SOC_IS_FBX(unit) || SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) { 
                switch (mem) {
                    case L2Xm:
                        if (!soc_L2Xm_field32_get(unit, entry, VALIDf)) {
                            continue;
                        }
                        break; 
                    case L2_USER_ENTRYm:
                        if (!soc_L2_USER_ENTRYm_field32_get(unit, 
                               entry, VALIDf)) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV4_UNICASTm:
                        if (!soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, 
                               entry, VALIDf)) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV4_MULTICASTm:
                        if (!soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit,
                               entry, VALIDf)) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV6_UNICASTm:
                        if (!(soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit,
                                entry, VALID_0f) && 
                              soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit,
                                entry, VALID_1f))) {
                            continue;
                        }
                        break;
                    case L3_ENTRY_IPV6_MULTICASTm:
                        if (!(soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit,
                                entry, VALID_0f) &&
                              soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit,
                                entry, VALID_1f) &&
                              soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit,
                                entry, VALID_2f) &&
                              soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(unit, 
                                entry, VALID_3f))) {
                            continue;
                        }
                        break;
                    default:
                        break;
                }
            }
#endif /* BCM_FIREBOLT_SUPPORT */
#endif /* BCM_XGS3_SWITCH_SUPPORT */
	}
#endif /* BCM_XGS_SWITCH_SUPPORT */

	if (byte_search) {
	    match = (do_search_entry((uint8 *) entry, entry_bytes,
				     pat, patsize) >= 0);
	} else {
	    int i;

	    for (i = 0; i < entry_dw; i++)
		if ((entry[i] & pat_mask[i]) != pat_entry[i])
		    break;

	    match = (i == entry_dw);
	}

	if (match) {
	    printk("%s.%s[%d]: ",
		   ufname, SOC_BLOCK_NAME(unit, copyno), index);
	    soc_mem_entry_dump(unit, mem, entry);
	    printk("\n");
	    found_count++;
	}
    }

    if (found_count == 0) {
	printk("Nothing found\n");
    }

    rv = CMD_OK;

 done:
    return rv;
}

/*
 * Print a one line summary for matching memories
 * If substr_match is NULL, match all memories.
 * If substr_match is non-NULL, match any memories whose name
 * or user-friendly name contains that substring.
 */

static void
mem_list_summary(int unit, char *substr_match)
{
    soc_mem_t		mem;
    int			i, copies, dlen;
    int			found = 0;
    char		*dstr;

    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
	if (!soc_mem_is_valid(unit, mem)) {
	    continue;
        }

#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
            continue;
        }
#endif /* BCM_HAWKEYE_SUPPORT */

	if (substr_match != NULL &&
	    strcaseindex(SOC_MEM_NAME(unit, mem), substr_match) == NULL &&
	    strcaseindex(SOC_MEM_UFNAME(unit, mem), substr_match) == NULL) {
	    continue;
	}

	copies = 0;
	SOC_MEM_BLOCK_ITER(unit, mem, i) {
	    copies += 1;
	}

	dlen = strlen(SOC_MEM_DESC(unit, mem));
	if (dlen > 38) {
	    dlen = 34;
	    dstr = "...";
	} else {
	    dstr = "";
	}
	if (!found) {
	    printk(" %-6s  %-22s%5s/%-4s %s\n",
		   "Flags", "Name", "Entry",
		   "Copy", "Description");
	    found = 1;
	}

	printk(" %c%c%c%c%c%c  %-22s%5d",
	       soc_mem_is_readonly(unit, mem) ? 'r' : '-',
	       soc_mem_is_debug(unit, mem) ? 'd' : '-',
	       soc_mem_is_sorted(unit, mem) ? 's' :
	       soc_mem_is_hashed(unit, mem) ? 'h' :
	       soc_mem_is_cam(unit, mem) ? 'A' : '-',
	       soc_mem_is_cbp(unit, mem) ? 'c' : '-',
	       (soc_mem_is_bistepic(unit, mem) ||
		soc_mem_is_bistffp(unit, mem) ||
		soc_mem_is_bistcbp(unit, mem)) ? 'b' : '-',
	       soc_mem_is_cachable(unit, mem) ? 'C' : '-',
	       SOC_MEM_UFNAME(unit, mem),
	       soc_mem_index_count(unit, mem));
	if (copies == 1) {
	    printk("%5s %*.*s%s\n",
		   "",
		   dlen, dlen, SOC_MEM_DESC(unit, mem), dstr);
	} else {
	    printk("/%-4d %*.*s%s\n",
		   copies,
		   dlen, dlen, SOC_MEM_DESC(unit, mem), dstr);
	}
    }

    if (found) {
	printk("Flags: (r)eadonly, (d)ebug, (s)orted, (h)ashed\n"
	       "       C(A)M, (c)bp, (b)ist-able, (C)achable\n");
    } else if (substr_match != NULL) {
	printk("No memory found with the substring '%s' in its name.\n",
	       substr_match);
    }
}

/*
 * List the tables, or fields of a table entry
 */

cmd_result_t
cmd_esw_mem_list(int unit, args_t *a)
{
    soc_mem_info_t	*m;
    soc_field_info_t	*fld;
    char		*tab, *s;
    soc_mem_t		mem;
    uint32		entry[SOC_MAX_MEM_WORDS];
    uint32		mask[SOC_MAX_MEM_WORDS];
    int			have_entry, i, dw, copyno;
    int			copies, disabled, dmaable;
    char		*dmastr;
    uint32		flags;
    int			minidx, maxidx;
    uint8               at;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (!soc_property_get(unit, spn_MEMLIST_ENABLE, 1)) {
        return CMD_OK;
    }
    tab = ARG_GET(a);

    if (!tab) {
	mem_list_summary(unit, NULL);
	return CMD_OK;
    }

    if (parse_memory_name(unit, &mem, tab, &copyno) < 0) {
	if ((s = strchr(tab, '.')) != NULL) {
	    *s = 0;
	}
	mem_list_summary(unit, tab);
	return CMD_OK;
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
	printk("ERROR: Memory \"%s\" not valid for this unit\n", tab);
	return CMD_FAIL;
    }

#if defined(BCM_HAWKEYE_SUPPORT)
    if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
        printk("ERROR: Memory \"%s\" not valid for this unit\n", tab);
        return CMD_FAIL;
    }
#endif /* BCM_HAWKEYE_SUPPORT */

    if (copyno < 0) {
	copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        if (copyno < 0) {
            printk("ERROR: Memory \"%s\" has no instance on this unit\n", tab);
            return CMD_FAIL;
        }
    } else if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
	printk("ERROR: Invalid copy number %d for memory %s\n", copyno, tab);
	return CMD_FAIL;
    }

    m = &SOC_MEM_INFO(unit, mem);
    flags = m->flags;

    dw = BYTES2WORDS(m->bytes);

    if ((s = ARG_GET(a)) == 0) {
	have_entry = 0;
    } else {
	for (i = 0; i < dw; i++) {
	    if (s == 0) {
		printk("Not enough data specified (%d words needed)\n", dw);
		return CMD_FAIL;
	    }
	    entry[i] = parse_integer(s);
	    s = ARG_GET(a);
	}
	if (s) {
	    printk("Extra data specified (ignored)\n");
	}
	have_entry = 1;
    }

    printk("Memory: %s.%s",
	   SOC_MEM_UFNAME(unit, mem),
	   SOC_BLOCK_NAME(unit, copyno));
    s = SOC_MEM_UFALIAS(unit, mem);
    if (s && *s && strcmp(SOC_MEM_UFNAME(unit, mem), s) != 0) {
	printk(" alias %s", s);
    }
    printk(" address 0x%08x\n", soc_mem_addr_get(unit, mem, copyno, 0, &at));

    printk("Flags:");
    if (flags & SOC_MEM_FLAG_READONLY) {
	printk(" read-only");
    }
    if (flags & SOC_MEM_FLAG_VALID) {
	printk(" valid");
    }
    if (flags & SOC_MEM_FLAG_DEBUG) {
	printk(" debug");
    }
    if (flags & SOC_MEM_FLAG_SORTED) {
	printk(" sorted");
    }
    if (flags & SOC_MEM_FLAG_CBP) {
	printk(" cbp");
    }
    if (flags & SOC_MEM_FLAG_CACHABLE) {
	printk(" cachable");
    }
    if (flags & SOC_MEM_FLAG_BISTCBP) {
	printk(" bist-cbp");
    }
    if (flags & SOC_MEM_FLAG_BISTEPIC) {
	printk(" bist-epic");
    }
    if (flags & SOC_MEM_FLAG_BISTFFP) {
	printk(" bist-ffp");
    }
    if (flags & SOC_MEM_FLAG_UNIFIED) {
	printk(" unified");
    }
    if (flags & SOC_MEM_FLAG_HASHED) {
	printk(" hashed");
    }
    if (flags & SOC_MEM_FLAG_WORDADR) {
	printk(" word-addressed");
    }
    if (flags & SOC_MEM_FLAG_MONOLITH) {
	printk(" monolithic");
    }
    if (flags & SOC_MEM_FLAG_BE) {
	printk(" big-endian");
    }
    if (flags & SOC_MEM_FLAG_MULTIVIEW) {
        printk(" multiview");
    }
    printk("\n");

    printk("Blocks: ");
    copies = disabled = dmaable = 0;
    SOC_MEM_BLOCK_ITER(unit, mem, i) {
	if (SOC_INFO(unit).block_valid[i]) {
	    dmastr = "";
#ifdef BCM_XGS_SWITCH_SUPPORT
	    if (soc_mem_dmaable(unit, mem, i)) {
		dmastr = "/dma";
		dmaable += 1;
	    }
#endif
	    printk(" %s%s", SOC_BLOCK_NAME(unit, i), dmastr);
	} else {
	    printk(" [%s]", SOC_BLOCK_NAME(unit, i));
	    disabled += 1;
	}
	copies += 1;
    }
    printk(" (%d cop%s", copies, copies == 1 ? "y" : "ies");
    if (disabled) {
	printk(", %d disabled", disabled);
    }
#ifdef BCM_XGS_SWITCH_SUPPORT
    if (dmaable) {
	printk(", %d dmaable", dmaable);
    }
#endif
    printk(")\n");

    minidx = soc_mem_index_min(unit, mem);
    maxidx = soc_mem_index_max(unit, mem);
    printk("Entries: %d with indices %d-%d (0x%x-0x%x)",
	   maxidx - minidx + 1,
	   minidx,
	   maxidx,
	   minidx,
	   maxidx);
    printk(", each %d bytes %d words\n", m->bytes, dw);

    printk("Entry mask:");
    soc_mem_datamask_get(unit, mem, mask);
    for (i = 0; i < dw; i++) {
	if (mask[i] == 0xffffffff) {
	    printk(" -1");
	} else if (mask[i] == 0) {
	    printk(" 0");
	} else {
	    printk(" 0x%08x", mask[i]);
	}
    }
    printk("\n");

    s = SOC_MEM_DESC(unit, mem);
    if (s && *s) {
	printk("Description: %s\n", s);
    }

    for (fld = &m->fields[m->nFields - 1]; fld >= &m->fields[0]; fld--) {
	printk("  %s<%d", SOC_FIELD_NAME(unit, fld->field), fld->bp + fld->len - 1);
	if (fld->len > 1) {
	    printk(":%d", fld->bp);
	}
	if (have_entry) {
	    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
	    char tmp[132];

	    memset(fval, 0, sizeof (fval));
	    soc_mem_field_get(unit, mem, entry, fld->field, fval);
	    format_long_integer(tmp, fval, SOC_MAX_MEM_FIELD_WORDS);
	    printk("> = %s\n", tmp);
	} else {
	    printk(">\n");
	}
    }

    return CMD_OK;
}

/*
 * Turn on/off software caching of hardware tables
 */

cmd_result_t
mem_esw_cache(int unit, args_t *a)
{
    soc_mem_t		mem;
    int			copyno;
    char		*c;
    int			enable, r;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (ARG_CNT(a) == 0) {
	for (enable = 0; enable < 2; enable++) {
	    printk("Caching is %s for:\n", enable ? "on" : "off");

	    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
		int any_printed;

		if ((!SOC_MEM_IS_VALID(unit, mem)) ||
                    (!soc_mem_is_cachable(unit, mem))) {
		    continue;
                }

		any_printed = 0;

		SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
		    if ((!enable) ==
			(!soc_mem_cache_get(unit, mem, copyno))) {
			if (!any_printed) {
			    printk("    ");
			}
			printk(" %s.%s",
			       SOC_MEM_UFNAME(unit, mem),
			       SOC_BLOCK_NAME(unit, copyno));
			any_printed = 1;
		    }
		}

		if (any_printed) {
		    printk("\n");
		}
	    }
	}

	return CMD_OK;
    }

    while ((c = ARG_GET(a)) != 0) {
	switch (*c) {
	case '+':
	    enable = 1;
	    c++;
	    break;
	case '-':
	    enable = 0;
	    c++;
	    break;
	default:
	    enable = 1;
	    break;
	}

	if (parse_memory_name(unit, &mem, c, &copyno) < 0) {
	    printk("%s: Unknown table \"%s\"\n", ARG_CMD(a), c);
	    return CMD_FAIL;
	}

        if (!SOC_MEM_IS_VALID(unit, mem)) {
            debugk(DK_ERR, "Error: Memory %s not valid for chip %s.\n",
                   SOC_MEM_UFNAME(unit, mem), SOC_UNIT_NAME(unit));
            continue;
        }

	if (!soc_mem_is_cachable(unit, mem)) {
	    printk("%s: Memory %s is not cachable\n",
		   ARG_CMD(a), SOC_MEM_UFNAME(unit, mem));
	    return CMD_FAIL;
	}

	if ((r = soc_mem_cache_set(unit, mem, copyno, enable)) < 0) {
	    printk("%s: Error setting cachability for %s: %s\n",
		   ARG_CMD(a), SOC_MEM_UFNAME(unit, mem), soc_errmsg(r));
	    return CMD_FAIL;
	}
    }

    return CMD_OK;
}

#ifdef INCLUDE_MEM_SCAN

/*
 * Turn on/off memory scan thread
 */

char memscan_usage[] =
    "Parameters: [Interval=<USEC>] [Rate=<ENTRIES>] [on] [off]\n\t"
    "Interval specifies how often to run (0 to stop)\n\t"
    "Rate specifies the number of entries to process per interval\n\t"
    "Any memories that are cached will be scanned (see 'cache' command)\n";

cmd_result_t
mem_scan(int unit, args_t *a)
{
    parse_table_t	pt;
    int			running, rv;
    sal_usecs_t	interval = 0;
    int		rate = 0;
    char		*c;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((running = soc_mem_scan_running(unit, &rate, &interval)) < 0) {
	printk("soc_mem_scan_running %d: ERROR: %s\n",
	       unit, soc_errmsg(running));
	return CMD_FAIL;
    }

    if (!running) {
	interval = 100000;
	rate = 64;
    }

    if (ARG_CNT(a) == 0) {
	printk("%s: %s on unit %d\n",
	       ARG_CMD(a), running ? "Running" : "Not running", unit);
	printk("%s:   Interval: %d usec\n",
	       ARG_CMD(a), interval);
	printk("%s:   Rate: %d\n",
	       ARG_CMD(a), rate);

	return CMD_OK;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Interval", PQ_INT | PQ_DFL, 0, &interval, 0);
    parse_table_add(&pt, "Rate", PQ_INT | PQ_DFL, 0, &rate, 0);

    if (parse_arg_eq(a, &pt) < 0) {
	printk("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
	parse_arg_eq_done(&pt);
	return CMD_FAIL;
    }

    parse_arg_eq_done(&pt);

    if ((c = ARG_GET(a)) != NULL) {
	if (sal_strcasecmp(c, "off") == 0) {
	    interval = 0;
	    rate = 0;
	} else if (!sal_strcasecmp(c, "on") == 0) {
	    return CMD_USAGE;
	}
    }

    if (interval == 0) {
	if ((rv = soc_mem_scan_stop(unit)) < 0) {
	    printk("soc_mem_scan_stop %d: ERROR: %s\n",
		   unit, soc_errmsg(rv));
	    return CMD_FAIL;
	}

	printk("%s: Stopped on unit %d\n", ARG_CMD(a), unit);
    } else {
	if ((rv = soc_mem_scan_start(unit, rate, interval)) < 0) {
	    printk("soc_mem_scan_start %d: ERROR: %s\n",
		   unit, soc_errmsg(rv));
	    return CMD_FAIL;
	}

	printk("%s: Started on unit %d\n", ARG_CMD(a), unit);
    }

    return CMD_OK;
}

#endif /* INCLUDE_MEM_SCAN */


void 
mem_watch_cb(int unit, soc_mem_t mem, uint32 flags, int copyno, int index_min, 
             int index_max, void *data_buffer, void *user_data)
{
    int i;
    
    if (INVALIDm == mem) {
        printk("Invalid memory....\n");
        return;
    }
    if (index_max < index_min) {
        printk("Wrong indexes ....\n");
        return;
    }
    if (NULL == data_buffer) {
        printk("Buffer is NULL .... \n");
        return;
    }
    for (i = 0; i <= (index_max - index_min); i++) {
        printk("Unit = %d, mem = %s (%d), copyno = %d index = %d, Entry - ", 
               unit, SOC_MEM_NAME(unit, mem), mem, copyno, i + index_min);
        soc_mem_entry_dump(unit,mem, (char *)data_buffer + i);
    }

    printk("\n");

    return;
}

cmd_result_t
mem_watch(int unit, args_t *a)
{
    soc_mem_t       mem;
    char		    *c, *memname;
    int             view_len, copyno;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }
    if (ARG_CNT(a) == 0) {
        return CMD_USAGE;
    }
    if (NULL == (c = ARG_GET(a))) {
        return CMD_USAGE;
    }

    /* Deal with VIEW:MEMORY if applicable */
    memname = strstr(c, ":");
    view_len = 0;
    if (memname != NULL) {
        memname++;
        view_len = memname - c;
    } else {
        memname = c;
    }

    if (parse_memory_name(unit, &mem, memname, &copyno) < 0) {
        printk("ERROR: unknown table \"%s\"\n",c);
        return CMD_FAIL;
    }

    if (NULL == (c = ARG_GET(a))) {
        return CMD_USAGE;
    }

    if (sal_strcasecmp(c, "off") == 0) {
        /* unregister call back */
        soc_mem_snoop_unregister(unit, mem);
    } else if (sal_strcasecmp(c, "read") == 0) {
        /* register callback with read flag */
       soc_mem_snoop_register(unit,mem, SOC_MEM_SNOOP_READ, mem_watch_cb, NULL);
    } else if (sal_strcasecmp(c, "write") == 0) {
        /* register callback with write flag */
      soc_mem_snoop_register(unit,mem, SOC_MEM_SNOOP_WRITE, mem_watch_cb, NULL); 
    } else {
        return CMD_USAGE;
    }

    return CMD_OK;
}