#ifndef TODO_H
#define TODO_H

#include <stdbool.h>
#include <sqlite3.h>
#include <stdio.h>

enum URGENCIES
{
	URG_ENUM_BEG = 0, // flag for the first value in the enum, always add 1
	URG_NONE,
	URG_MIN,
	URG_MED,
	URG_MAX,
	URG_NUC,
	URG_ENUM_END, // flag for the last value in the enum
	              // could also do:
	              //URG_ENUM_FIRST = URG_NONE,
	              //URG_ENUM_LAST = URG_NUC,
};

enum colors
{
	COLOR_NONE = 0, // dummy -- does nothing
	COLOR_ALLOFF,   // all colors off
	// [F]oregrounds
	COLOR_F_FLASH,
	COLOR_F_BLACK,
	COLOR_F_RED,
	COLOR_F_YELLOW,
	COLOR_F_GREEN,
	COLOR_F_PURPLE,
	// [B]ackgrounds
	COLOR_B_RED,
	COLOR_B_YELLOW,
	COLOR_B_GREEN,
	COLOR_B_PURPLE,
	COLOR_B_BLACK,
	// special
	COLOR_BRIGHT,
	COLOR_UNLINE,
};

#define NO_REC 	// turn the recurring functionality off for now

#define NON_FATAL       0
#define FATAL           1
#define MAIN_TABLE_NAME "todo"
// TODO perhaps a way to define constants for each of the table column headers (i.e. URG)
#define NUM_COLS 7	// 6+1 for id column

extern const char * COL_NAMES[NUM_COLS];

enum col_index {
	COL_ENUM_BEG = -1,
	COL_ID,
	COL_TEXT,
	COL_TAG,
	COL_URG,
	COL_DOBY,
	COL_REC,
	COL_DATE,
	COL_ENUM_END,
};

extern const char * URGENCY_NAMES[];
extern const char * color_codes[];

#define MAX_TEXT_LEN 100
#define MAX_TAG_LEN  15
#define MAX_DATE_LEN 11
#define MAX_URG_LEN  2                // 1 for number, 1 for end \0
#define MAX_TIME_LEN	6 // xx:xx = 4 x's + colon + \0 = 6
#define MAX_DOBY_LEN MAX_DATE_LEN + MAX_TIME_LEN
#define SQL_INS_LEN  MAX_TEXT_LEN + MAX_TAG_LEN + MAX_DATE_LEN + MAX_DOBY_LEN + (MAX_URG_LEN * 2) + 100	// liberal 100 for the SQL syntax
#define MAX_ROWS	25

struct pk_id_map	// this approach gives less looping in exchange for a higher memory usage
					// not a bad tradeoff, I'd say.
{
	int pk;	// pk = primary key
	int print_id;
};

struct row_entry
{
	char text[MAX_TEXT_LEN];
	char tag[MAX_TAG_LEN];
	
	char doby_time[MAX_TIME_LEN];
	char doby_date[MAX_DATE_LEN];
	
	int rec; // currently only supporting repeat within 9 days
	char date[MAX_DATE_LEN];

	int  urgency;
};

#define COLOR_OFF()                    \
	printf("%s", color_codes[COLOR_ALLOFF]); \
	printf("\n");
#define COLOR_OFF_N() printf("%s", color_codes[COLOR_ALLOFF]);
#define COLOR_ON(x, y)            \
	do                            \
	{                             \
		printf("%s", color_codes[(x)]); \
		printf("%s", color_codes[(y)]); \
	} while (0)

int get_row_count(char *, sqlite3 *); 

int get_cur_width(void);

int user_add_item(struct row_entry *);

int map_urg_to_color(int);

void sql_err_handler(char *, _Bool);

int begin_index(sqlite3_stmt *, struct pk_id_map *);

int index_print(sqlite3_stmt *, struct pk_id_map *);

int print_callback(void *, int, char **, char **);

int print_todos(struct pk_id_map *, sqlite3 *);

static int index_tgroup_print(char *, struct pk_id_map *, int *, sqlite3 *, _Bool);

int just_index(sqlite3 *, struct pk_id_map *);

int sql_insert(struct row_entry *, sqlite3 *);

#endif
