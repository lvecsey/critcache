/*
    critcache library - higher level interface for use from library
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

#include <string.h>

#include "critcache.h"

#include "cceasy.h"

cceasy init_cceasy(char *bind_ipport, char *send_ipport) {

  cceasy cce;
  
  long int input_bindipaddress[4], input_bindport;
  long int input_sendipaddress[4], input_sendport;

  socklen_t addrlen;

  long int n;
  
  int retval;

  memset(&cce, 0, sizeof(cceasy));
  
  cce.state = 0;

  retval = sscanf(bind_ipport, "%ld.%ld.%ld.%ld:%ld", input_bindipaddress+0, input_bindipaddress+1, input_bindipaddress+2, input_bindipaddress+3, &input_bindport);

  if (retval != 5) {
    fprintf(stderr, "%s: Trouble with IP:PORT to bind from.\n", __FUNCTION__);
    return cce;
  }

  if (send_ipport != NULL) {
  
    retval = sscanf(send_ipport, "%ld.%ld.%ld.%ld:%ld", input_sendipaddress+0, input_sendipaddress+1, input_sendipaddress+2, input_sendipaddress+3, &input_sendport);

    if (retval != 5) {
      fprintf(stderr, "%s: Trouble with IP:PORT to send to.\n", __FUNCTION__);
      return cce;
    }

    cce.send_ipaddress[0] = input_sendipaddress[0];
    cce.send_ipaddress[1] = input_sendipaddress[1];
    cce.send_ipaddress[2] = input_sendipaddress[2];
    cce.send_ipaddress[3] = input_sendipaddress[3];  

    cce.send_port = input_sendport;
    
    memset(&(cce.dest_addr), 0, sizeof(cce.dest_addr));
    cce.dest_addr.sin_family = AF_INET;
    cce.dest_addr.sin_port = htons(cce.send_port);
    memcpy(&(cce.dest_addr.sin_addr.s_addr), cce.send_ipaddress, 4); 
    
  }

  cce.pent = getprotobyname("UDP");
  if (cce.pent==NULL) {
    perror("getprotobyname");
    return cce;
  }
  
  cce.s = socket(AF_INET, SOCK_DGRAM, (cce.pent)->p_proto); 
  if (cce.s==-1) {
    perror("socket");
    return cce;
  }

  cce.bind_ipaddress[0] = input_bindipaddress[0];
  cce.bind_ipaddress[1] = input_bindipaddress[1];
  cce.bind_ipaddress[2] = input_bindipaddress[2];
  cce.bind_ipaddress[3] = input_bindipaddress[3];  

  cce.bind_port = input_bindport;

  addrlen = sizeof(struct sockaddr_in);
  
  for (n = 0; n < 10; n++) {
  
    memset(&(cce.bind_addr), 0, sizeof(cce.bind_addr));
    cce.bind_addr.sin_family = AF_INET;
    cce.bind_addr.sin_port = htons(cce.bind_port+n);
    memcpy(&(cce.bind_addr.sin_addr.s_addr), cce.bind_ipaddress, 4); 

    retval = bind(cce.s, &(cce.bind_addr), addrlen);

    if (!retval) {
      break;
    }
      
  }

  if (n == 10) {
    perror("bind");
    return cce;
  }

  cce.buflen = 1536;
  
  cce.buf = malloc(cce.buflen);
  if (cce.buf == NULL) {
    perror("malloc");
    return cce;
  }

  cce.log_func = NULL;
  
  cce.init = 1;
  
  return cce;

}

int getreq_cceasy(cceasy *cce, char *key, char *fill_value) {

  ccpkt_unpacked pkt_unpacked;

  size_t len;

  socklen_t addrlen;

  int retval;
  
  addrlen = sizeof(struct sockaddr_in);

  pkt_unpacked.cmdhdr.cmd = CCGET_CMD;

  pkt_unpacked.strkey.len = strlen(key);
  pkt_unpacked.key = key;

  pkt_unpacked.strvalue.len = 0;
  pkt_unpacked.value = NULL;
	
  pkt_unpacked.retpack.ret = CCNONE_RET;
	
  retval = ccfill_repack(&pkt_unpacked, cce->buf, &len);

  retval = sendto(cce->s, cce->buf, len, 0, &(cce->dest_addr), addrlen);	

  fprintf(stderr, "Sendto crit_server len %lu retval %d\n", len, retval);
	
  if (retval == len) {

    long int counter;

    ssize_t bytes_read;
    
    counter = 0;
	  
    for (;counter < 10;) {

      ccpkt_unpacked confirm_unpacked;    

      ccpktcmd_hdr *confirm_cmdhdr;    

      struct sockaddr_in local_fromaddr;
	    
      int flags;

      flags = 0;

      addrlen = sizeof(struct sockaddr_in);
      bytes_read = recvfrom(cce->s, cce->buf, cce->buflen, flags, (struct sockaddr*) &local_fromaddr, &addrlen);

      counter++;

      if (bytes_read >= cc_minpktlen) {
      
	retval = ccfill_unpack(cce->buf, &confirm_unpacked);
	      
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble unpacking packet.\n", __FUNCTION__);
	  continue;
	}

	confirm_cmdhdr = & (confirm_unpacked.cmdhdr);

	if (confirm_cmdhdr->cmd != CCADD_CMD && confirm_cmdhdr->cmd != CCDEL_CMD) {
	  continue;
	}

	switch(confirm_unpacked.retpack.ret) {
	case CCDONE_RET:
	  strncpy(fill_value, confirm_unpacked.value, strlen(confirm_unpacked.value));
	  counter=10;
	  break;

	case CCFAIL_RET:
	  fill_value[0] = 0;
	  counter=10;
	  break;

	default:
	  counter=10;
	  break;
		
	}
      
      }
      
    }

  }

  return 0;

}

int addreq_cceasy(cceasy *cce, char *key, char *value) {

  ccpkt_unpacked pkt_unpacked;

  size_t len;

  socklen_t addrlen;

  int retval;
  
  addrlen = sizeof(struct sockaddr_in);

  pkt_unpacked.cmdhdr.cmd = CCADD_CMD;

  pkt_unpacked.strkey.len = strlen(key);
  pkt_unpacked.key = key;

  pkt_unpacked.strvalue.len = strlen(value);
  pkt_unpacked.value = value;
	
  pkt_unpacked.retpack.ret = CCNONE_RET;
	
  retval = ccfill_repack(&pkt_unpacked, cce->buf, &len);

  retval = sendto(cce->s, cce->buf, len, 0, &(cce->dest_addr), addrlen);	

  if (retval == len) {

    long int counter;

    ssize_t bytes_read;
    
    counter = 0;
	  
    for (;counter < 10;) {

      ccpkt_unpacked confirm_unpacked;    

      ccpktcmd_hdr *confirm_cmdhdr;    

      struct sockaddr_in local_fromaddr;
	    
      int flags;

      flags = 0;

      addrlen = sizeof(struct sockaddr_in);
      bytes_read = recvfrom(cce->s, cce->buf, cce->buflen, flags, (struct sockaddr*) &local_fromaddr, &addrlen);

      counter++;

      if (bytes_read >= cc_minpktlen) {
      
	retval = ccfill_unpack(cce->buf, &confirm_unpacked);
	      
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble unpacking packet.\n", __FUNCTION__);
	  continue;
	}

	confirm_cmdhdr = & (confirm_unpacked.cmdhdr);

	if (confirm_cmdhdr->cmd != CCADD_CMD && confirm_cmdhdr->cmd != CCDEL_CMD) {
	  continue;
	}

	switch(confirm_unpacked.retpack.ret) {
	case CCDONE_RET:
	  counter=10;
	  break;

	case CCFAIL_RET:
	  counter=10;
	  break;

	default:
	  counter=10;
	  break;
		
	}
      
      }
      
    }

  }

  return 0;

}

int delreq_cceasy(cceasy *cce, char *key, char *value) {

  ccpkt_unpacked pkt_unpacked;

  size_t len;

  socklen_t addrlen;

  int retval;
  
  addrlen = sizeof(struct sockaddr_in);

  pkt_unpacked.cmdhdr.cmd = CCDEL_CMD;

  pkt_unpacked.strkey.len = strlen(key);
  pkt_unpacked.key = key;

  pkt_unpacked.strvalue.len = strlen(value);
  pkt_unpacked.value = value;
	
  pkt_unpacked.retpack.ret = CCNONE_RET;
	
  retval = ccfill_repack(&pkt_unpacked, cce->buf, &len);

  retval = sendto(cce->s, cce->buf, len, 0, &(cce->dest_addr), addrlen);	

  if (retval == len) {

    long int counter;

    ssize_t bytes_read;
    
    counter = 0;
	  
    for (;counter < 10;) {

      ccpkt_unpacked confirm_unpacked;    

      ccpktcmd_hdr *confirm_cmdhdr;    

      struct sockaddr_in local_fromaddr;
	    
      int flags;

      flags = 0;

      addrlen = sizeof(struct sockaddr_in);
      bytes_read = recvfrom(cce->s, cce->buf, cce->buflen, flags, (struct sockaddr*) &local_fromaddr, &addrlen);

      counter++;

      if (bytes_read >= cc_minpktlen) {
      
	retval = ccfill_unpack(cce->buf, &confirm_unpacked);
	      
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble unpacking packet.\n", __FUNCTION__);
	  continue;
	}

	confirm_cmdhdr = & (confirm_unpacked.cmdhdr);

	if (confirm_cmdhdr->cmd != CCADD_CMD && confirm_cmdhdr->cmd != CCDEL_CMD) {
	  continue;
	}

	switch(confirm_unpacked.retpack.ret) {
	case CCDONE_RET:
	  counter=10;
	  break;

	case CCFAIL_RET:
	  counter=10;
	  break;

	default:
	  counter=10;
	  break;
		
	}
      
      }
      
    }

  }

  return 0;

}
