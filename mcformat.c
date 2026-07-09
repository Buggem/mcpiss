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

#include <string.h>
#include <stdlib.h>

#include "mcformat.h"

// MC color coding + formatting

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
