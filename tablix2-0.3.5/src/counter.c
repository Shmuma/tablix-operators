/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2004 Tomaz Solc                                      */

/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */

/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */

/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/* $Id: counter.c,v 1.9 2006-02-24 10:22:42 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "counter.h"
#include "error.h"
#include "gettext.h"
#include "assert.h"
#include "main.h"

#define MSG_FIFO_LEN	105
#define MSG_TTL		100

/* internal (don't touch these from outside) */
static struct timeval cnt_start;
static int cnt_last;
static int cnt_gens;
static int cnt_on;

/* external (set from outside before counter_init) */
int cnt_stopped;
int cnt_recv;
int cnt_total;
int cnt_newline;

typedef struct message_t message;

struct message_t {
	char *content;
	int sender;
	int numrecv;
	int ttl;
	
	message *next;
};

static message *msg_fifo=NULL;
static message *msg_fifo_write=NULL;
static message *msg_fifo_read=NULL;

void msg_update()
{
	message *cur;

	cur=msg_fifo_read;
	while(cur!=msg_fifo_write) {
		cur->ttl--;
		cur=cur->next;
	}

	while((msg_fifo_read->ttl<=0||msg_fifo_read->numrecv>=cnt_total)&&
					(msg_fifo_read!=msg_fifo_write)) {

		counter_clear();

		if(msg_fifo_read->numrecv>1) {
			printf("[xxxxx] ");
		} else {
			printf("[%05x] ", msg_fifo_read->sender);
		}
		
		printf("%s\n", msg_fifo_read->content);

		msg_fifo_read=msg_fifo_read->next;
	}
}

void msg_flush()
{
	message *cur;

	cur=msg_fifo_read;
	while(cur!=msg_fifo_write) {
		cur->ttl=0;
		cur=cur->next;
	}

	msg_update();
}

void msg_new(int sender, const char *fmt, ...)
{
	va_list ap;
	message *cur;
	int flag;
	char content[LINEBUFFSIZE];

	va_start(ap, fmt);

	vsnprintf(content, LINEBUFFSIZE, fmt, ap);
	
	va_end(ap);

	msg_update();

	cur=msg_fifo_read;
	flag=1;
	while(cur!=msg_fifo_write) {
		if((!strcmp(cur->content, content))&&(cur->sender!=sender)) {
			cur->numrecv++;
			flag=0;
		}
		cur=cur->next;
	}

	if(flag) {
		strcpy(msg_fifo_write->content, content);
		cur->numrecv=1;
		cur->sender=sender;
		cur->ttl=MSG_TTL;

		msg_fifo_write=msg_fifo_write->next;
		if(msg_fifo_write==msg_fifo_read) error("Message fifo full!");
	}
}

void counter_init()
{
	int n;
	message *cur;
	message *prev;

	assert(msg_fifo==NULL);

	gettimeofday(&cnt_start, NULL);
	cnt_gens=0;
	cnt_on=0;
	cnt_last=0;

	prev=NULL;
	for(n=0;n<MSG_FIFO_LEN;n++) {
		cur=malloc(sizeof(*msg_fifo)*MSG_FIFO_LEN);
		cur->content=malloc(sizeof(*cur->content)*LINEBUFFSIZE);

		if(prev!=NULL) {
			prev->next=cur;
		} else {
			msg_fifo=cur;
		}

		prev=cur;
	}
	prev->next=msg_fifo;

	msg_fifo_read=msg_fifo;
	msg_fifo_write=msg_fifo;
}

void counter_update(int sender, int fitness, int mandatory, int gens)
{
	float gpm;
	struct timeval now;
	int s;

	msg_update();

	cnt_gens++;
	gettimeofday(&now, NULL);

	if(!(now.tv_sec-cnt_last)) return;

	cnt_on=1;

	s=now.tv_sec-cnt_start.tv_sec;
	gpm=((float) cnt_gens*60)/s;

        printf(_("[%x] reports %d (%d) at %d"), sender, fitness, mandatory, gens);
	printf(_(", %.1f GPM"), gpm);
	printf(_(", %02d:%02d:%02d elapsed"), s/3600, (s/60)%60, s%60);
	printf(_(", %d/%d running "), cnt_total-cnt_stopped, cnt_total);
	printf("%c", cnt_newline?'\n':'\r');
	fflush(stdout);
	cnt_last=now.tv_sec;
}

void counter_clear()
{
	int n;
	if(cnt_on&&(!cnt_newline)) {
		for(n=0;n<80;n++) printf(" ");
		printf("\r");
		fflush(stdout);
		cnt_on=0;
	}
}
