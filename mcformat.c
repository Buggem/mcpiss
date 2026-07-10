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

#include <yyjson.h>

#include "mcformat.h"

// MC color coding + formatting

char* sTable[256];

#ifndef NO_MCTEXT_COMPONENTS
idxtocol_t sTableJSON[17] = {
	{'0', "black"       },
	{'1', "dark_blue"   },
	{'2', "dark_green"  },
	{'3', "dark_aqua"   },
	{'4', "dark_red"    },
	{'5', "dark_purple" },
	{'6', "gold"        },
	{'7', "gray"        },
	{'8', "dark_gray"   },
	{'9', "blue"        },
	{'a', "green"       },
	{'b', "aqua"        },
	{'c', "red"         },
	{'d', "light_purple"},
	{'e', "yellow"      },
	{'f', "white"       },
	{0,   NULL          }
};
#endif

void initSTable()
{
	memset(sTable, 0, sizeof(sTable));
	sTable[(unsigned char)'0'] = "\033[30m";
	sTable[(unsigned char)'1'] = "\033[34m";
	sTable[(unsigned char)'2'] = "\033[32m";
	sTable[(unsigned char)'3'] = "\033[36m";
	sTable[(unsigned char)'4'] = "\033[31m";
	sTable[(unsigned char)'5'] = "\033[35m";
	sTable[(unsigned char)'6'] = "\033[33m";
	sTable[(unsigned char)'7'] = "\033[37m";
	sTable[(unsigned char)'8'] = "\033[90m";
	sTable[(unsigned char)'9'] = "\033[94m";
	sTable[(unsigned char)'a'] = "\033[92m";
	sTable[(unsigned char)'b'] = "\033[96m";
	sTable[(unsigned char)'c'] = "\033[91m";
	sTable[(unsigned char)'d'] = "\033[95m";
	sTable[(unsigned char)'e'] = "\033[93m";
	sTable[(unsigned char)'f'] = "\033[97m";

	sTable[(unsigned char)'k'] = "\033[46m";
	sTable[(unsigned char)'l'] = "\033[1m";
	sTable[(unsigned char)'m'] = "\033[9m";
	sTable[(unsigned char)'n'] = "\033[4m";
	sTable[(unsigned char)'o'] = "\033[3m";
	sTable[(unsigned char)'r'] = "\033[0m";
}

char* toSTable(const char* curStr)
{
	char* coloredStr = NULL;
	size_t curStrLen = strlen(curStr);
	size_t newLen    = curStrLen;

	// get size of new string
	for (size_t i = 0; i < curStrLen; i++)
	{
		//printf("%-4u | %-2c - %-4u\n", (unsigned char)curStr[i], curStr[i], (unsigned char)S_TABLE_CHAR);
		if (i + 1 < curStrLen)
		if (curStr[i + 1] == S_TABLE_CHAR && (curStr[i] & 0x80) != 0 && curStr[i] != S_TABLE_CHAR)
		{
			newLen--;
			continue;
		}

		if (curStr[i] == S_TABLE_CHAR)
		if (i + 1 < curStrLen)
		{
			i++;
			//printf("%d - %u\n", sTable[(unsigned char)curStr[i]], curStr[i]);
			if (sTable[(unsigned char)curStr[i]] != NULL)
			{
				newLen += strlen(sTable[(unsigned char)curStr[i]]);
				newLen -= 2;
				//printf("[ %d -> %d ]\n", curStrLen, newLen);
			}

			continue;
		}
	}

	// Actually colour the string

	// P.S. - Yes, I do use Australian English.
	// It's only spelt "color" for consistency in programming.
	coloredStr = calloc(newLen + 1, 1);
	if (coloredStr == NULL)
	{
		fprintf(stderr, "Could not calloc coloredStr for toSTable.\n");
		return NULL;
	}

	for (size_t i = 0; i < curStrLen; i++)
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
			if (sTable[(unsigned char)curStr[i]] != NULL)
			{
				strcat(coloredStr, sTable[(unsigned char)curStr[i]]);
			}

			continue;
		}

		// concat the char if it's not before the S_TABLE_CHAR and has a high flag set,
		// or S_TABLE_CHAR itself, or a valid sTable after S_TABLE_CHAR
		strncat(coloredStr, &curStr[i], 1);
	}

	return coloredStr;
}

#ifndef NO_MCTEXT_COMPONENTS
char* toSTableJSON_H(yyjson_val *curVal)
{
	size_t newLen = 0;
	char color = 0;
	char* result = NULL;

	if (curVal == NULL)
	{
		fprintf(stderr, "toSTableJSON: curVal is NULL.\n");
		return NULL;
	}
	yyjson_val *curVal_text = yyjson_obj_get(curVal, "text");
	yyjson_val *curVal_col  = yyjson_obj_get(curVal, "color");

	yyjson_val *curVal_bold = yyjson_obj_get(curVal, "bold");
	yyjson_val *curVal_itlc = yyjson_obj_get(curVal, "italic");
	yyjson_val *curVal_unln = yyjson_obj_get(curVal, "underlined");
	yyjson_val *curVal_strk = yyjson_obj_get(curVal, "strikethrough");
	yyjson_val *curVal_obfu = yyjson_obj_get(curVal, "obfuscated");

	if (!yyjson_is_obj(curVal))
	{
		if (yyjson_is_str(curVal))
		{
			newLen += strlen(yyjson_get_str(curVal));
			goto afterlen;
		} else
		{
			fprintf(stderr, "toSTableJSON: curVal is not a JSON object or string.\n");
			return NULL;
		}
	}

	if(curVal_text == NULL)
	{
		fprintf(stderr, "toSTableJSON: curVal does not follow text component rules.\n");
		return NULL;
	}

	// start adding to len
	newLen += strlen(yyjson_get_str(curVal_text));

	if (curVal_bold != NULL)
	if (yyjson_get_bool(curVal_bold))
		newLen += 3;
	if (curVal_itlc != NULL)
	if (yyjson_get_bool(curVal_itlc))
		newLen += 3;
	if (curVal_unln != NULL)
	if (yyjson_get_bool(curVal_unln))
		newLen += 3;
	if (curVal_strk != NULL)
	if (yyjson_get_bool(curVal_strk))
		newLen += 3;
	if (curVal_obfu != NULL)
	if (yyjson_get_bool(curVal_obfu))
		newLen += 3;

	if (curVal_col  != NULL)
	{
		const char* curVal_colStr = yyjson_get_str(curVal_col);
		for(int i = 0; sTableJSON[i].colorName != NULL; i++)
		{
			if(strcmp(curVal_colStr, sTableJSON[i].colorName) == 0)
			{
				//printf("Color: %s\n", sTableJSON[i].colorName);
				color = sTableJSON[i].idx;
				newLen += 3;
			}
		}
	}

	newLen += 3;

afterlen:

	//printf("new len: %d\n", newLen);
	result = calloc(newLen + 1, 1);
	if (result == NULL)
	{
		fprintf(stderr, "Could not calloc result for toSTableJSON_H.\n");
		return NULL;
	}

	//newLen += strlen(yyjson_get_str(curVal_text));

	if (curVal_bold != NULL)
	if (yyjson_get_bool(curVal_bold))
		strcat(result, "\xc2\xa7l");
	if (curVal_itlc != NULL)
	if (yyjson_get_bool(curVal_itlc))
		strcat(result, "\xc2\xa7o");
	if (curVal_unln != NULL)
	if (yyjson_get_bool(curVal_unln))
		strcat(result, "\xc2\xa7n");
	if (curVal_strk != NULL)
	if (yyjson_get_bool(curVal_strk))
		strcat(result, "\xc2\xa7m");
	if (curVal_obfu != NULL)
	if (yyjson_get_bool(curVal_obfu))
		strcat(result, "\xc2\xa7k");

	if (color != 0)
	{
		char resS[4] = {'\xc2', '\xa7', color, 0};
		strncat(result, resS, 3);
	}

	// current
	strcat(result, yyjson_is_str(curVal) ? yyjson_get_str(curVal) : yyjson_get_str(curVal_text));

	if (color != 0) strcat(result, "\xc2\xa7r");

	return result;
}


char* toSTableJSON(yyjson_val *curVal)
{
	// I hate this code. But it isn't recursive, so I love this code.
	valsc_t* allText = NULL;
	size_t allTextLen = 1; // null terminator
	size_t allTextLenOld = allTextLen;
	size_t aTColLen = 1;

	if (curVal == NULL)
	{
		fprintf(stderr, "toSTableJSON: curVal is NULL.\n");
		return NULL;
	}

	// add one, allocate, and set the old
	allTextLen++;
	allText = calloc(allTextLen, sizeof(valsc_t));

	allText[0].val = curVal;
	allText[0].str = toSTableJSON_H(curVal);
	allText[0].strSize = strlen(allText[0].str);

	allTextLenOld = allTextLen;
	//printf("curVal : 0x%08x\n", curVal);

	//PRINT_STJSON

	for(size_t i = 0; i < allTextLen - 1; i++)
	{
		// extra
		size_t iExtra = i + 1;
		yyjson_val *curVal_extra = yyjson_obj_get(allText[i].val, "extra");

		// if it aint null then it aint hull
		if (curVal_extra != NULL)
		{
			size_t idx, max = yyjson_arr_size(curVal_extra);
			yyjson_val *extra;

			{
				allTextLen += max;
				valsc_t* allTextNew = calloc(allTextLen, sizeof(valsc_t));

				if (allTextNew == NULL)
				{
					free(allText);
					free(allTextNew);
					return NULL;
				}

				memcpy(allTextNew, allText, iExtra * sizeof(valsc_t));
				memcpy(allTextNew + iExtra + max, allText + iExtra, (allTextLenOld - iExtra) * sizeof(valsc_t));

				free(allText);

				// all text
				allText = allTextNew;
				allTextLenOld = allTextLen;
			}

			yyjson_arr_foreach(curVal_extra, idx, max, extra) {

				//printf("set to %-4d : new len %-4d\n", (int)allTextLenOld-1, (int)allTextLen);

				allText[iExtra+idx].val = extra;
				allText[iExtra+idx].str = toSTableJSON_H(extra);
				allText[iExtra+idx].strSize = strlen(allText[iExtra+idx].str);

				//PRINT_STJSON
			}

		}

	}

	for(size_t i = 0; i < allTextLen - 1; i++)
		aTColLen += allText[i].strSize;

	char* result = calloc(aTColLen, 1);
	if (result == NULL)
	{
		fprintf(stderr, "Could not calloc result for toSTableJSON.\n");
		return NULL;
	}

	for(size_t i = 0; i < allTextLen - 1; i++)
	{
		if(allText[i].str != NULL)
		{
			strcat(result, allText[i].str);
			free(allText[i].str);
		}
	}

	free(allText);


	return result;
}

#ifdef MCFORMAT_TEST
int main(int argc, char** argv)
{
	const char *json = "{\"color\":\"dark_red\",\"extra\":[\"\\n\",{\"color\":\"gray\",\"extra\":[\" \",{\"color\":\"green\",\"text\":\"more RAM\"},\" \",\"for\",\" \",{\"bold\":true,\"text\":\"free!\"},{\"color\":\"dark_gray\",\"text\":\" \\u003e \"},{\"underlined\":true,\"color\":\"green\",\"clickEvent\":{\"action\":\"open_url\",\"value\":\"https://craft.link/ram\"},\"text\":\"craft.link/ram\"}],\"text\":\"Get this server\"}],\"text\":\"Server not found.\"}";
	initSTable();

	// Read JSON and get root
	yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
	yyjson_val *root = yyjson_doc_get_root(doc);

	char* stJSON = toSTableJSON(root);
	char* stColor = stJSON == NULL ? NULL : toSTable(stJSON);
	if(stJSON != NULL)
	{
		printf("%s\n", stJSON);
		free(stJSON);
		if(stColor != NULL) printf("%s\n", stColor);
	}

	// Free the doc
	yyjson_doc_free(doc);
}
#endif
#endif
