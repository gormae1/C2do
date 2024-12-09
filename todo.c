#include "todo.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <readline/readline.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


const char * COL_NAMES[NUM_COLS] = {
    0,      // for SQL row id
    "text", // todo text
    "tag",  // type of entry
    "urg",  // urgency number (URG_ENUM_BEG+1 to URG_ENUM_END-1)
    "doby", // date to do by (date @ hh:mm)
    "rec",  // if recurring, days until next instance
    "date", // date todo item was created (needed for recurring, but is also useful)
};

const char * color_codes[] = {
    "\0",      // for COLOR_NONE
    "\033[0m", // all off
    // start foregrounds
    "\033[5m",
    "\033[30m",
    "\033[31m",
    "\033[33m",
    "\033[32m",
    "\033[35m",
    // start backgrounds
    "\033[41m",
    "\033[43m",
    "\033[42m",
    "\033[45m",
    "\033[40m",
    // start special
    "\033[1m",
    "\033[4m",
};
const char * URGENCY_NAMES[] = {
    [URG_NONE] = "NONE",
    [URG_MIN]  = "MIN",
    [URG_MED]  = "MED",
    [URG_MAX]  = "MAX",
    [URG_NUC]  = "NUC"};

int map_urg_to_color(int urg)
{
	int bg_color;

	switch (urg)
	{
	case URG_NONE:
		bg_color = COLOR_F_PURPLE;
		break;
	case URG_MIN:
		bg_color = COLOR_F_GREEN;
		break;
	case URG_MED:
		bg_color = COLOR_F_YELLOW;
		break;
	case URG_MAX:
		bg_color = COLOR_F_RED;
		break;
	case URG_NUC:
		COLOR_ON(COLOR_F_FLASH, COLOR_B_BLACK);
		return 0; // nuclear urgency requires a different situation than the rest
	default:
		COLOR_OFF();
		COLOR_ON(COLOR_F_RED, COLOR_NONE);
		fprintf(stderr, "Internal error, unknown URGENCY value:%d\n", urg);
		COLOR_OFF();

		return 1;
	}
	COLOR_OFF_N();
	COLOR_ON(bg_color, COLOR_NONE);
	return 0;
}

// date should be yy-mm-dd
// TODO: get rid of this method and implement more solid approach
static int parse_date(char * date)
{
	time_t      raw;
	struct tm * info;
	char        buf[5]; // for parsing each yy\0,mm\0,dd\0

	time(&raw);
	info = localtime(&raw);
	printf("DATE = %s\n", date);
	// year
	buf[0]        = date[0];
	buf[1]        = date[1];
	buf[2]        = date[2];
	buf[3]        = date[3];
	buf[4]        = 0;
	info->tm_year = atoi(buf); // should atoi() be used? IDK

	// month
	buf[0]       = date[0 + 5];
	buf[1]       = date[1 + 5];
	buf[2]       = 0;
	info->tm_mon = atoi(buf);
	printf("atoi month: %s\n", (buf));

	// day
	buf[0] = date[0 + 8];
	buf[1] = date[1 + 8];
	buf[2] = 0;
	printf("atoi day: %d\n", atoi(buf));
	info->tm_mon = atoi(buf);

	return 0;
}

int user_add_item(struct row_entry * to_fill)
{
	// char        buf[MAX_TEXT_LEN];
	char *      uin_buf;
	time_t      t  = time(NULL);
	struct tm * tm = localtime(&t);

	// printf("text:");
	// if (nnlfget(buf, MAX_TEXT_LEN, stdin) == NULL)
	if ((uin_buf = readline("text:")) == NULL)
	{
		fprintf(stderr, "something went wrong with adding that string");
		return -EIO; /* treat as fatal & let calling function free() memory */
	}
	snprintf(to_fill->text, MAX_TEXT_LEN, "%s", uin_buf);

	// printf("tag:");
	// if (nnlfget(buf, MAX_TEXT_LEN, stdin) == NULL || *buf == '\n')
	if ((uin_buf = readline("tag:")) != NULL && uin_buf[0] != 0)
	{
		snprintf(to_fill->tag, MAX_TAG_LEN, "%s", uin_buf);
	} else
	{
		sprintf(to_fill->tag, "*other*"); 
	}

	to_fill->urgency = URG_NONE; // default urgency: URG_NONE
	if ((uin_buf = readline("urgency (none, min, med, max, nuclear):")) != NULL && uin_buf[0] != '\n')
	{
		// to lower for user input consistency
		for (int idx = 0; uin_buf[idx]; idx++)
			uin_buf[idx] = toupper(uin_buf[idx]);

		for (int i = 1; i < URG_ENUM_END; i++)
			if (strcmp(uin_buf, URGENCY_NAMES[i]) == 0) to_fill->urgency = i;
	}

#ifdef USING_DATE
	if ((uin_buf = readline("do by ((hh:mm) dd-mm-yy):")) != NULL && *uin_buf != '\n')
	{
		/*
		// ensures somewhat correct input
		for (int i = 0; i < strlen(uin_buf); i++)
		{
		    // TODO remove this with a call to time_t
		    if (!isdigit(uin_buf[i]) && uin_buf[i] != ':' && uin_buf[i] != '-' && uin_buf[i] != ' ')
		    {
		        COLOR_ON(COLOR_F_RED, COLOR_NONE);
		        fprintf(stderr, "Invalid do by date: %s\n", uin_buf);
		        COLOR_OFF();
		        snprintf(to_fill->doby_, MAX_DOBY_LEN, 0); // terminate at first char (ignore do by time)
		        break;
		    }
		}
		if ((uin_buf[0] - '0') > 2 || (uin_buf[0] - '0') == 2 && (uin_buf[1] - '0') > 4)
		{
		    COLOR_ON(COLOR_F_RED, COLOR_NONE);
		    fprintf(stderr, "Insert do by time in 24hr format.\n");
		    COLOR_OFF();
		    snprintf(to_fill->doby, MAX_DOBY_LEN, 0); // ignore do by time because of this
		}
		*/
		// parse_date(uin_buf+6);
		snprintf(to_fill->doby_date, MAX_DOBY_LEN, 0);
		snprintf(to_fill->doby_time, MAX_DOBY_LEN, 0);
	}
	else
	{
		snprintf(to_fill->doby_date, MAX_DOBY_LEN, 0);
		snprintf(to_fill->doby_time, MAX_DOBY_LEN, 0);
	}
#endif

#ifndef NO_REC
	if ((uin_buf = readline("recurring:")) != NULL)
	{
		to_fill->rec = atoi(uin_buf);
	}
	else
#endif
	{
		to_fill->rec = 0;
	}

	snprintf(to_fill->date, MAX_DATE_LEN, (tm->tm_mon < 10) ? "%d:0%d:%d" : "%d:%d:%d",
	         tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);

	return 0;
}

int sql_insert(struct row_entry * to_insert, sqlite3 * db)
{
	char * sql_err_msg = 0;
	char   sql_cmd[SQL_INS_LEN];
	char * append_offset = sql_cmd; // using this as the first argument to sprintf()
	                                // to create a sort of strcat() that
	                                // accepts formatted strings. Should be
	                                // completely safe
	// also note snprintf() isn't used here because we are
	// 100% certain that SQL_INS_LEN will be enough space

	append_offset += sprintf(sql_cmd, "INSERT INTO " MAIN_TABLE_NAME " (");
	for (int i = 1; i < NUM_COLS; i++)
	{
		append_offset += sprintf(append_offset,
		                         (i == NUM_COLS - 1 ? "%s" : "%s,"), // needed so ',' is put after last item
		                         COL_NAMES[i]);
	}

	append_offset += sprintf(append_offset,
	                         ") VALUES (\"%s\",\"%s\",%d,\"%s\",%d,\"%s\")",
	                         to_insert->text,
	                         to_insert->tag,
	                         to_insert->urgency,
	                         to_insert->doby_time,
	                         to_insert->rec,
	                         to_insert->date);
	// printf("SQL Command: %s\n", sql_cmd);
	if (sqlite3_exec(db, sql_cmd, NULL, NULL, &sql_err_msg) != SQLITE_OK)
	{
		sql_err_handler(sql_err_msg, NON_FATAL);
		return -1;
	}

	return 0;
}

void sql_err_handler(char * err_msg, _Bool fatality)
{
	printf("SQL error: %s\n", err_msg);
	sqlite3_free(err_msg);

	if (fatality == FATAL)
	{
		fprintf(stderr, "Execution will be halted.\n");
		// TODO fatal error cleanup function
	}
}

static int index_tgroup_print(char * tag_name, struct pk_id_map * order_map, int * map_idx_track, sqlite3 * db, _Bool last_tag)
{
	char           sql_cmd[SQL_INS_LEN];
	sqlite3_stmt * sublist_stmt = NULL;
	int            pk;

	sprintf(sql_cmd, "SELECT * FROM " MAIN_TABLE_NAME " WHERE %s = \"%s\" ORDER BY %s DESC", COL_NAMES[COL_TAG], tag_name[0] == 0 ? "" : tag_name, COL_NAMES[COL_URG]);
	sqlite3_prepare_v3(db, sql_cmd, strlen(sql_cmd), 0, &sublist_stmt, NULL);
	
	int step_err;
	while ((step_err = sqlite3_step(sublist_stmt)) == SQLITE_ROW)
	{
		pk                            = sqlite3_column_int(sublist_stmt, COL_ID);
		order_map[*map_idx_track - 1] = (struct pk_id_map){pk, *map_idx_track};
		if (*map_idx_track >= (MAX_ROWS - 1))
		{
			COLOR_ON(COLOR_NONE, COLOR_NONE);
			fprintf(stderr, "Somehow there are more rows than allowed (%d). Was the database modified externally?\n", MAX_ROWS);
			COLOR_OFF();
			return -1;
		}

		int    urg_number = sqlite3_column_int(sublist_stmt, COL_URG);
		char * tag_parse  = (char *)sqlite3_column_text(sublist_stmt, COL_TAG);

		printf(" └────>> ");
		map_urg_to_color(urg_number);
		COLOR_ON(COLOR_BRIGHT, COLOR_NONE); // make bold as well
		printf("(%d)", *map_idx_track);     // sqlite3_column_int(sublist_stmt, COL_ID));
		COLOR_OFF_N();

		printf(" %s", sqlite3_column_text(sublist_stmt, COL_TEXT));
		putchar('\n');

		(*map_idx_track)++;
	
	}

	if (!last_tag) // new line only if not absolute last in list of todo items
		printf("\n");    // seperate next tag group from this one

	return ((step_err != SQLITE_DONE) ? -1 : 0);
}

int print_todos(struct pk_id_map * order_map, sqlite3 * db)
{
	int            step_err;
	int            map_idx_track = 1;
	_Bool          first_pass    = 1;
	char           tag_tracker[MAX_TAG_LEN];
	sqlite3_stmt * list_stmt = NULL;
	char           sql_cmd[SQL_INS_LEN];
	int unique_tag_count = -1;
	int track_rows = 0;

	sprintf(sql_cmd, "SELECT COUNT(DISTINCT tag) FROM " MAIN_TABLE_NAME " ORDER BY %s DESC", COL_NAMES[COL_TAG]);
	unique_tag_count = get_row_count(sql_cmd, db);
	
	sprintf(sql_cmd, "SELECT * FROM " MAIN_TABLE_NAME " ORDER BY %s DESC", COL_NAMES[COL_TAG]);
	sqlite3_prepare_v3(db, sql_cmd, strlen(sql_cmd), 0, &list_stmt, NULL);


	while ((step_err = sqlite3_step(list_stmt)) == SQLITE_ROW)
	{
		int    urg_number = sqlite3_column_int(list_stmt, COL_URG);
		char * tag_parse  = (char *)sqlite3_column_text(list_stmt, COL_TAG);

		if (strcmp(tag_tracker, tag_parse) != 0 || first_pass)
		{
			track_rows++;
			if (first_pass) strncpy(tag_tracker, tag_parse, MAX_TAG_LEN);
			printf("─┬>[%s]\n", tag_parse[0] == 0 ? "*other*" : tag_parse);

			if (index_tgroup_print(tag_parse, order_map, &map_idx_track, db, track_rows == unique_tag_count) == -1)
			{
				COLOR_ON(COLOR_NONE, COLOR_NONE);
				fprintf(stderr, "Failed issuing sub-sort. Just what has happened?\n");
				COLOR_OFF();
				return -1;
			}
		}
		first_pass = 0;
		strncpy(tag_tracker, tag_parse, MAX_TAG_LEN); // update tracker to follow along (dragged 1 behind)
	}
	return ((step_err != SQLITE_DONE) ? 1 : 0);
}

int get_row_count(char * sql_cmd, sqlite3 * db)
{
	sqlite3_stmt * count_stmt = NULL;
	int step_err;

	step_err = sqlite3_prepare_v3(db, sql_cmd, strlen(sql_cmd), 0, &count_stmt, NULL);
	if (step_err != SQLITE_OK)
	{
		// TODO: general error for this case.
	}
	step_err = sqlite3_step(count_stmt);

	
	int row_count = sqlite3_column_int(count_stmt, 0);
	sqlite3_finalize(count_stmt);
	return row_count;
}

int just_index(sqlite3 * db, struct pk_id_map * order_map)
{
	int            step_err;
	int            map_idx_track = 1;
	_Bool          first_pass    = 1;
	char           tag_tracker[MAX_TAG_LEN];
	char           sql_cmd[SQL_INS_LEN];
	int            pk;
	sqlite3_stmt * sublist_stmt = NULL;
	sqlite3_stmt * list_stmt    = NULL;

	sprintf(sql_cmd, "SELECT * FROM " MAIN_TABLE_NAME " ORDER BY %s DESC", COL_NAMES[COL_TAG]);
	sqlite3_prepare_v3(db, sql_cmd, strlen(sql_cmd), 0, &list_stmt, NULL);

	while ((step_err = sqlite3_step(list_stmt)) == SQLITE_ROW)
	{
		int    urg_number = sqlite3_column_int(list_stmt, COL_URG);
		char * tag_parse  = (char *)sqlite3_column_text(list_stmt, COL_TAG);

		if (strcmp(tag_tracker, tag_parse) != 0 || first_pass)
		{
			if (first_pass) strncpy(tag_tracker, tag_parse, MAX_TAG_LEN);
			sprintf(sql_cmd, "SELECT * FROM " MAIN_TABLE_NAME " WHERE %s = \"%s\" ORDER BY %s DESC", COL_NAMES[COL_TAG], tag_parse[0] == 0 ? "" : tag_parse, COL_NAMES[COL_URG]);
			sqlite3_prepare_v3(db, sql_cmd, strlen(sql_cmd), 0, &sublist_stmt, NULL);

			while ((step_err = sqlite3_step(sublist_stmt)) == SQLITE_ROW)
			{
				pk                           = sqlite3_column_int(sublist_stmt, COL_ID);
				order_map[map_idx_track - 1] = (struct pk_id_map){pk, map_idx_track};
				if (map_idx_track >= (MAX_ROWS - 1))
				{
					COLOR_ON(COLOR_NONE, COLOR_NONE);
					fprintf(stderr, "Somehow there are more rows than allowed (%d). Was the database modified externally?\n", MAX_ROWS);
					COLOR_OFF();
					return -1;
				}
				map_idx_track++;
			}
		}
		first_pass = 0;
		strncpy(tag_tracker, tag_parse, MAX_TAG_LEN); // update tracker to follow along (dragged 1 behind)
	}

	return ((step_err != SQLITE_DONE) ? 1 : 0);
}
/*


        if (sqlite3_column_text(list_stmt, COL_DOBY)[0] != 0)
        {
            printf("   └─┬─> ");
            print_section = 0;
            printf("Do by: ");
            printf("%s", "doby placeholder");
            //strchr(
            //printf("%c%c:%c%c @ %c%c:%c%c", col_content[COL_DOBY]);
#ifndef NO_REC
            if (sqlite3_column_text(list_stmt, COL_REC) != 0)
                printf(" ∞ %s\n", sqlite3_column_text(list_stmt, COL_REC));
#endif
            printf("     └───> ");
        }
        */
