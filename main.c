#include "todo.h"

#include <errno.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_DB_PATH "/home/rog/Text/2do.sqlite3"

int main(int argc, char * argv[])
{
	sqlite3 * db;
	char *    db_path;
	bool create_table = false;

	char * in_line = "\0"; // starts as single null char string for entry to an if statement later on
	char   menu_selection;
	_Bool  is_in_string      = 0;
	_Bool  stringed_commands = 0;
	int    str_idx           = 0;
	int    prev_del_idx      = -1;	// for warning user about duplicate deletion commands (that will do nothing)
									// due to an un-reindexed database.

	sqlite3_stmt * list_stmt = NULL;
	char           sql_cmd[SQL_INS_LEN];
	int            err;
	char *         sql_err_msg = 0;

	struct pk_id_map order_map[MAX_ROWS];

	if (argc < 2)
	{
		COLOR_ON(COLOR_F_RED, COLOR_NONE);
		printf("Database path not provided, creating new database.\n");
		COLOR_OFF();
		db_path = readline("Name for the database file:");
		if (db_path == 0 || db_path == NULL) 
		{
			COLOR_ON(COLOR_F_RED, COLOR_NONE);
			printf("Could not read database file name");
			COLOR_OFF();
			exit(EXIT_FAILURE);
		}
		create_table = true;
	} else
	{
		db_path = argv[1];
	}

	if (sqlite3_open(db_path, &db) != 0)
	{
		COLOR_ON(COLOR_F_RED, COLOR_NONE);
		printf("Cannot open database: %s\n", sqlite3_errmsg(db));
		COLOR_OFF();
		exit(EXIT_FAILURE);
	}

	if (create_table)
	{
		sprintf(sql_cmd, "CREATE TABLE " MAIN_TABLE_NAME "(ID INT PRIMARY KEY, %s TEXT, %s TEXT, %s INTEGER, %s TEXT, %s INTEGER, %s TEXT);", COL_NAMES[1], COL_NAMES[2], COL_NAMES[3], COL_NAMES[4], COL_NAMES[5], COL_NAMES[6]);
		if (sqlite3_exec(db, sql_cmd, NULL, NULL, &sql_err_msg) != SQLITE_OK)
		{
			sql_err_handler(sql_err_msg, FATAL);
			printf("Couldn't create table in new database file.\n");
			exit(EXIT_FAILURE);
		}

	}

	// index the database contents before beginning main loop
	just_index(db, order_map);

	while (true)
	{
		COLOR_ON(COLOR_BRIGHT, COLOR_NONE);
		// process every character in readline()'s output
		if (in_line[str_idx] == '\0') // hit end of user input
		{
		readin:
			if (is_in_string) free(in_line); // free before reading anew

			str_idx = is_in_string = stringed_commands = 0;

			in_line = readline("2do>");
			if (in_line == 0 || in_line == NULL)
			{
				// C-d (EOF)
				//fprintf(stderr, "\n");
				printf("All changes saved. Cheers.\n");
				exit(EXIT_FAILURE);
			}

			if (in_line[0] == '\0')
			{
				printf("use \'h\' for help\n");
				is_in_string = 1;
				goto readin;
			}

			add_history(in_line);
			menu_selection = in_line[0];
			is_in_string   = 1;
		}
		else
		{
			menu_selection = in_line[str_idx];
		}

		str_idx++;

		COLOR_OFF_N();

		switch (menu_selection)
		{
		case 'c':;
			stringed_commands = 1;
			break;
		case 'l':;
			// sqlite3_prepare_v3(db, sql_cmd, strlen(sql_cmd), 0, &list_stmt, NULL);
			err = print_todos(order_map, db);
			if (err != 0)
			{
				COLOR_ON(COLOR_F_RED, COLOR_NONE);
				sql_err_handler(sql_err_msg, NON_FATAL); // most likely DB is empty
				printf("Database is empty.");
				COLOR_OFF();
			}

			prev_del_idx = -1; // reset
			break;
		case 'd':;
			int    printed_id;
			char   id[2]   = "\0\0";
			char * uin_buf = id;
			if (stringed_commands)
			{
				int collected_nums = 0;
				// store the current index since we might
				// be incrementing it in the following block
				int str_idx_cpy = str_idx;
				while (collected_nums < 2)
				{
					if (!isdigit(in_line[str_idx_cpy + collected_nums]))
					{
						break;
					}
					id[collected_nums] = in_line[str_idx_cpy + collected_nums];
					collected_nums++;
					// IMPORTANT, KLUDGE.
					// must advance the string index var since we
					// don't want to go through the main user input switch statment
					// with the numbers just entered.
					str_idx++;
				}

				// sif (in_line[str_idx] == '\0') is_in_string = 0;	// went through the whole string
			}

			if (id[0] == '\0')
			{
				uin_buf    = readline("id:");
				printed_id = atoi(uin_buf);
			}
			else { printed_id = atoi(id); }

			int pk = -1;
			for (int i = 0; i < MAX_ROWS; i++)
			{
				if (order_map[i].print_id == printed_id)
					pk = order_map[i].pk;
			}
			if (pk == -1)
			{
				COLOR_ON(COLOR_F_RED, COLOR_NONE);
				fprintf(stderr, "Couldn't find entry with id %s \n", uin_buf);
				COLOR_OFF();
				break;
			}
			if (prev_del_idx == printed_id)
			{
				COLOR_ON(COLOR_F_YELLOW, COLOR_NONE);
				printf("The index of the items don't get refreshed until a list command is issued. Thus you don't need to account for the list shifting around while deleting items. Only one item was deleted.\n");
				COLOR_OFF();
			}
			prev_del_idx = printed_id;

			sprintf(sql_cmd, "DELETE FROM " MAIN_TABLE_NAME " WHERE id = %d", pk);
			err = sqlite3_exec(db, sql_cmd, NULL, NULL, &sql_err_msg);
			if (err != SQLITE_OK)
			{
				COLOR_ON(COLOR_F_RED, COLOR_NONE);
				sql_err_handler(sql_err_msg, NON_FATAL);
				COLOR_OFF();
			}

			break;
		case 'a':;
			struct row_entry * ins;
			ins = (struct row_entry *)malloc(sizeof(struct row_entry));
			if (ins == NULL)
			{
				break;
			}
			err = user_add_item(ins);

			if (ins == NULL || err != 0)
			{
				COLOR_ON(COLOR_F_RED, COLOR_NONE);
				fprintf(stderr, "Couldn't retrieve the entry inputted\n");
				COLOR_OFF();
				free(ins);
				break;
			}

			sql_insert(ins, db);
			free(ins);
			break;
		case 'h':
			printf("l: (l)ist items       a: (a)dd item\nd: (d)elete item      c: (c)hain one line commands\ne: (e)xit\n");
			break;

		case 'e':
			sqlite3_close(db);
			COLOR_OFF_N(); // ensure all color is returned to normal
			printf("All changes saved. Cheers.\n");
			exit(EXIT_SUCCESS);

		default:
			printf("use \'h\' for help\n");
			break;
		}
		if (!is_in_string)
			free(in_line);
	}
}
