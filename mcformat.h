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

// MC color coding + formatting

#define S_TABLE_CHAR '\xa7'

#ifndef NO_MCTEXT_COMPONENTS
#define PRINT_STJSON { printf("======\n"); for(int i = 0; i < allTextLen; i++) printf("%-3d : 0x%08x\n", i, allText[i].val); }

typedef struct {
	char idx;
	char* colorName;
} idxtocol_t;

typedef struct {
	yyjson_val* val;
	char* str;
	size_t strSize;
} valsc_t;

extern idxtocol_t sTableJSON[17];
#endif

extern char* sTable[256];

void  initSTable();
char*   toSTable(const char* curStr);

#ifndef NO_MCTEXT_COMPONENTS
char* toSTableJSON_H(yyjson_val *curVal);
char* toSTableJSON(yyjson_val *curVal);
#endif
