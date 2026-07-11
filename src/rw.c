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
#include <errno.h>
#include <stdbool.h>

#include "rw.h"

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
		ssize_t cB_read = read_full(sockfd, &currentByte, 1);

		if (cB_read == (ssize_t)-1)
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
