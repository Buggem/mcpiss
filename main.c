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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <yyjson.h>

// compat - thanks, SO
#if __BIG_ENDIAN__
    #define htonll(x)   (x)
    #define ntohll(x)   (x)
#else
    #define htonll(x)   ((((uint64_t)htonl(x & 0xFFFFFFFF)) << 32) | htonl(x >> 32))
    #define ntohll(x)   ((((uint64_t)ntohl(x & 0xFFFFFFFF)) << 32) | ntohl(x >> 32))
#endif

unsigned short PORT = 25565;
bool verbose = false;
#define PROTOCOL_VERSION 776

#define INTENT_STATUS   1
#define INTENT_LOGIN    2
#define INTENT_TRANSFER 3

#define SEVERE_PING 300
#define   HIGH_PING 150
#define MEDIUM_PING 50

// fullreads
ssize_t rewr_full(int fildes, void *buf, size_t nbyte, ssize_t (*rewr)(int, void*, size_t))
{
	ssize_t bytesLeft = nbyte;

	while (bytesLeft != 0)
	{
		ssize_t result = rewr(fildes, buf + (nbyte - bytesLeft), bytesLeft);

		if (result == -1 && errno == EINTR) continue;
		if (result <=  0) return result;

		bytesLeft -= result;
	}

	return nbyte;
}
ssize_t read_full (int fildes, void *buf, size_t nbyte)
{
	return rewr_full(fildes, buf, nbyte, read );
}
ssize_t write_full(int fildes, const void *buf, size_t nbyte)
{
	return rewr_full(fildes, (void*)buf, nbyte, (ssize_t (*)(int, void*, size_t))write);
}


// Loosely based off the minecraft wiki's psuedocode imp - https://minecraft.wiki/w/Java_Edition_protocol/Packets#VarInt_and_VarLong
int readVarInt(int sockfd, int* result)
{
	unsigned int value = 0;

	for (int position = 0; position < 32; position += 7)
	{
		unsigned char currentByte = 0;
		size_t cB_read = read_full(sockfd, &currentByte, 1);

		if (cB_read == -1)
		{
			fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
			return 1;
		} else if (cB_read == 0)
		{
			fprintf(stderr, "Didn't read shit. Socket has probably prematurely closed\n");
			return 1;
		}

		value |= (unsigned int)(currentByte & 0x7f) << position;

		if((currentByte & 0x80) == 0)
		{
			// two's compliment should be handled by the cpu
			*result = (int)value;

			return 0;
		}
	}


	fprintf(stderr, "VarInt is too big.\n");
	return 1;
}

int writeVarInt(unsigned int value, int sockfd, bool onlyComputeLen)
{
	unsigned char computedLen = 0;
	unsigned char toWrite = 0;

	while ((value & ~0x7f) != 0)
	{
		toWrite = (unsigned char)(value & 0x7f) | 0x80;
		if (!onlyComputeLen) write_full(sockfd, &toWrite, 1);
		computedLen++;

		value >>= 7;
	}

	toWrite = (unsigned char)(value & 0xff);
	if (!onlyComputeLen) write_full(sockfd, &toWrite, 1);
	computedLen++;

	return computedLen;
}

// MC color coding + formatting
#define S_TABLE_CHAR '\xa7'
//char* sTableStr = "\xa7";
char* sTable[256];
void initSTable()
{
	memset(sTable, 0, sizeof(sTable));
	sTable['0'] = "\033[30m";
	sTable['1'] = "\033[34m";
	sTable['2'] = "\033[32m";
	sTable['3'] = "\033[36m";
	sTable['4'] = "\033[31m";
	sTable['5'] = "\033[35m";
	sTable['6'] = "\033[33m";
	sTable['7'] = "\033[37m";
	sTable['8'] = "\033[90m";
	sTable['9'] = "\033[94m";
	sTable['a'] = "\033[92m";
	sTable['b'] = "\033[96m";
	sTable['c'] = "\033[91m";
	sTable['d'] = "\033[95m";
	sTable['e'] = "\033[93m";
	sTable['f'] = "\033[97m";

	sTable['k'] = "x";
	sTable['l'] = "\033[1m";
	sTable['m'] = "\033[9m";
	sTable['n'] = "\033[4m";
	sTable['o'] = "\033[3m";
	sTable['r'] = "\033[0m";
}

char* toSTable(const char* curStr)
{
	char* coloredStr = NULL;
	size_t curStrLen = strlen(curStr);
	size_t newLen    = curStrLen;

	// get size of new string
	for (int i = 0; i < curStrLen; i++)
	{
		//printf("%-4u | %-2c - %-4u\n", (unsigned char)curStr[i], curStr[i], (unsigned char)S_TABLE_CHAR);
		if (i + 1 < curStrLen)
		if (curStr[i + 1] == S_TABLE_CHAR && (curStr[i] & 0x80) != 0)
		{
			newLen--;
			continue;
		}

		if (curStr[i] == S_TABLE_CHAR)
		if (i + 1 < curStrLen)
		{
			i++;
			//printf("%d\n", sTable[curStr[i]]);
			if (sTable[curStr[i]] != NULL)
			{
				newLen += strlen(sTable[curStr[i]]);
				newLen -= 2;
				//if (verbose) printf("[ %d -> %d ]\n", curStrLen, newLen);
			}

			continue;
		}
	}

	// Actually colour the string

	// P.S. - Yes, I do use Australian English.
	// It's only spelt "color" for consistency in programming.
	coloredStr = calloc(newLen + 1, 1);
	for (int i = 0; i < curStrLen; i++)
	{
		if (i + 1 < curStrLen)
		if (curStr[i + 1] == S_TABLE_CHAR && (curStr[i] & 0x80) != 0)
		{
			continue;
		}

		if (curStr[i] == S_TABLE_CHAR)
		if (i + 1 < curStrLen)
		{
			i++;
			if (sTable[curStr[i]] != NULL)
			{
				strcat(coloredStr, sTable[curStr[i]]);
			}

			continue;
		}

		// concat the char if it's not before the S_TABLE_CHAR and has a high flag set,
		// or S_TABLE_CHAR itself, or a valid sTable after S_TABLE_CHAR
		strncat(coloredStr, &curStr[i], 1);
	}

	return coloredStr;
}

int main(int argc, char** argv)
{
	char* resStr = NULL;
	int resLen = 0;
	int64_t resPing = 0;
	yyjson_doc *resStr_doc = NULL;
	char* fakeDNS = NULL;

	int sockfd = -1;
	struct sockaddr_in serv_addr;
	unsigned long serv_ip = INADDR_LOOPBACK;

	for (int i = 1; i < argc; i++) {
		if (argv[i] == NULL) break;
		size_t argvILen = strlen(argv[i]);

		if (strcmp(argv[i], "--ip") == 0)
		{
			if((i + 1) < argc)
			if(argv[i + 1] != NULL)
			{
				i++;


				// very complicated and overly confusing IP parsing script that will totally get hacked
				int j = 0;
				unsigned char parsedip[4] = {0, 0, 0, 0};

				for (int k = 0; k < 4; k++)
				{
					while (argv[i][j] != 0 && argv[i][j] != '.')
					{
						if (argv[i][j] < '0' || argv[i][j] > '9')
						{
							fprintf(stderr, "Non-numeric detected in IP! Exiting...\n");
							return 1;
						}
						unsigned long npipk = (unsigned long)parsedip[k] * 10 + (argv[i][j] - '0');
						if (npipk > 255)
						{
							fprintf(stderr, "IP overflow! Exiting...\n");
							return 1;
						}
						parsedip[k] = npipk;

						j++;
					}
					//printf("%c - %d\n", argv[i][j], parsedip[k]);

					if(argv[i][j] == 0)
					{
						//printf("Got 0\n");
						break;
					}

					j++;
				}
				serv_ip = (unsigned long)parsedip[0] << 24 |
					  (unsigned long)parsedip[1] << 16 |
					  (unsigned long)parsedip[2] << 8  |
					  (unsigned long)parsedip[3];
				//printf("%lu\n", serv_ip);

				continue;
			}

			break;
		}

		if (strcmp(argv[i], "--port") == 0)
		{
			if((i + 1) < argc)
			if(argv[i + 1] != NULL)
			{
				i++;
				PORT = 0;

				int j = 0;
				while (argv[i][j] != 0)
				{
					if (argv[i][j] < '0' || argv[i][j] > '9')
					{
						fprintf(stderr, "Non-numeric detected in port! Exiting...\n");
						return 1;
					}
					unsigned long nport = (unsigned long)PORT * 10 + (argv[i][j] - '0');
					if (nport > 65535)
					{
						fprintf(stderr, "Port range not between 0 and 65535! Exiting...\n");
						return 1;
					}
					PORT = nport;

					j++;
				}

				continue;
			}

			break;
		}

		if (strcmp(argv[i], "--dns") == 0)
		{
			if((i + 1) < argc)
			if(argv[i + 1] != NULL)
			{
				i++;
				fakeDNS = argv[i];

				continue;
			}

			break;
		}

		if (strcmp(argv[i], "--verbose") == 0) verbose = true;

		if (argvILen >= 2)
		if (argv[i][0] == '-' && argv[i][1] != '-')
		{
			for(int j = 1; j < argvILen; j++)
			{
				if(argv[i][j] == 'v') verbose = true;
				break;
			}
		}
	}

	// init sTable
	initSTable();

	// create and verify socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Could not create socket: %s\n", strerror(errno));
		return 1;
	}

	// setup serv addr
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(serv_ip);
	serv_addr.sin_port = htons(PORT);

	// connect the client to server
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0)
	{
		fprintf(stderr, "Could not connect to the server: %s\n", strerror(errno));
		return 1;
	} else
	{
		if(verbose) printf("Successfully established connection - shaking hands\n");
	}


	// TCP_CORK is only on Linux.
	// GNUobs cant fight me. This is called Linux, not GNU/Linux.
	unsigned int corked = 1;

	// cork (pause data)
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// I honestly hate this code.
	{
	unsigned int shakeLen = 0;

	char shakeAddr[255] = {'\0'};
	if(fakeDNS == NULL)
	{
		snprintf(shakeAddr, sizeof(shakeAddr), "%d.%d.%d.%d",
			(serv_ip >> 24) & 0xff,
			(serv_ip >> 16) & 0xff,
			(serv_ip >>  8) & 0xff,
			(serv_ip      ) & 0xff
		);
	} else {
		snprintf(shakeAddr, sizeof(shakeAddr), "%s",
			fakeDNS
		);
	}
	if(verbose) printf("%s\n", shakeAddr);
	unsigned int shakeAddr_len = strlen(shakeAddr);

	// Packet Header (no length, duh)

	shakeLen += writeVarInt(0,                sockfd, true);
	// Handshake
	shakeLen += writeVarInt(PROTOCOL_VERSION, sockfd, true);
	shakeLen += writeVarInt(shakeAddr_len,    sockfd, true);
	shakeLen += shakeAddr_len;
	shakeLen += 2;
	shakeLen += writeVarInt(INTENT_STATUS,    sockfd, true);


	// Packet Header
	writeVarInt(shakeLen,         sockfd, false);
	writeVarInt(0,                sockfd, false);
	// Handshake
	writeVarInt(PROTOCOL_VERSION, sockfd, false);
	writeVarInt(shakeAddr_len,    sockfd, false);
	write_full(sockfd, shakeAddr, shakeAddr_len);
	write_full(sockfd, &serv_addr.sin_port, 2);
	writeVarInt(INTENT_STATUS,    sockfd, false);

	}

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));


	// cork (pause data)
	corked = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// Ask server for a status
	{
	unsigned int shakeLen = 0;

	// Packet Header (no length, hurrrrrr durrrr)

	shakeLen += writeVarInt(0, sockfd, true);
	// There is no content for status_request


	writeVarInt(shakeLen, sockfd, false);
	writeVarInt(0,        sockfd, false);

	}

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// Get info
	{
	int packLen = 0, packId = 0;
	size_t res_read = -1;

	readVarInt(sockfd, &packLen);
	readVarInt(sockfd, &packId);

	readVarInt(sockfd, &resLen);
	if (verbose) printf("Response length: %d\n", resLen);

	if (resLen > 65535)
	{
		fprintf(stderr, "Likely not a Minecraft server - length of response is beyond 65535.\nExiting...\n");
		return 1;
	}
	resStr = calloc(resLen + 1, 1);
	if ((res_read = read_full(sockfd, resStr, resLen)) == -1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		return 1;
	} else if (res_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		return 1;
	}

	resStr_doc = yyjson_read(resStr, resLen, 0);
	if(resStr_doc == NULL)
	{
		fprintf(stderr, "Likely not a Minecraft server - yyJSON could not read the JSON response:\n%s\nExiting...\n", resStr);
		return 1;
	}
	if(verbose) printf("%s\n", resStr);
	}



	// Ping-pong before closing

	// cork (pause data)
	corked = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// Ping server
	{
	unsigned int shakeLen = 0;

	// Packet Header (no length, hurrrrrr durrrr)

	shakeLen += writeVarInt(1, sockfd, true);
	// Timestamp
	shakeLen += 8;

	// Packet Header
	writeVarInt(shakeLen, sockfd, false);
	writeVarInt(1,        sockfd, false);
	// Timestamp
	int64_t curMsec = 0;
	uint64_t ucurMsec = 0;
	struct timeval curTime;
	gettimeofday(&curTime, NULL);
	curMsec = (int64_t)curTime.tv_sec * 1000 + (int64_t)curTime.tv_usec / 1000;
	ucurMsec = htonll(curMsec);
	write_full(sockfd, &ucurMsec, 8);

	if(verbose) printf("Timestamp %-10s: %lu\n", "sent", curMsec);
	}

	// uncork (flush data)
	corked = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &corked, sizeof(corked));

	// Pong
	{
	int packLen = 0, packId = 0;

	readVarInt(sockfd, &packLen);
	readVarInt(sockfd, &packId);

	// Timestamp
	int64_t resMsec = 0;
	uint64_t uresMsec = 0;
	size_t res_read = -1;

	if ((res_read = read_full(sockfd, &uresMsec, 8)) == -1)
	{
		fprintf(stderr, "Could not read socket: %s\n", strerror(errno));
		return 1;
	} else if (res_read == 0)
	{
		fprintf(stderr, "Socket closed? Didn't read anything!\nExiting...\n");
		return 1;
	}

	// Read current timestamp
	int64_t curMsec = 0;

	struct timeval curTime;
	gettimeofday(&curTime, NULL);
	curMsec = (int64_t)curTime.tv_sec * 1000 + (int64_t)curTime.tv_usec / 1000;

	uresMsec = ntohll(uresMsec);
	resMsec = (int64_t)uresMsec;
	resPing = curMsec - resMsec;
	if(verbose) printf("Timestamp %-10s: %lu\n", "recieved", resMsec);
	if(verbose) printf("Timestamp %-10s: %lu\n", "right now", curMsec);
	}

	// Parse info with yyjson
	yyjson_val *resStr_root       = yyjson_doc_get_root(resStr_doc);

	yyjson_val *resStr_desc       = yyjson_obj_get(resStr_root, "description");
	yyjson_val *resStr_players    = yyjson_obj_get(resStr_root, "players");
	yyjson_val *resStr_playersLst = yyjson_obj_get(resStr_players, "sample");
	yyjson_val *resStr_playersCur = yyjson_obj_get(resStr_players, "online");
	yyjson_val *resStr_playersMax = yyjson_obj_get(resStr_players, "max");
	yyjson_val *resStr_version    = yyjson_obj_get(resStr_root, "version");
	yyjson_val *resStr_versionStr = yyjson_obj_get(resStr_version, "name");
	yyjson_val *resStr_versionPrt = yyjson_obj_get(resStr_version, "protocol");
	yyjson_val *resStr_secChat    = yyjson_obj_get(resStr_root, "enforcesSecureChat");
	int playersCur = yyjson_get_int(resStr_playersCur),
	    playersMax = yyjson_get_int(resStr_playersMax);

	// Print server info

	printf("==============================\n");
	printf("%-20s: \033[35m%s\033[0m \033[36m(protocol v%d)\033[0m\n", "Version", yyjson_get_str(resStr_versionStr), yyjson_get_int(resStr_versionPrt));
	{
	const char* resStr_descStrC = yyjson_get_str(resStr_desc);
	char*  resStr_descStrColor  = toSTable(resStr_descStrC);
	size_t resStr_descStrLen    = strlen(resStr_descStrColor);
	char*  resStr_descStr       = calloc(resStr_descStrLen + 1, 1);
	size_t resStr_descNL        = 1;

	memcpy(resStr_descStr, resStr_descStrColor, resStr_descStrLen + 1);
	free(resStr_descStrColor);

	for (size_t i = 0; resStr_descStr[i] != 0; i++)
	{
		if (resStr_descStr[i] == '\n')
		{
			resStr_descStr[i] = 0;
			resStr_descNL++;
		}
	}

	for (size_t p = 0, i = 0; i < resStr_descNL; i++, p += strlen(resStr_descStr + p) + 1)
	{
		if (i == 0)
			printf("%-20s: ", "Description");
		else
			printf("%-20s: ", "");

		printf("%s\033[0m\n", resStr_descStr + p);
	}

	free(resStr_descStr);
	}
	printf("%-20s: %s%ums\033[0m\n", "Ping",
		 resPing >= MEDIUM_PING ?
		(resPing >= HIGH_PING ?
		(resPing >= SEVERE_PING
		? "\033[1;31m"
		: "\033[31m")
		: "\033[33m")
		: "\033[32m",
		resPing);
	printf("%-20s: %s%d/%d\033[0m\n", "Players",
		 playersCur >= ( playersMax      / 2) ?
		(playersCur >= ((playersMax * 3) / 4) ?
		(playersCur ==   playersMax
		? "\033[1;31m"
		: "\033[31m")
		: "\033[33m")
		: "",
		playersCur,
		playersMax
	);

	{
	size_t idx, max;
	yyjson_val *player;
	yyjson_arr_foreach(resStr_playersLst, idx, max, player)
	{
		if(idx == 0)
		{
			printf("^--------,\n");
		}

		yyjson_val *playerName = yyjson_obj_get(player, "name");
		yyjson_val *playerId   = yyjson_obj_get(player, "id");
		char* playerNameStr    = toSTable(yyjson_get_str(playerName));
		printf("%-9s%-11d: %-16s\033[0m (%s)\n", "", (int)idx, playerNameStr, yyjson_get_str(playerId));
		free(playerNameStr);
	}
	}

	printf("%-20s: %s\033[0m\n", "Secure Chat", yyjson_get_bool(resStr_secChat) ? "\033[32mEnforced\033[0m" : "\033[31mOff\033[0m");

	yyjson_doc_free(resStr_doc);
	free(resStr);
	close(sockfd);

	return 0;
}
