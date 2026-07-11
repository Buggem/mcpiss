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

#include <sys/types.h>
#include <stdbool.h>

// fullreads
ssize_t rewr_full(int fildes, void *buf, size_t nbyte, ssize_t (*rewr)(int, void*, size_t));
ssize_t read_full (int fildes, void *buf, size_t nbyte);
ssize_t write_full(int fildes, const void *buf, size_t nbyte);

int readVarInt(int sockfd, int* result);
int writeVarInt(unsigned int value, int sockfd, bool onlyComputeLen);
