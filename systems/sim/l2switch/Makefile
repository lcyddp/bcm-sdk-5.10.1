# $Id: Makefile 1.7 Broadcom SDK $
# $Copyright: Copyright 2011 Broadcom Corporation.
# This program is the proprietary software of Broadcom Corporation
# and/or its licensors, and may only be used, duplicated, modified
# or distributed pursuant to the terms and conditions of a separate,
# written license agreement executed between you and Broadcom
# (an "Authorized License").  Except as set forth in an Authorized
# License, Broadcom grants no license (express or implied), right
# to use, or waiver of any kind with respect to the Software, and
# Broadcom expressly reserves all rights in and to the Software
# and all intellectual property rights therein.  IF YOU HAVE
# NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
# IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
# ALL USE OF THE SOFTWARE.  
#  
# Except as expressly set forth in the Authorized License,
#  
# 1.     This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof,
# and to use this information only in connection with your use of
# Broadcom integrated circuit products.
#  
# 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
# PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
# OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
# 
# 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
# INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
# ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
# TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
# THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
# WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
# ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
#

LOCALDIR = systems/sim/l2switch

SDK :=$(shell if [ -n "$$SDK" ] ; then\
	echo $$SDK;\
	else\
	cd $(dir $(lastword $(MAKEFILE_LIST))); while /usr/bin/test ! -e RELEASE ; do \
	dir=`cd ../;pwd`;			\
	if [ "$$dir" = "/" ] ; then		\
	   echo Cannot find SDK in $(lastword $(MAKEFILE_LIST)) 1>&2;		\
	   exit 1;				\
	fi ;					\
	cd $$dir;				\
	done ;					\
	pwd;					\
	fi)

ifeq ($(SDK),)					
$(error Please run this in a tree)		
endif 				

export SDK

ifeq ($(INSTALL_DIR),)
INSTALL_DIR=.
endif

include ${SDK}/make/Make.config

#
# To use Purify, add a line here similar to the following:
#
#CC=/net/igor/export/home/pure/purify-4.2-solaris2/purify gcc

#
# Define all targets to build
#

L2SWITCH = ${INSTALL_DIR}/l2switch.${targetplat}${all_suffix}
L2SWITCH_LIB = ${LIBDIR}/libl2switch.a

all_targets = ${L2SWITCH} ${L2SWITCH_LIB}

#
# Define targets to be built by default.
#
build_targets = ${all_targets}

#
# Libraries
#

#DIAG_LIBS = \
#	${COND_LIB_EDITLINE} \
#	libdiag.a libtest.a \
#	${COND_LIB_TCL} \
#	libsal_appl.a libsal_appl_plat.a libplibde.a

L2SWITCH_LIBS =  ${APPL_LIBS} ${DRV_LIBS} ${DIAG_LIBS}
L2SWITCH_LIBS_BLD = $(addprefix ${BLDROOT}/,${L2SWITCH_LIBS})


all:: ${BLDDIR}/.tree ${build_targets}

# Libraries

.PHONY: _bcm_libraries

_bcm_libraries:
ifndef NO_BCM
	$(MAKE) -C ${SDK}
endif

# Example L2 Switch Application

${L2SWITCH}: _bde _bcm_libraries ${BOBJS} ${BLDDIR}/version.o
	@rm -f ${L2SWITCH}	# Prevent core dump if currently running
	${CC} -o ${L2SWITCH} ${BOBJS} ${BLDDIR}/version.o \
		${L2SWITCH_LIBS_BLD} \
		${L2SWITCH_LIBS_BLD} \
		${L2SWITCH_LIBS_BLD} \
		${LDFLAGS}

L2SWITCH_SRCS = \
	l2switch.c
L2SWITCH_OBJS = $(addsuffix .o, $(basename ${L2SWITCH_SRCS}))
L2SWITCH_BOBJS = $(addprefix ${BLDDIR}/,${L2SWITCH_OBJS})

${L2SWITCH_LIB}: ${L2SWITCH_BOBJS}
	${RM} $@
	$Q$(AR) ${ARFLAGS} $@ $(sort ${L2SWITCH_BOBJS})

# Build Version

${BLDDIR}/version.c: ${BLDDIR}/.tree
	$(MKVERS) > $@
${BLDDIR}/version.o: ${BLDDIR}/version.c
	${CC} ${CFLAGS} -c $< -o $@

# Make.depend is before clean:: so that Make.depend's clean:: runs first.

include ${SDK}/make/Make.depend

# Clean

clean:: ${BLDDIR}/.tree 
	rm -f ${all_targets}
	rm -f ${BOBJS}
	rm -f ${LIBDIR}/libl2switch.a
	rm -f ${BLDDIR}/version.c ${BLDDIR}/version.o
ifndef NO_BCM
	$(MAKE) -C ${SDK} clean
endif
	$(MAKE) -C ${SDK}/systems/bde/pli clean
	@# Rather than just removing the build directory, the following
	@# will detect any file missed by 'clean' so it can be fixed.
	@-find ${BLDROOT} -depth \
		\( -name .tree -exec rm {} \; \) -o \
		\( -type d -exec rmdir {} \; \) -o \
		-exec echo WARNING: make clean did not remove {} \; \
		-exec rm -f {} \;

distclean:: clean

.PHONY: _bde
_bde:
ifndef NO_BDE
	$(MAKE) -C ${SDK}/systems/bde/pli
endif
