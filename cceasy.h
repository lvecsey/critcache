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

#ifndef CCEASY_H
#define CCEASY_H

#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct {

  uint64_t state;

  unsigned char bind_ipaddress[4];
  unsigned char send_ipaddress[4];

  uint16_t bind_port;
  uint16_t send_port;

  struct protoent *pent;
  
  int s;
  
  struct sockaddr_in bind_addr, dest_addr;
  
  char *buf;

  size_t buflen;

  int (*log_func)(char *msg);
  
  int init : 1;
  
} cceasy;

cceasy init_cceasy(char *bind_ipport, char *send_ipport);

int addreq_cceasy(cceasy *cce, char *key, char *value);

int getreq_cceasy(cceasy *cce, char *key, char *fill_value);

int delreq_cceasy(cceasy *cce, char *key, char *value);

#endif
