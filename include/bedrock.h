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

#define BEDROCK_PROTOCOL_VERSION 776

#define BEDROCK_MCPE_STR "MCPE"
#define BEDROCK_MCEE_STR "MCEE"
typedef enum {
	BEDROCK_MCPE,
	BEDROCK_MCEE
} bedrockedt_t;

typedef struct {
	int ping;
	char* str;
	unsigned short len;
} bedrockpong_t;

typedef struct {
	bedrockedt_t edition;
	char* name;
	int protocol;
	char* version;
	int players;
	int max;
	uint64_t serverUUID;
	char* desc;
	char* gameModeStr;
	int one; // gamemode according to the wiki, but is the same between survival and creative?
	unsigned short port;
	unsigned short port6;
} bedrockinf_t;

extern unsigned char bedrock_magic[16];

void bedrock_netInit(int* sockfd, struct sockaddr_in* serv_addr, uint32_t serv_ip);

void          bedrock_cs_unconnectedPing(int sockfd);
bedrockpong_t bedrock_sc_unconnectedPong(int sockfd);
