/*
    critcache client example using the critcache library
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
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>

#include <signal.h>

#include "critcache.h"

void action_alarm(int a, siginfo_t *b, void *c) {

  printf("FAIL");
  exit(-1);
  
}

int main(int argc, char *argv[]) {

  long int input_bindipaddress[4], input_bindport;
  long int input_sendipaddress[4], input_sendport;  

  unsigned char bind_ipaddress[4];
  unsigned char send_ipaddress[4];  
  
  uint16_t bind_port;
  uint16_t send_port;  
  
  struct protoent *pent;
  
  char buf[1536];
  size_t len;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  struct sockaddr_in bind_addr, dest_addr;

  char *cmdstr;
  
  int flags_nonblocking = MSG_DONTWAIT;

  int flags = 0;

  int retval;

  ssize_t bytes_read, bytes_written;

  long int n;
  
  int s;

  long int counter;

  ccpkt_unpacked pkt_unpacked;

  ccpktcmd_hdr *cmdhdr;

  struct timespec start, end;

  struct sigaction act;
  
  retval = argc>1 ? sscanf(argv[1], "%ld.%ld.%ld.%ld:%ld", input_bindipaddress+0, input_bindipaddress+1, input_bindipaddress+2, input_bindipaddress+3, &input_bindport) : -1;

  if (retval != 5) {
    fprintf(stderr, "%s: Trouble with IP:PORT to bind from.\n", __FUNCTION__);
    return -1;
  }

  retval = argc>2 ? sscanf(argv[2], "%ld.%ld.%ld.%ld:%ld", input_sendipaddress+0, input_sendipaddress+1, input_sendipaddress+2, input_sendipaddress+3, &input_sendport) : -1;

  if (retval != 5) {
    fprintf(stderr, "%s: Trouble with IP:PORT to send to.\n", __FUNCTION__);
    return -1;
  }
  
  cmdstr = argc>3 ? argv[3] : NULL;

  cmdhdr = & (pkt_unpacked.cmdhdr);
  
  if (!strncmp(cmdstr, "get", 3)) {
    cmdhdr->cmd = CCGET_CMD;
  }

  else if (!strncmp(cmdstr, "add", 3)) {
    cmdhdr->cmd = CCADD_CMD;
  }

  else if (!strncmp(cmdstr, "del", 3)) {
    cmdhdr->cmd = CCDEL_CMD;
  }
  
  else {
    fprintf(stderr, "%s: Trouble with cmd, must be get or add.\n", __FUNCTION__);
    return -1;
  }

  pkt_unpacked.key = argc>4 ? argv[4] : NULL;
  pkt_unpacked.value = argc>5 ? argv[5] : NULL;

  if (pkt_unpacked.key != NULL) {
    pkt_unpacked.strkey.len = strlen(pkt_unpacked.key);
  }

  if (pkt_unpacked.value != NULL) {
    pkt_unpacked.strvalue.len = strlen(pkt_unpacked.value);
  }

  pkt_unpacked.retpack.ret = CCNONE_RET;
  
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

  bind_ipaddress[0] = input_bindipaddress[0];
  bind_ipaddress[1] = input_bindipaddress[1];
  bind_ipaddress[2] = input_bindipaddress[2];
  bind_ipaddress[3] = input_bindipaddress[3];  

  bind_port = input_bindport;
  
  for (n = 0; n < 10; n++) {
  
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(bind_port+n);
    memcpy(&bind_addr.sin_addr.s_addr, bind_ipaddress, 4); 
  
    retval = bind(s, &bind_addr, addrlen);

    if (!retval) break;
    
  }

  retval = clock_gettime(CLOCK_REALTIME, &start);
  if (retval == -1) {
    perror("clock_gettime");
    return -1;
  }

  send_ipaddress[0] = input_sendipaddress[0];
  send_ipaddress[1] = input_sendipaddress[1];
  send_ipaddress[2] = input_sendipaddress[2];
  send_ipaddress[3] = input_sendipaddress[3];  

  send_port = input_sendport;
  
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(send_port);
  memcpy(&dest_addr.sin_addr.s_addr, send_ipaddress, 4); 

  addrlen = sizeof(struct sockaddr_in);

  retval = ccfill_repack(&pkt_unpacked, buf, &len);

  if (retval == -1) {
    fprintf(stderr, "%s: Trouble assembling packet to send.\n", __FUNCTION__);
    return -1;
  }

  retval = sendto(s, buf, len, 0, &dest_addr, addrlen);

  if (retval != len) {
    fprintf(stderr, "%s: Failed to send packet of len %ld\n", __FUNCTION__, len);
    return -1;
  }

  memset(&act, 0, sizeof(struct sigaction));

  act.sa_handler = NULL;
  act.sa_sigaction = action_alarm;
  act.sa_flags = 0;
  act.sa_restorer = NULL;
  
  retval = sigaction(SIGALRM, &act, NULL);
  if (retval == -1) {
    perror("sigaction");
    return -1;
  }
  
  alarm(20);

  counter = 0;

  printf("Waiting for confirmation packet..\n");

  for (;counter < 10;) {

    ccpkt_unpacked confirm_unpacked;    

    ccpktcmd_hdr *confirm_cmdhdr;    

    len = sizeof(buf);
    addrlen = sizeof(struct sockaddr_in);
    bytes_read = recvfrom(s, buf, len, flags, (struct sockaddr*) &dest_addr, &addrlen); 

    counter++;
    
    if (bytes_read >= cc_minpktlen) {
      
      retval = ccfill_unpack(buf, &confirm_unpacked);

      if (retval == -1) {
	fprintf(stderr, "%s: Trouble unpacking packet.\n", __FUNCTION__);
	continue;
      }

      retval = clock_gettime(CLOCK_REALTIME, &end);
      if (retval == -1) {
	perror("clock_gettime");
	return -1;
      }

      {
	double elapsed;
	elapsed = end.tv_sec;
	elapsed += end.tv_nsec / 1.0e9;
	elapsed -= start.tv_nsec / 1.0e9;
	elapsed -= start.tv_sec;
	printf("Elapsed time for packet transfer: %gs\n", elapsed);
      }
	
      confirm_cmdhdr = & (confirm_unpacked.cmdhdr);

      {
	char retstr[40];
	retval = ccfill_retstr(confirm_cmdhdr->cmd, retstr);
	printf("Confirmation cmd: %s\n", retstr);
      }
	
      if (confirm_cmdhdr->cmd != CCADD_CMD && confirm_cmdhdr->cmd != CCDEL_CMD) {
	printf("Skipping confirmation cmd %lu\n", confirm_cmdhdr->cmd);
	continue;
      }

      switch(confirm_unpacked.retpack.ret) {
      case CCDONE_RET:
	printf("ReturnCode: Done\n");
	printf("Value: %s\n", confirm_unpacked.value);
	printf("SUCCESS");
	return 0;

      case CCFAIL_RET:
	printf("ReturnCode: Problem.\n");
	printf("SUCCESS");
	return 0;

      default:
	printf("Retpack ret %lu\n", confirm_unpacked.retpack.ret);
	printf("FAIL");
	return -1;
	
      }
      
    }
      
  }
  
  printf("FAIL");
  
  return -1;

}
