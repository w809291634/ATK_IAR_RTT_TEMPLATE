/*
 * vivi/drivers/serial/term.c: It's a simple serial termial.
 *
 * Copyright (C) 2002 MIZI Research, Inc.
 *
 * This code is GPL.
 *
 * Author: Janghoon Lyu <nandy@mizi.com>
 * Date  : $Date: 2004/02/04 06:22:25 $
 *
 * $Revision: 1.1.1.1 $
 */


#include "term.h"
#include "getcmd.h"
#include "printk.h"
#include "command.h"

extern char prompt[];
const char password[]="xpro";
const char iappassword[]="iapprogram";
const char verst[]=__TIME__;
const char versd[]=__DATE__;
char cmd_buf[MAX_CMDBUF_SIZE];
//extern unsigned short workmode;
void serial_terminal(void)
{
	for (;;) {
		printk("%s > ", prompt);
		getcmd(cmd_buf, MAX_CMDBUF_SIZE);

		/* execute a user command */
		if (cmd_buf[0])
			exec_string(cmd_buf);
	}
}
