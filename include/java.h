/*
    MCPISS - a prettier scanner of Minecraft servers
    Copyright (C) 2026  Max Parry

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <netinet/in.h>

#define JAVA_PROTOCOL_VERSION 776

#define JAVA_INTENT_STATUS   1
#define JAVA_INTENT_LOGIN    2
#define JAVA_INTENT_TRANSFER 3

void java_netInit(int* sockfd, struct sockaddr_in* serv_addr, uint32_t serv_ip);

void  java_cs_handshake(int sockfd, char* fakeDNS, struct sockaddr_in serv_addr, uint32_t serv_ip);
void  java_cs_statusRequest(int sockfd);
char* java_sc_statusResponse(int sockfd);
void  java_cs_pingRequest(int sockfd);
int   java_sc_pongResponse(int sockfd);

