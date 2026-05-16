/*
 * file_manager.h
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */

#ifndef FILE_MANAGER_H_
#define FILE_MANAGER_H_

#include <global.h>

#define 	MAX_FILE_NAME	256

typedef enum
{
	DEL_TYPE_MODEL_FILE,
	DEL_TYPE_PATTERN_FILE,
	DEL_TYPE_BITMAP_DIR,
	DEL_TYPE_BITMAP_FILE,
	DEL_TYPE_GROUP_FILE
} enum_del_type_t;

char 		**pp_file, **pp_sub;
int 		file_count, file_index;
int 		sub_count, sub_index;

extern int 	get_file_list(enum_del_type_t type);
extern char *get_cur_file_name(void);
extern void file_list_inc(void);
extern void file_list_dec(void);
extern void free_file_list(void);

extern int 	get_sub_bmp_list(char *path);
extern char *get_sub_bmp_name(void);
extern void sub_bmp_list_inc(void);
extern void sub_bmp_list_dec(void);
extern void free_sub_bmp_list(void);

extern int 	remove_dirs(char *path);

#endif /* FILE_MANAGER_H_ */
