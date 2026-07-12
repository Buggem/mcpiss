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
#include "bedrock.h"

unsigned short PORT = 25565;

bool verbose = false;
int varient = 0; // 0 - java, 1 - bedrock


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
	bedrockinf_t bedrockResInfo;

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

		if (strcmp(argv[i], "--bedrock") == 0)
		{
			varient = 1;
			PORT = 19132;
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

	memset(&bedrockResInfo, 0, sizeof(bedrockResInfo));

	// init sTable
	initSTable();

	if (varient == 0)
	{
		java_netInit(&sockfd, &serv_addr, serv_ip);

		java_cs_handshake(sockfd, fakeDNS, serv_addr, serv_ip);
		java_cs_statusRequest(sockfd);

#ifdef CUSTOM_RESPONSE
		resStr = "{}";
		resLen = strlen(resStr);
#else
		resStr = java_sc_statusResponse(sockfd);
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

		java_cs_pingRequest(sockfd);
		resPing = java_sc_pongResponse(sockfd);

	} else if (varient == 1)
	{
		bedrockpong_t result;

		int curI = 0, newI = 0;

		bedrock_netInit(&sockfd, &serv_addr, serv_ip);

		bedrock_cs_unconnectedPing(sockfd);
		result = bedrock_sc_unconnectedPong(sockfd);

		resPing = result.ping;

		for(int i = 0; i < result.len; i++) {
			if(result.str[i] == ';') result.str[i] = '\0';
		}

		if (curI < result.len)
		{
			if (memcmp(result.str + curI, BEDROCK_MCEE_STR, strlen(BEDROCK_MCEE_STR) < strlen(result.str) ? strlen(BEDROCK_MCEE_STR) : strlen(result.str)) == 0)
				bedrockResInfo.edition = BEDROCK_MCEE;
			else
				bedrockResInfo.edition = BEDROCK_MCPE;
		}
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.name = result.str + curI;
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.protocol = (int)strtol(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.version = result.str + curI;
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.players = (int)strtol(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.max = (int)strtol(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.serverUUID = strtoull(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.desc = result.str + curI;
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.gameModeStr = result.str + curI;
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.one = (int)strtol(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.port  = (unsigned short)strtoul(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		if (curI < result.len)
			bedrockResInfo.port6 = (unsigned short)strtoul(result.str + curI, NULL, 10);
		else
			exit(1);

		newI  = strlen(result.str + curI) + 1;
		curI += newI;
		if (verbose) printf("%d < %d\n", curI, result.len);

		/*
		printf("Education Edition: %d\nName: %s\nProtocol: %d\nVersion: %s\nPlayers: %d/%d\nServer UUID: %d\nDescription: %s\nGame mode: %s\n???: %d\nPort: %u (IPv6: %u)\n",
			bedrockResInfo.edition,
			bedrockResInfo.name,
			bedrockResInfo.protocol, bedrockResInfo.version,
			bedrockResInfo.players, bedrockResInfo.max,
			bedrockResInfo.serverUUID,
			bedrockResInfo.desc,
			bedrockResInfo.gameModeStr,
			bedrockResInfo.one,
			bedrockResInfo.port, bedrockResInfo.port6
		);
		*/
	}

	//if (varient != 0) return 1;

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
	char* versionColored =
		resStr_versionStr != NULL ? toSTable(yyjson_get_str(resStr_versionStr)) :
		((bedrockResInfo.version != NULL && varient == 1) ? bedrockResInfo.version : "\033[90mOld\033[0m");

	printf("%-20s: \033[35m%s\033[0m %s\033[36m(protocol v%d)\033[0m\n", "Version", versionColored,
		(bedrockResInfo.edition == BEDROCK_MCEE && varient == 1) ? "Education Edition " : "",
		resStr_versionPrt != NULL ? yyjson_get_int(resStr_versionPrt) :
		((bedrockResInfo.protocol != 0 && varient == 1) ? bedrockResInfo.protocol : -1)
	);

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
		if (resStr_descStrColor == NULL)
		{
			fprintf(stderr, "Could not convert with toSTable.\nExiting...\n");
			return 1;
		}
		size_t resStr_descStrLen    = strlen(resStr_descStrColor);
		char*  resStr_descStr       = calloc(resStr_descStrLen + 1, 1);
		if (resStr_descStr == NULL)
		{
			fprintf(stderr, "Could not calloc resStr_descStr.\nExiting...\n");
			return 1;
		}
		size_t resStr_descNL        = 1;

		memcpy(resStr_descStr, resStr_descStrColor, resStr_descStrLen + 1);
		free(resStr_descStrColor);


		RS_DESC

		free(resStr_descStr);
	} else if (resStr_desc != NULL)
	{
#ifndef NO_MCTEXT_COMPONENTS
		char* resStr_descStrNC = toSTableJSON(resStr_desc);
		if (resStr_descStrNC == NULL)
		{
			fprintf(stderr, "Could not convert with toSTableJSON.\nExiting...\n");
			return 1;
		}
		char* resStr_descStr = toSTable(resStr_descStrNC);
		if (resStr_descStr == NULL)
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
	} else if ((bedrockResInfo.name != NULL && bedrockResInfo.desc != NULL) && varient == 1)
	{
		char* resStr_descStrNC = calloc(strlen(bedrockResInfo.name) + 1 + strlen(bedrockResInfo.desc) + 1, 1);
		strcat(resStr_descStrNC, bedrockResInfo.name);
		strcat(resStr_descStrNC, "\n");
		strcat(resStr_descStrNC, bedrockResInfo.desc);

		if (resStr_descStrNC == NULL)
		{
			fprintf(stderr, "Could not convert with toSTableJSON.\nExiting...\n");
			return 1;
		}
		char* resStr_descStr = toSTable(resStr_descStrNC);
		if (resStr_descStr == NULL)
		{
			fprintf(stderr, "Could not convert with toSTable.\nExiting...\n");
			return 1;
		}
		size_t resStr_descNL = 1;


		free(resStr_descStrNC);


		RS_DESC

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
		(unsigned int)resPing);
	{
	int playersCur = resStr_playersCur != NULL ? yyjson_get_int(resStr_playersCur) : 0,
	    playersMax = resStr_playersMax != NULL ? yyjson_get_int(resStr_playersMax) : 0;
	printf("%-20s: %s", "Players",
		(resStr_playersCur != NULL && resStr_playersMax != NULL) ?
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
	else if (varient == 1)
		printf("%d", bedrockResInfo.players);
	else
		printf("???");
	if (resStr_playersMax != NULL)
		printf("/%d", playersMax);
	else if (varient == 1)
		printf("/%d", bedrockResInfo.max);
	else
		printf("/???");

	printf("\033[0m\n");
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

	if (varient == 0)
	printf("%-20s: %s\033[0m\n", "Secure Chat", resStr_secChat == NULL ? "\033[90mUnspecified\033[0m" : (yyjson_get_bool(resStr_secChat) ? "\033[32mEnforced\033[0m" : "\033[31mOff\033[0m"));

	if (resStr_doc != NULL) yyjson_doc_free(resStr_doc);
#ifndef CUSTOM_RESPONSE
	if (resStr != NULL) free(resStr);
#endif
	close(sockfd);

	return 0;
}
