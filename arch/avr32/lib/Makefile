#
# (C) Copyright 2002-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2004-2006 Atmel Corporation
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y	+= memset.o
ifndef CONFIG_SYS_GENERIC_BOARD
obj-y	+= board.o
endif
obj-$(CONFIG_CMD_BOOTM) += bootm.o
obj-y	+= interrupts.o
obj-y	+= dram_init.o
