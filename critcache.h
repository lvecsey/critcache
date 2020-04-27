/*
    critcache library - work with a critbit tree over UDP
    Copyright (C) 2020  Lester Vecsey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CRITCACHE_H
#define CRITCACHE_H

#include <stdint.h>

enum { CCNONE_CMD, CCADD_CMD, CCGET_CMD, CCDEL_CMD };

typedef struct {
  uint64_t cmd;
} ccpktcmd_hdr;

typedef struct {
  uint64_t len;
} ccpktlen_hdr;

#define CC_MAXKEYLEN 80
#define CC_MAXVALUELEN 400

enum { CCNONE_RET, CCDONE_RET, CCFAIL_RET };

typedef struct {
  uint64_t ret;
} ccpktret_pack;

typedef struct {

  ccpktcmd_hdr cmdhdr;
  ccpktlen_hdr strkey;
  char *key;
  ccpktlen_hdr strvalue;  
  char *value;
  ccpktret_pack retpack;
  
} ccpkt_unpacked;

#define cc_minpktlen (sizeof(uint64_t) * 3)

int ccfill_unpack(char *pktbuf, ccpkt_unpacked *pkt_unpacked);

int ccfill_repack(ccpkt_unpacked *pkt_unpacked, char *buf, size_t *fill_len);

int ccfill_retstr(uint64_t cmd, char *retstr);

#endif

