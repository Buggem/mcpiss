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
#include <stdbool.h>
#include <stdint.h>

#include <time.h>
#include <sys/time.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <yyjson.h>

#include "rw.h"
#include "mcformat.h"
#include "main.h"

#include "java.h"


unsigned short PORT = 25565;
bool verbose = false;



void printVersion()
{
	printf("%s\n", VERSION_STRING);
	exit(1);
}
void printHelp(int argc, char** argv)
{
	printf("%s\n" USAGE_STRING "\n", VERSION_STRING, argc >= 1 ? argv[0] : "mcpiss");
	exit(1);
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
	uint32_t serv_ip = INADDR_LOOPBACK;

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
				serv_ip = (uint32_t)parsedip[0] << 24 |
					  (uint32_t)parsedip[1] << 16 |
					  (uint32_t)parsedip[2] << 8  |
					  (uint32_t)parsedip[3];
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
		if (strcmp(argv[i], "--version") == 0) printVersion();
		if (strcmp(argv[i], "--help"   ) == 0) printHelp(argc, argv);

		if (argvILen >= 2)
		if (argv[i][0] == '-' && argv[i][1] != '-')
		{
			for (size_t j = 1; j < argvILen; j++)
			{
				if (argv[i][j] == 'v') verbose = true;
				if (argv[i][j] == 'V') printVersion();
				if (argv[i][j] == 'H') printHelp(argc, argv);
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


	cs_handshake(sockfd, fakeDNS, serv_addr, serv_ip);
	cs_statusRequest(sockfd);

#ifdef CUSTOM_RESPONSE
	resStr = "{}";
	resLen = strlen(resStr);
#else
	resStr = sc_statusResponse(sockfd);
	resLen = strlen(resStr);
#endif
	resStr_doc = yyjson_read(resStr, resLen, 0);
	if(resStr_doc == NULL)
	{
		fprintf(stderr, "Likely not a Minecraft server - yyJSON could not read the JSON response:\n%s\nExiting...\n", resStr);
		return 1;
	}
	if(verbose) printf("%s\n", resStr);

	// Ping-pong before closing

	cs_pingRequest(sockfd);
	resPing = sc_pongResponse(sockfd);

	// Parse info with yyjson
	yyjson_val *resStr_root       = yyjson_doc_get_root(resStr_doc);

	yyjson_val *resStr_desc       = yyjson_obj_get(resStr_root, "description");
	yyjson_val *resStr_players    = yyjson_obj_get(resStr_root, "players");
	yyjson_val *resStr_playersLst = resStr_players != NULL ? yyjson_obj_get(resStr_players, "sample") : NULL;
	yyjson_val *resStr_playersCur = resStr_players != NULL ? yyjson_obj_get(resStr_players, "online") : NULL;
	yyjson_val *resStr_playersMax = resStr_players != NULL ? yyjson_obj_get(resStr_players, "max")    : NULL;
	yyjson_val *resStr_version    = yyjson_obj_get(resStr_root, "version");
	yyjson_val *resStr_versionStr = resStr_version != NULL ? yyjson_obj_get(resStr_version, "name")     : NULL;
	yyjson_val *resStr_versionPrt = resStr_version != NULL ? yyjson_obj_get(resStr_version, "protocol") : NULL;
	yyjson_val *resStr_secChat    = yyjson_obj_get(resStr_root, "enforcesSecureChat");

	// Print server info

	printf("==============================\n");
	{
	char* versionColored = resStr_versionStr != NULL ? toSTable(yyjson_get_str(resStr_versionStr)) : "\033[90mOld\033[0m";
	printf("%-20s: \033[35m%s\033[0m \033[36m(protocol v%d)\033[0m\n", "Version", versionColored, resStr_versionPrt != NULL ? yyjson_get_int(resStr_versionPrt) : -1);
	if (resStr_versionStr != NULL) free(versionColored);
	}

#define RS_DESC for (size_t i = 0; resStr_descStr[i] != 0; i++) \
		{ \
			if (resStr_descStr[i] == '\n') \
			{ \
				resStr_descStr[i] = 0; \
				resStr_descNL++; \
			} \
		} \
		\
		for (size_t p = 0, i = 0; i < resStr_descNL; i++, p += strlen(resStr_descStr + p) + 1) \
		{ \
			if (i == 0) printf("%-20s: ", "Description"); \
			else printf("%-20s: ", ""); \
		\
			printf("%s\033[0m\n", resStr_descStr + p); \
		}

	if (yyjson_is_str(resStr_desc))
	{
		const char* resStr_descStrC = yyjson_get_str(resStr_desc);
		char*  resStr_descStrColor  = toSTable(resStr_descStrC);
		if(resStr_descStrColor == NULL)
		{
			fprintf(stderr, "Could not convert with toSTable.\nExiting...\n");
			return 1;
		}
		size_t resStr_descStrLen    = strlen(resStr_descStrColor);
		char*  resStr_descStr       = calloc(resStr_descStrLen + 1, 1);
		if(resStr_descStr == NULL)
		{
			fprintf(stderr, "Could not calloc resStr_descStr.\nExiting...\n");
			return 1;
		}
		size_t resStr_descNL        = 1;

		memcpy(resStr_descStr, resStr_descStrColor, resStr_descStrLen + 1);
		free(resStr_descStrColor);


		RS_DESC

		free(resStr_descStr);
	} else if(resStr_desc != NULL)
	{
#ifndef NO_MCTEXT_COMPONENTS
		char* resStr_descStrNC = toSTableJSON(resStr_desc);
		if(resStr_descStrNC == NULL)
		{
			fprintf(stderr, "Could not convert with toSTableJSON.\nExiting...\n");
			return 1;
		}
		char* resStr_descStr = toSTable(resStr_descStrNC);
		if(resStr_descStr == NULL)
		{
			fprintf(stderr, "Could not convert with toSTable.\nExiting...\n");
			return 1;
		}
		size_t resStr_descNL = 1;


		free(resStr_descStrNC);


		RS_DESC

		free(resStr_descStr);
#else
		fprintf(stderr, "Description is pretty formatted. Thanks, Minecraft.\n");
#endif
	}

	printf("%-20s: %s%ums\033[0m\n", "Ping",
		 resPing >= MEDIUM_PING ?
		(resPing >= HIGH_PING ?
		(resPing >= SEVERE_PING
		? "\033[1;31m"
		: "\033[31m")
		: "\033[33m")
		: "\033[32m",
		(unsigned int)resPing);
	{
	int playersCur = resStr_playersCur != NULL ? yyjson_get_int(resStr_playersCur) : 0,
	    playersMax = resStr_playersMax != NULL ? yyjson_get_int(resStr_playersMax) : 0;
	printf("%-20s: %s", "Players",
		((resStr_playersCur != NULL) && (resStr_playersMax != NULL)) ?
		(playersCur >= ( playersMax      / 2) ?
		(playersCur >= ((playersMax * 3) / 4) ?
		(playersCur ==   playersMax
		? "\033[1;31m"
		: "\033[31m")
		: "\033[33m")
		: "") : ""
	);

	// get info on players
	if (resStr_playersCur != NULL)
		printf("%d", playersCur);
	else
		printf("???");
	if (resStr_playersMax != NULL)
		printf("/%d\033[0m\n", playersMax);
	else
		printf("/???\033[0m\n");

	// scan players
	if (resStr_playersLst != NULL)
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

			printf("%-9s%-10d : ", "", (int)idx);
			if (playerName != NULL)
			{
				char* playerNameStr    = toSTable(yyjson_get_str(playerName));
				printf("%-16s\033[0m", playerNameStr);
				if (playerId)
					printf(" {%s}", yyjson_get_str(playerId));
				printf("\n");
				free(playerNameStr);
			} else if(playerId != NULL)
			{
				printf("%-16s\033[0m {%s}\n", "Unnamed", yyjson_get_str(playerId));
			} else
				printf("No Information\n");
		}
	}

	}

	printf("%-20s: %s\033[0m\n", "Secure Chat", resStr_secChat == NULL ? "\033[90mUnspecified\033[0m" : (yyjson_get_bool(resStr_secChat) ? "\033[32mEnforced\033[0m" : "\033[31mOff\033[0m"));

	yyjson_doc_free(resStr_doc);
#ifndef CUSTOM_RESPONSE
	free(resStr);
#endif
	close(sockfd);

	return 0;
}
