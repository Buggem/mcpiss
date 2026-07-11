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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include <sys/time.h>
#include <time.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rw.h"
#include "main.h"

#include "java.h"

void java_netInit(int* sockfd, struct sockaddr_in* serv_addr, uint32_t serv_ip)
{
	// create and verify socket
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Could not create socket: %s\n", strerror(errno));
		exit(1);
	}

	// setup serv addr
	memset(serv_addr, 0, sizeof(*serv_addr));
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_addr.s_addr = htonl(serv_ip);
	serv_addr->sin_port = htons(PORT);

	// connect the client to server
	if (connect(*sockfd, (struct sockaddr*)serv_addr, sizeof(*serv_addr)) != 0)
	{
		fprintf(stderr, "Could not connect to the server: %s\n", strerror(errno));
		exit(1);
	} else
	{
		if(verbose) printf("Successfully established connection - shaking hands\n");
	}
}

void java_cs_handshake(int sockfd, char* fakeDNS, struct sockaddr_in serv_addr, uint32_t serv_ip)
{
	unsigned int shakeLen = 0;

	char shakeAddr[255] = {'\0'};
	unsigned int shakeAddr_len = 0;

	// TCP_CORK is only on Linux.
	// GNUobs cant fight me. This is called Linux, not GNU/Linux.
	unsigned int corked = 1;

	// cork (pause data)
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// I honestly hate this code.
	if(fakeDNS == NULL)
	{
		snprintf(shakeAddr, sizeof(shakeAddr), "%u.%u.%u.%u",
			(unsigned char)((serv_ip >> 24) & 0xff),
			(unsigned char)((serv_ip >> 16) & 0xff),
			(unsigned char)((serv_ip >>  8) & 0xff),
			(unsigned char)((serv_ip      ) & 0xff)
		);
	} else {
		snprintf(shakeAddr, sizeof(shakeAddr), "%s",
			fakeDNS
		);
	}
	if(verbose) printf("%s\n", shakeAddr);
	shakeAddr_len = strlen(shakeAddr);

	// Packet Header (no length, duh)

	shakeLen += writeVarInt(0,                sockfd, true);
	// Handshake
	shakeLen += writeVarInt(JAVA_PROTOCOL_VERSION, sockfd, true);
	shakeLen += writeVarInt(shakeAddr_len,    sockfd, true);
	shakeLen += shakeAddr_len;
	shakeLen += 2;
	shakeLen += writeVarInt(JAVA_INTENT_STATUS,    sockfd, true);


	// Packet Header
	writeVarInt(shakeLen,         sockfd, false);
	writeVarInt(0,                sockfd, false);
	// Handshake
	writeVarInt(JAVA_PROTOCOL_VERSION, sockfd, false);
	writeVarInt(shakeAddr_len,    sockfd, false);
	write_full(sockfd, shakeAddr, shakeAddr_len);
	write_full(sockfd, &serv_addr.sin_port, 2);
	writeVarInt(JAVA_INTENT_STATUS,    sockfd, false);

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

}

// C->S Ask server for a status
void java_cs_statusRequest(int sockfd)
{
	unsigned int shakeLen = 0;

	// cork (pause data)
	unsigned int corked = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// Packet Header (no length, hurrrrrr durrrr)

	shakeLen += writeVarInt(0, sockfd, true);
	// There is no content for status_request

	writeVarInt(shakeLen, sockfd, false);
	writeVarInt(0,        sockfd, false);

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));
}

// S->C Get info
char* java_sc_statusResponse(int sockfd)
{
	char* resStr = NULL;
	int resLen = 0, packLen = 0, packId = 0;
	ssize_t res_read = (ssize_t)-1;

	readVarInt(sockfd, &packLen);
	readVarInt(sockfd, &packId);

	readVarInt(sockfd, &resLen);
	if (verbose) printf("Response length: %d\n", resLen);

	if (resLen > 65535)
	{
		fprintf(stderr, "Likely not a Minecraft server - length of response is beyond 65535.\nExiting...\n");
		exit(1);
	}

	resStr = calloc(resLen + 1, 1);
	if (resStr == NULL)
	{
		fprintf(stderr, "Could not calloc resStr.\nExiting...\n");
		exit(1);
	}

	if ((res_read = read_full(sockfd, resStr, resLen)) == (ssize_t)-1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		exit(1);
	} else if (res_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		exit(1);
	}

	return resStr;
}

// Ping-pong before closing

// C->S Ping
void java_cs_pingRequest(int sockfd)
{
	unsigned int shakeLen = 0;

	int64_t curMsec = 0;
	uint64_t ucurMsec = 0;
	struct timeval curTime;

	// cork (pause data)
	unsigned int corked = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// Ping server

	// Packet Header (no length, hurrrrrr durrrr)

	shakeLen += writeVarInt(1, sockfd, true);
	// Timestamp
	shakeLen += 8;

	// Packet Header
	writeVarInt(shakeLen, sockfd, false);
	writeVarInt(1,        sockfd, false);
	// Timestamp
	gettimeofday(&curTime, NULL);
	curMsec = (int64_t)curTime.tv_sec * 1000 + (int64_t)curTime.tv_usec / 1000;
	ucurMsec = htonll(curMsec);
	write_full(sockfd, &ucurMsec, 8);

	if(verbose) printf("Timestamp %-10s: %lu\n", "sent", curMsec);

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));
}

// S->C Pong
int java_sc_pongResponse(int sockfd)
{
	int packLen = 0, packId = 0;

	int64_t resMsec = 0;
	uint64_t uresMsec = 0;
	ssize_t res_read = -1;

	int64_t curMsec = 0;

	struct timeval curTime;

	readVarInt(sockfd, &packLen);
	readVarInt(sockfd, &packId);

	// Timestamp
	if ((res_read = read_full(sockfd, &uresMsec, 8)) == (ssize_t)-1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		exit(1);
	} else if (res_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		exit(1);
	}

	// Read current timestamp
	gettimeofday(&curTime, NULL);
	curMsec = (int64_t)curTime.tv_sec * 1000 + (int64_t)curTime.tv_usec / 1000;

	uresMsec = ntohll(uresMsec);
	resMsec = (int64_t)uresMsec;
	if(verbose) printf("Timestamp %-10s: %lu\n", "recieved", resMsec);
	if(verbose) printf("Timestamp %-10s: %lu\n", "right now", curMsec);


	return curMsec - resMsec;
}
