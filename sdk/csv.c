/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/csv.h>

#ifdef _HOST

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
	u32 column;
	const char* string;
} ColumnName;

ColumnName column_names[] = {
	{COLUMN_NAME    , "NAME"    },
	{COLUMN_TYPE    , "TYPE"    },
	{COLUMN_COMMENT , "COMMENT" },
	{COLUMN_SIZE    , "SIZE"    },
	{COLUMN_CORE    , "CORE"    },
	{COLUMN_SPCORE  , "SPCORE"  },
	{COLUMN_MPCORE  , "MPCORE"  },
	{COLUMN_FRONTEND, "FRONTEND"},
	{COLUMN_FRONTBIN, "FRONTBIN"}
};

SymbolTable parse_table(Buffer input)
{
	SymbolTable table = {};
	memset(table.levels, 0xff, sizeof(table.levels));
	
	Column columns[MAX_COLUMNS];
	
	const char* ptr = input.data;
	
	/* First we iterate over all of the headings and record information about
	   what is stored in each of the columns, and how each of the levels relate
	   to the overlays. */
	u32 column_count = parse_table_header(&ptr, &table, columns, TRUE);
	
	/* Each remaining line in the table file will be a symbol. */
	const char* backup_ptr = ptr;
	while (ptr < input.data + input.size)
		if (*ptr++ == '\n')
			table.symbol_count++;
	ptr = backup_ptr;
	
	table.symbols = checked_malloc(table.symbol_count * sizeof(Symbol));
	memset(table.symbols, 0, table.symbol_count * sizeof(Symbol));
	
	u32 symbol = 0;
	u32 column = 0;
	
	/* Finally we iterate over each of the symbols and fill in all the names,
	   sizes, and addresses from the rows in the CSV file. */
	while (ptr < input.data + input.size)
	{
		if (*(ptr - 1) == '\n')
		{
			column = 0;
			
			while (*ptr != '\n' && ptr < input.data + input.size)
			{
				CHECK(column < column_count, "Mismatched columns on line %d.", symbol + 1);
				
				switch (columns[column])
				{
					case COLUMN_NAME:
					case COLUMN_COMMENT:
					{
						const char* begin;
						const char* end;
						if (*ptr == '"')
						{
							begin = ptr + 1;
							end = strstr(begin, "\"");
						}
						else
						{
							begin = ptr;
							end = strstr(begin, ",");
						}
						
						if (!end)
							end = strstr(ptr, "\n");
						CHECK(end, "Unexpected end of input table file on line %d.", symbol + 1);
						
						char* string = checked_malloc(end - begin + 1);
						memcpy(string, begin, end - begin);
						string[end - begin] = '\0';
						
						if (columns[column] == COLUMN_NAME)
						{
							table.symbols[symbol].name = string;
							
							/* Some of these symbols are only used by the
							   injector so we need to make sure they are
							   retained in the output. */
							if (strncmp(string, "_racdoor_", 9) == 0)
								table.symbols[symbol].used = TRUE;
						}
						else
							table.symbols[symbol].comment = string;
						
						ptr = end;
						if (*ptr == '"')
							ptr++;
						
						break;
					}
					case COLUMN_TYPE:
						if (strncmp(ptr, "NOTYPE", 6) == 0)
							table.symbols[symbol].type = STT_NOTYPE;
						else if (strncmp(ptr, "OBJECT", 6) == 0)
							table.symbols[symbol].type = STT_OBJECT;
						else if (strncmp(ptr, "FUNC", 4) == 0)
							table.symbols[symbol].type = STT_FUNC;
						else if (strncmp(ptr, "SECTION", 7) == 0)
							table.symbols[symbol].type = STT_SECTION;
						else if (strncmp(ptr, "FILE", 4) == 0)
							table.symbols[symbol].type = STT_FILE;
						else if (strncmp(ptr, "COMMON", 6) == 0)
							table.symbols[symbol].type = STT_COMMON;
						else if (strncmp(ptr, "TLS", 3) == 0)
							table.symbols[symbol].type = STT_TLS;
						else
							ERROR("Invalid TYPE on line %d.", symbol + 1);
						
						while (*ptr >= 'A' && *ptr <= 'Z')
							ptr++;
						
						break;
					default:
					{
						if (*ptr == ',')
							break;
						
						char* end = NULL;
						u32 value = (u32) strtoul(ptr, &end, 16);
						CHECK(end != ptr, "Invalid %s on line %d.",
							columns[column] == COLUMN_SIZE ? "SIZE" : "address",
							symbol + 1);
						
						if (columns[column] == COLUMN_SIZE)
							table.symbols[symbol].size = value;
						else if (columns[column] == COLUMN_CORE)
							table.symbols[symbol].core_address = value;
						else if (columns[column] == COLUMN_SPCORE)
							table.symbols[symbol].spcore_address = value;
						else if (columns[column] == COLUMN_MPCORE)
							table.symbols[symbol].mpcore_address = value;
						else if (columns[column] == COLUMN_FRONTEND)
							table.symbols[symbol].frontend_address = value;
						else if (columns[column] == COLUMN_FRONTBIN)
							table.symbols[symbol].frontbin_address = value;
						else
						{
							u32 overlay = columns[column] - COLUMN_FIRST_OVERLAY;
							CHECK(overlay < table.overlay_count, "Invalid column on line %d.", symbol + 1);
							
							table.symbols[symbol].overlay_addresses[overlay] = value;
							table.symbols[symbol].overlay = TRUE;
							
							CHECK(table.symbols[symbol].core_address == 0,
								"Symbol '%s' has multiple addresses in the CSV file.",
								table.symbols[symbol].name);
						}
						
						ptr = end;
					}
				}
				
				CHECK(*ptr == ',' || *ptr == '\n', "Invalid delimiters on line %d.", symbol + 1);
				if (*ptr == ',')
					ptr++;
				
				column++;
			}
			
			symbol++;
		}
		else
			ptr++;
	}
	
	return table;
}

u32 parse_table_header(const char** p, SymbolTable* table, Column* columns, b8 expect_newline)
{
	const char* ptr = *p;
	u32 column = 0;
	
	while (*ptr != '\0' && *ptr != '\n')
	{
		CHECK(column < MAX_COLUMNS, "Too many columns.");
		
		b8 quoted = FALSE;
		if (*ptr == '"')
		{
			quoted = TRUE;
			ptr++;
		}
		
		b8 done = FALSE;
		for (u32 i = 0; i < ARRAY_SIZE(column_names); i++)
		{
			if (strncmp(ptr, column_names[i].string, strlen(column_names[i].string)) == 0)
			{
				columns[column] = column_names[i].column;
				ptr += strlen(column_names[i].string);
				done = TRUE;
				break;
			}
		}
		
		if (!done)
		{
			columns[column] = COLUMN_FIRST_OVERLAY + table->overlay_count;
			
			char* end = NULL;
			u32 first, last;
			char opening = *ptr;
			
			switch (opening)
			{
				case '[': /* Closed interval. */
					ptr++;
					
					first = strtoul(ptr, &end, 10);
					CHECK(end != ptr && first < MAX_LEVELS, "Invalid interval column heading (bad level number).");
					ptr = end;
					
					CHECK(*ptr++ == ',', "Invalid interval column heading (missing comma).");
					
					last = strtoul(ptr, &end, 10);
					CHECK(end != ptr && last < MAX_LEVELS && first < last, "Invalid interval column heading (bad level number).");
					ptr = end;
					
					CHECK(*ptr++ == ']', "Invalid interval column heading (missing closing bracket).");
					
					for (u32 i = first; i <= last; i++)
						table->levels[i] = (u8) table->overlay_count;
					
					table->level_count = MAX(table->level_count, last);
					
					break;
				case '{': /* Set. */
					do
					{
						ptr++;
						
						u32 level = strtoul(ptr, &end, 10);
						CHECK(end != ptr && level < MAX_LEVELS, "Invalid set column heading (bad level number).");
						ptr = end;
						
						table->levels[level] = (u8) table->overlay_count;
						table->level_count = MAX(table->level_count, level);
					} while (*ptr == ',');
					
					CHECK(*ptr++ == '}', "Invalid set column heading (missing closing bracket).");
					
					break;
				default: /* Scalar. */
					first = strtoul(ptr, &end, 10);
					CHECK(end != ptr && first < MAX_LEVELS, "Invalid scalar column heading (bad level number).");
					ptr = end;
					
					table->levels[first] = (u8) table->overlay_count;
					table->level_count = MAX(table->level_count, first + 1);
			}
			
			table->overlay_count++;
		}
		
		if (quoted)
			CHECK(*ptr++ == '"', "Unexpected characters in table header.");
		
		if (*ptr == ',')
			ptr++;
		
		column++;
	}
	
	if (expect_newline)
		CHECK(*ptr++ == '\n', "Unexpected end of input table file.");
	
	*p = ptr;
	
	return column;
}

#endif
