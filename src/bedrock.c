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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include <sys/mman.h>

#include <sys/time.h>
#include <time.h>

#include <netinet/udp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rw.h"
#include "main.h"

#include "bedrock.h"

unsigned char bedrock_magic[16] = {
	0x00,
	0xff,
	0xff,
	0x00,
	0xfe,
	0xfe,
	0xfe,
	0xfe,
	0xfd,
	0xfd,
	0xfd,
	0xfd,
	0x12,
	0x34,
	0x56,
	0x78
};

void bedrock_netInit(int* sockfd, struct sockaddr_in* serv_addr, uint32_t serv_ip)
{
	// create and verify socket
	if ((*sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
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
		if(verbose) printf("Successfully established connection - pinged unconnected\n");
	}
}

void bedrock_cs_unconnectedPing(int sockfd)
{
	unsigned char packetId = 0x01;
	int64_t clientUUID = 0;

	int64_t curMsec = 0;
	struct timeval curTime;

	// cork (pause data)
	unsigned int corked = 1;
	setsockopt(sockfd, IPPROTO_UDP, UDP_CORK, &corked, sizeof(corked));

	if (verbose) printf("Sending unconnected ping\n");

	// Packet ID
	write_full(sockfd, &packetId, sizeof(packetId));

	// Timestamp
	gettimeofday(&curTime, NULL);
	curMsec = (int64_t)curTime.tv_sec * 1000 + (int64_t)curTime.tv_usec / 1000;
	write_full(sockfd, &curMsec, 8);

	// Magic (OFFLINE_MESSAGE_DATA_ID)
	write_full(sockfd, bedrock_magic, sizeof(bedrock_magic));

	// Client UUID
	write_full(sockfd, &clientUUID, 8);

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_UDP, UDP_CORK, &corked, sizeof(corked));
}

bedrockpong_t bedrock_sc_unconnectedPong(int sockfd)
{
	bedrockpong_t result = { 0, NULL, 0 };

	unsigned char packetId = 0;
	int64_t serverUUID = 0;
	unsigned char resMagic[16];

	char* resStr = NULL;
	unsigned short resLen = 0;

	int64_t curMsec  = 0;
	struct timeval curTime;
	int64_t resMsec  = 0;
	ssize_t res_read = -1, resStr_read = -1;

	int donebuffd = memfd_create("udpres", 0);
	char buf[1 + 8 + 8 + sizeof(bedrock_magic) + 2 + 65535];
	ssize_t donebuf_read = read(sockfd, buf, sizeof(buf));
	if (donebuf_read == (ssize_t)-1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		exit(1);
	} else if (donebuf_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		exit(1);
	}

	// check about the actual donebuf
	if (donebuffd < 0)
	{
		fprintf(stderr, "Couldn't create donebuffd: %s\nExiting...\n", strerror(errno));
		exit(1);
	}

	if (ftruncate(donebuffd, sizeof(buf)) < 0)
	{
		fprintf(stderr, "Couldn't truncate (expand) donebuffd: %s\nExiting...\n", strerror(errno));
		exit(1);
	}

	write(donebuffd, buf, sizeof(buf));
	lseek(donebuffd, 0, SEEK_SET);

	if (verbose) printf("Getting unconnected pong\n");

	// Packet ID
	read_full(donebuffd, &packetId, sizeof(packetId));

	if(packetId != 0x1c)
	{
		fprintf(stderr, "Packet ID is 0x%02x not 0x1c.\nExiting...\n", packetId);
		exit(1);
	}

	// Timestamp
	if ((res_read = read_full(donebuffd, &resMsec, sizeof(resMsec))) == (ssize_t)-1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		exit(1);
	} else if (res_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		exit(1);
	}

	// Client UUID
	read_full(donebuffd, &serverUUID, sizeof(serverUUID));

	// Magic (OFFLINE_MESSAGE_DATA_ID)
	read_full(donebuffd, resMagic, sizeof(resMagic));
	if (memcmp(bedrock_magic, resMagic, sizeof(bedrock_magic)) != 0)
	{
		fprintf(stderr, "Magic didn't match!\nExiting...\n");
		exit(1);
	}

	// Server ID String
	read_full(donebuffd, &resLen, sizeof(resLen));
	resLen = ntohs(resLen);
	if (verbose) printf("resStr len: %u\n");

	resStr = calloc(resLen+1, 1);
	if ((resStr_read = read_full(donebuffd, resStr, resLen)) == (ssize_t)-1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		exit(1);
	} else if (resStr_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		exit(1);
	}

	if (verbose) printf("%s\n", resStr);

	// Read current timestamp
	gettimeofday(&curTime, NULL);
	curMsec = (int64_t)curTime.tv_sec * 1000 + (int64_t)curTime.tv_usec / 1000;

	if (verbose) printf("Timestamp %-10s: %lu\n", "recieved", resMsec);
	if (verbose) printf("Timestamp %-10s: %lu\n", "right now", curMsec);

	close(donebuffd);

	result.ping = curMsec - resMsec;
	result.str  = resStr;
	result.len  = resLen;

	return result;
}
