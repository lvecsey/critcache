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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdint.h>

#include <string.h>
#include <errno.h>

#include "critcache.h"

int ccfill_unpack(char *pktbuf, ccpkt_unpacked *pkt_unpacked) {

  size_t len;
  
  memcpy(& (pkt_unpacked->cmdhdr.cmd), pktbuf, sizeof(uint64_t));

  pktbuf += sizeof(uint64_t);
  
  switch(pkt_unpacked->cmdhdr.cmd) {
  case CCGET_CMD:

    memcpy(& (pkt_unpacked->strkey.len), pktbuf, sizeof(uint64_t));

    pktbuf += sizeof(uint64_t);

    len = pkt_unpacked->strkey.len;
    pkt_unpacked->key = malloc(len + 1);
    if (pkt_unpacked->key == NULL) {
      perror("malloc");
      return -1;
    }

    memcpy(pkt_unpacked->key, pktbuf, len);
    pkt_unpacked->key[len] = 0;

    pktbuf += len;

    pkt_unpacked->strvalue.len = 0;
    pkt_unpacked->value = NULL;

    pkt_unpacked->retpack.ret = CCNONE_RET;

    return 0;

  case CCDEL_CMD:
    
  case CCADD_CMD:

    memcpy(& (pkt_unpacked->strkey.len), pktbuf, sizeof(uint64_t));

    pktbuf += sizeof(uint64_t);

    len = pkt_unpacked->strkey.len;
    if (len > CC_MAXKEYLEN) {
      fprintf(stderr, "%s: pkt says key len %ld\n", __FUNCTION__, len);
      return -1;
    }
    pkt_unpacked->key = malloc(len + 1);
    if (pkt_unpacked->key == NULL) {
      perror("malloc");
      return -1;
    }

    memcpy(pkt_unpacked->key, pktbuf, len);
    pkt_unpacked->key[len] = 0;

    pktbuf += len;

    memcpy(& (pkt_unpacked->strvalue.len), pktbuf, sizeof(uint64_t));

    pktbuf += sizeof(uint64_t);
    
    len = pkt_unpacked->strvalue.len;
    if (len > CC_MAXVALUELEN) {
      fprintf(stderr, "%s: pkt says value len %ld\n", __FUNCTION__, len);
      return -1;
    }
    pkt_unpacked->value = malloc(len + 1);
    if (pkt_unpacked->value == NULL) {
      perror("malloc");
      return -1;
    }

    memcpy(pkt_unpacked->value, pktbuf, len);
    pkt_unpacked->value[len] = 0;

    pktbuf += len;

    memcpy(& (pkt_unpacked->retpack.ret), pktbuf, sizeof(uint64_t));

    pktbuf += sizeof(uint64_t);
    
    return 0;

  default:

    fprintf(stderr, "%s: Unknown cmd %lu in packet.\n", __FUNCTION__, pkt_unpacked->cmdhdr.cmd);
    return -1;
    
  }
  
  return -1;
  
}

int ccfill_repack(ccpkt_unpacked *pkt_unpacked, char *buf, size_t *fill_len) {

  char *start_buf;

  start_buf = buf;
  
  memcpy(buf, & (pkt_unpacked->cmdhdr.cmd), sizeof(uint64_t));

  buf += sizeof(uint64_t);

  memcpy(buf, & (pkt_unpacked->strkey.len), sizeof(uint64_t));

  buf += sizeof(uint64_t);

  {

    long int keylen;

    keylen = pkt_unpacked->strkey.len;

    memcpy(buf, pkt_unpacked->key, keylen);

    buf += keylen;
    
  }
  
  if (pkt_unpacked->cmdhdr.cmd == CCADD_CMD || pkt_unpacked->cmdhdr.cmd == CCDEL_CMD) {

    memcpy(buf, & (pkt_unpacked->strvalue.len), sizeof(uint64_t));

    buf += sizeof(uint64_t);

    {

      long int valuelen;
      valuelen = pkt_unpacked->strvalue.len;
      memcpy(buf, pkt_unpacked->value, valuelen);

      buf += valuelen;
    
    }

  }

  else {

    uint64_t zerolen;

    zerolen = 0;
    
    memcpy(buf, &zerolen, sizeof(uint64_t));

    buf += sizeof(uint64_t);

  }
  
  memcpy(buf, & (pkt_unpacked->retpack.ret), sizeof(uint64_t));

  buf += sizeof(uint64_t);

  {
    
    size_t len;

    len = (buf - start_buf);

    *fill_len = len;

  }
    
  return 0;
  
}

int ccfill_retstr(uint64_t cmd, char *retstr) {

  char *str;

  switch(cmd) {
  case CCNONE_CMD:
    str = "CCNONE_CMD";
    break;
  case CCADD_CMD:
    str = "CCADD_CMD";
    break;
  case CCGET_CMD:
    str = "CCGET_CMD";
    break;
  case CCDEL_CMD:
    str = "CCDEL_CMD";
    break;
  default:
    str = "Unknown";
    break;
  }

  {
    size_t len;
    len = strlen(str);
    strncpy(retstr, str, len);
    retstr[len] = 0;
  }

  return 0;
  
}

