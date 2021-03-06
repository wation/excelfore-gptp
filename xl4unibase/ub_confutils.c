/*
 * excelfore-gptp - Implementation of gPTP(IEEE 802.1AS)
 * Copyright (C) 2019 Excelfore Corporation (https://excelfore.com)
 *
 * This file is part of excelfore-gptp.
 *
 * excelfore-gptp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * excelfore-gptp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with excelfore-gptp.  If not, see
 * <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
 */
#include "unibase_private.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "ub_confutils.h"

static char *conf_readline(FILE *inf)
{
	char a;
	static char line[128];
	int res;
	int rp=0;
	while((res=fread(&a, 1, 1, inf))){
		if(res<0){
			UB_LOG(UBL_WARN,"%s:error to read config file\n", __func__);
			return NULL;
		}
		if(a=='\n') break;
		line[rp++]=a;
		if(rp>=sizeof(line)-1){
			UB_LOG(UBL_WARN,"%s:too long line\n", __func__);
			return NULL;
		}
	}
	line[rp]=0;
	if(res>0 || strlen(line)>0) return line;
	return NULL;
}

static char *find_next_wspace(char *sp)
{
	bool inquote=false;
	while(*sp){
		if(*sp=='"') {
			inquote=inquote?false:true;
		}else if(!inquote && (*sp==' ' || *sp=='\t')) {
			return sp;
		}
		sp++;
	}
	return NULL;
}

static char *find_next_nwspace(char *sp)
{
	while(*sp){
		if(*sp!=' ' && *sp!='\t') return sp;
		sp++;
	}
	return NULL;
}

static uint8_t *get_byte_array(char *sp)
{
	int i;
	char *np;
	static uint8_t b[16];
	memset(b, 0, 16);
	for(i=0;i<16;i++){
		b[i]=strtol(sp, &np, 16);
		if(*np!=':') return b;
		sp=np+1;
	}
	return NULL;
}

static int config_set_oneitem(char *itemp, char *sp, ub_set_item_cb_t set_item)
{
	int64_t v;
	if(strchr(sp,':')){
		uint8_t *p;
		p=get_byte_array(sp);
		if(p) return set_item(itemp, p);
		return -1;
	}
	if(sp[0]=='"'){
		sp++;
		if(sp[strlen(sp)-1]!='"') return -1;
		sp[strlen(sp)-1]=0;
		return set_item(itemp, sp);
	}
	v=strtol(sp, NULL, 0);
	return set_item(itemp, &v);
}

int ub_read_config_file(char *fname, ub_set_item_cb_t set_item)
{
	FILE *inf;
	char *line;
	char *sp,*ep, *itemp;
	//int item;
	inf=fopen(fname, "r");
	if(!inf){
		UB_LOG(UBL_WARN,"%s:can't read config file:%s\n", __func__, fname);
		return -1;
	}
	while((line=conf_readline(inf))){
		sp=find_next_nwspace(line);
		if(!sp) continue;
		if(sp[0]=='#') continue;
		ep=find_next_wspace(sp);
		if(!ep) continue;
		*ep=0;
		itemp=sp;
		sp=ep+1;
		sp=find_next_nwspace(sp);
		if(!sp) continue;
		ep=find_next_wspace(sp);
		if(ep) *ep=0;
		if(!config_set_oneitem(itemp, sp, set_item)){
			UB_LOG(UBL_INFO,"%s:configured:%s\n", __func__, line);
		}else{
			UB_LOG(UBL_WARN,"%s:can't process this line:%s\n", __func__, line);
		}
	}
	fclose(inf);
	return 0;
}
