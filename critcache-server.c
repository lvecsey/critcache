/*
    critcache server example using the critcache library
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

#include <sys/mman.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>

#include <signal.h>
#include <time.h>

#include "critbit.h"

#include "critcache.h"

int lookup_value(const char *str, void *extra) {

  char *value_result = (char*) extra;
  
  if (str != NULL) {

    char *rptr;

    rptr = str;

    printf("Looking at str %s\n", str);
    
    while (rptr[0]) {

      if (rptr[0] == '-' && rptr[1] == '>') {
	size_t len;
	rptr += 2;
	len = strlen(rptr);
	strncpy(value_result, rptr, len);
	value_result[len] = 0;
	return 0;
      }

      rptr++;
      
    }
      
  }
    
  return 0;

}

int sendresult(int s, char *buf, struct sockaddr_in *dest_addr, ccpkt_unpacked *pkt_unpacked, uint64_t retcmd) {

 ccpkt_unpacked sendpkt_unpacked;	  

 size_t len;

 socklen_t addrlen;

 int retval;
 
 memcpy(&sendpkt_unpacked, pkt_unpacked, sizeof(ccpkt_unpacked));

 sendpkt_unpacked.retpack.ret = retcmd;

 retval = ccfill_repack(&sendpkt_unpacked, buf, &len);
	  
 addrlen = sizeof(struct sockaddr_in);

 retval = sendto(s, buf, len, 0, dest_addr, addrlen);
 if (retval == -1) {
   perror("sendto");
   return -1;
 }

 printf("sendto returned %d\n", retval);

 return 0;

}

critbit0_tree cb = { .root = NULL };

int dump_records(const char *str, void *extra) {

  int *fd;

  fd = (int*) extra;

  {

    ssize_t bytes_read;

    size_t len;

    len = strlen(str);
    
    bytes_read = write(fd[0], str, len);
    if (bytes_read != len) {
      perror("write");
      return 0;
    }

    bytes_read = write(fd[0], "\n", 1);
    if (bytes_read != 1) {
      perror("write");
      return 0;
    }
    
  }
  
  return 1;

}

#define critdump_fn "./critdump/critcache_serverdump.txt"

int critdump_file(critbit0_tree *ct, char *out_fn) {

  char tmpout_fn[240];

  int fd;

  mode_t mode;
  
  int retval;
  
  retval = sprintf(tmpout_fn, "%s.new", out_fn);

  mode =  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  fd = open("./critdump/critcache_serverdump.txt.new", O_RDWR | O_CREAT | O_TRUNC, mode);

  if (fd == -1) {
    perror("open");
    return -1;
  }
  
  retval = critbit0_allprefixed(&cb, "", dump_records, &fd);

  retval = close(fd);
  if (retval == -1) {
    perror("close");
    return -1;
  }
  
  retval = rename(tmpout_fn, out_fn);

  if (retval == -1) {
    perror("rename");
    return -1;
  }
  
  return 0;
  
}

void signal_handler(int x, siginfo_t *info, void *extra) {

  ucontext_t *uc;

  int retval;

  fprintf(stderr, "%s: Entered signal handler.\n", __FUNCTION__);

  uc = (ucontext_t*) extra;

  retval = critdump_file(&cb, critdump_fn);
  if (retval == -1) {
    fprintf(stderr, "%s: Trouble dumping critbit tree to a file.\n", __FUNCTION__);
    return;
  }  
  
  return;

}

int main(int argc, char *argv[]) {

  int retval;
  int s;

  struct protoent *pent;

  long int input_ipaddress[4], input_port;
  
  char buf[1536];
  size_t len;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  struct sockaddr_in bind_addr, dest_addr;
  
  ssize_t bytes_read;

  unsigned char ip_address[4] = { 192, 168, 2, 14 };

  uint16_t port = 7375;

  int flags_nonblocking = MSG_DONTWAIT;

  int flags = 0;

  ccpkt_unpacked pkt_unpacked;

  ccpktcmd_hdr *cmdhdr;

  struct sigaction act;

  struct sigaction oldact;

  struct timespec start, now;

  struct timespec last_critdump;
  
  double elapsed;
  
  long int counter;
  
  retval = argc>1 ? sscanf(argv[1], "%ld.%ld.%ld.%ld:%ld", input_ipaddress+0, input_ipaddress+1, input_ipaddress+2, input_ipaddress+3, &input_port) : -1;

  if (retval != 5) {
    fprintf(stderr, "%s: Trouble with IP:PORT to send to.\n", __FUNCTION__);
    return -1;
  }

  ip_address[0] = input_ipaddress[0];
  ip_address[1] = input_ipaddress[1];
  ip_address[2] = input_ipaddress[2];
  ip_address[3] = input_ipaddress[3];  

  port = input_port;
  
  pent = getprotobyname("UDP");
  if (pent==NULL) {
    perror("getprotobyname");
    return -1;
  }

  s = socket(AF_INET, SOCK_DGRAM, pent->p_proto); 
  if (s==-1) {
    perror("socket");
    return -1;
  }

  memset(&bind_addr, 0, sizeof(bind_addr));

  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(port);
  memcpy(&bind_addr.sin_addr.s_addr, ip_address, 4); 
  
  retval = bind(s, &bind_addr, addrlen);

  cmdhdr = & (pkt_unpacked.cmdhdr);

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = signal_handler;
  act.sa_flags = SA_SIGINFO;
  sigaddset(&act.sa_mask, SIGUSR1);

  retval = sigaction(SIGUSR1, &act, &oldact);
  if (retval==-1) {
    perror("sigaction");
    return -1;
  }
  
  counter = 0;

  retval = clock_gettime(CLOCK_REALTIME, &start);
  if (retval == -1) {
    perror("clock_gettime");
    return -1;
  }
  
  last_critdump = start;
  
  for (;;) {

    retval = clock_gettime(CLOCK_REALTIME, &now);
    if (retval == -1) {
      perror("clock_gettime");
      return -1;
    }

    elapsed = (now.tv_sec - last_critdump.tv_sec);

    if (elapsed > 30) {

      retval = critdump_file(&cb, critdump_fn);
      if (retval == -1) {
	fprintf(stderr, "%s: Trouble dumping critbit tree to a file.\n", __FUNCTION__);
	return -1;
      }  
      
      last_critdump = now;
      
    }
    
    len = sizeof(buf);
    addrlen = sizeof(struct sockaddr_in);
    bytes_read = recvfrom(s, buf, len, flags, (struct sockaddr*) &dest_addr, &addrlen); 
    
    counter++;
    
    if (bytes_read >= cc_minpktlen) {
      
      retval = ccfill_unpack(buf, &pkt_unpacked);

      if (retval == -1) {
	fprintf(stderr, "%s: Trouble unpacking packet.\n", __FUNCTION__);
	continue;
      }

      printf("cmd %lu\n", cmdhdr->cmd);
      
      if (cmdhdr->cmd == CCGET_CMD) {

	char prefix[CC_MAXKEYLEN+3];
	
	char value_result[CC_MAXVALUELEN];
	
	printf("key %s\n", pkt_unpacked.key);

	retval = sprintf(prefix, "%s->", pkt_unpacked.key);
	
	memset(value_result, 0, CC_MAXVALUELEN);

	printf("Looking up prefix %s\n", prefix);
	
	retval = critbit0_allprefixed(&cb, prefix, lookup_value, value_result);

	fprintf(stderr, "%s: critbit0_allprefixed returned %d\n", __FUNCTION__, retval);
	
	switch(retval) {
	case 0:

	  printf("[%ld] Setting value into packet %s\n", counter, value_result);

	  pkt_unpacked.cmdhdr.cmd = CCADD_CMD;
	  pkt_unpacked.strvalue.len = strlen(value_result);
	  pkt_unpacked.value = value_result;

	  retval = sendresult(s, buf, &dest_addr, &pkt_unpacked, CCDONE_RET);

	  if (retval == -1) {
	    printf("%s: Trouble sending result.\n", __FUNCTION__);
	    return -1;
	  }

	  break;
	
	case 1:
	  
	case 2:

	  printf("[%ld] No result for critbit search.\n", counter);

	  pkt_unpacked.cmdhdr.cmd = CCADD_CMD;
	  pkt_unpacked.strvalue.len = 0;
	  pkt_unpacked.value = NULL;
	  
	  retval = sendresult(s, buf, &dest_addr, &pkt_unpacked, CCFAIL_RET);

	  if (retval == -1) {
	    printf("%s: Trouble sending result.\n", __FUNCTION__);
	    return -1;
	  }

	  break;
	  
	}
		
      }

      else if (cmdhdr->cmd == CCADD_CMD) {
	char strbuf[512];
	printf("key %s value %s\n", pkt_unpacked.key, pkt_unpacked.value);
	retval = sprintf(strbuf, "%s->%s", pkt_unpacked.key, pkt_unpacked.value);
	printf("Inserting key value of %s\n", strbuf);
	retval = critbit0_insert(&cb, strbuf);
	printf("critbit0_insert returned %d\n", retval);

	switch(retval) {
	case 0:

	  retval = sendresult(s, buf, &dest_addr, &pkt_unpacked, CCFAIL_RET);

	  if (retval == -1) {
	    printf("%s: Trouble sending result.\n", __FUNCTION__);
	    return -1;
	  }

	  break;

	case 1:
	case 2:

	  retval = sendresult(s, buf, &dest_addr, &pkt_unpacked, CCDONE_RET);

	  if (retval == -1) {
	    printf("%s: Trouble sending result.\n", __FUNCTION__);
	    return -1;
	  }

	  break;
	  
	}
	
      }

      else if (cmdhdr->cmd == CCDEL_CMD) {

	char strbuf[512];

	printf("key %s value %s\n", pkt_unpacked.key, pkt_unpacked.value);
	retval = sprintf(strbuf, "%s->%s", pkt_unpacked.key, pkt_unpacked.value);
	retval = critbit0_delete(&cb, strbuf);

	switch(retval) {
	case 0:

	  retval = sendresult(s, buf, &dest_addr, &pkt_unpacked, CCFAIL_RET);

	  if (retval == -1) {
	    printf("%s: Trouble sending result.\n", __FUNCTION__);
	    return -1;
	  }

	  break;

	case 1:

	  retval = sendresult(s, buf, &dest_addr, &pkt_unpacked, CCDONE_RET);

	  if (retval == -1) {
	    printf("%s: Trouble sending result.\n", __FUNCTION__);
	    return -1;
	  }

	  break;
	  
	}

      }
	
    }
    
    if (bytes_read <= 0) break;
  }
    
  return 0;

}
