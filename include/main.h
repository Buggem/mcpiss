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

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>

// compat - thanks, SO
#if __BIG_ENDIAN__
	#define htonll(x)   (x)
	#define ntohll(x)   (x)
#else
	#define htonll(x)   ((((uint64_t)htonl(x & 0xFFFFFFFF)) << 32) | htonl(x >> 32))
	#define ntohll(x)   ((((uint64_t)ntohl(x & 0xFFFFFFFF)) << 32) | ntohl(x >> 32))
#endif

extern unsigned short PORT;
extern bool verbose;


#define SEVERE_PING 300
#define   HIGH_PING 150
#define MEDIUM_PING 50


#define VERSION_STRING "MCPISS 1.1 - Bedrock Support Update"
#define   USAGE_STRING "Usage: %s [-h|--help] [-V|--version] [-v|--verbose] [--ip X.X.X.X|--port XXXXX|--dns hostname]"
