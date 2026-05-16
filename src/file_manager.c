/*
 * file_manager.c
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */


#include <file_manager.h>
#include <group_data.h>

static int get_count(enum_del_type_t type, char *path)
{
	DIR				*dir_ptr	= NULL;
	struct dirent	*file		= NULL;
	struct stat		prop;
	char			file_path[MAX_PATH];
	int				count 	= 0;

	if( (dir_ptr = opendir(path)) == NULL )
	{
		fprintf(stderr, "%s directory does not exist!\n", path);
		return 0;
	}

	while( (file = readdir(dir_ptr)) != NULL )
	{
		if( !strcmp(file->d_name, ".") ) 	continue;
		if( !strcmp(file->d_name, "..") ) 	continue;

		sprintf(file_path, "%s/%s", path, file->d_name);

		if(lstat(file_path, &prop) == -1)	continue;

		if(type == DEL_TYPE_BITMAP_DIR)
		{
			if(S_ISDIR(prop.st_mode)) count++;
		}
		else
		{
			if(S_ISREG(prop.st_mode)) count++;
		}
	}

	closedir(dir_ptr);

	return count;
}

int get_file_list(enum_del_type_t type)
{
	DIR				*dir_ptr	= NULL;
	struct dirent	*file		= NULL;
	struct stat		prop;
	char			file_path[MAX_PATH];
	char			dir_path[MAX_PATH];
	int				i;

	free_file_list();

	memset(dir_path, 0, MAX_PATH);

	switch(type)
	{
	case DEL_TYPE_MODEL_FILE:
		sprintf(dir_path, "%s%s", DIR_ROOT, DIR_MODEL);
		break;
	case DEL_TYPE_PATTERN_FILE:
		sprintf(dir_path, "%s%s", DIR_ROOT, DIR_PATTERN);
		break;
	case DEL_TYPE_BITMAP_DIR:
	case DEL_TYPE_BITMAP_FILE:	// no use
		sprintf(dir_path, "%s%s", DIR_ROOT, DIR_BMP);
		break;
#ifdef GROUP_SELECT
	case DEL_TYPE_GROUP_FILE:
		sprintf(dir_path, "%s%s", DIR_ROOT, DIR_GROUP);
		break;
#endif
	}

	file_count = get_count(type, dir_path);
	if(file_count==0) 	return 0;

	pp_file = (char**)malloc(sizeof(char*)*file_count);
	if(pp_file == NULL)	return 0;

	for(i=0; i<file_count; i++)
	{
		pp_file[i] = (char*)malloc(sizeof(char)*MAX_FILE_NAME);
	}

	if( (dir_ptr = opendir(dir_path)) == NULL )
	{
		fprintf(stderr, "%s directory does not exist!\n", dir_path);
		return 0;
	}

	i = 0;
	while( (file = readdir(dir_ptr)) != NULL )
	{
		if(i>=file_count) break;

		if( !strcmp(file->d_name, ".") ) 	continue;
		if( !strcmp(file->d_name, "..") ) 	continue;

		sprintf(file_path, "%s/%s", dir_path, file->d_name);

		if(lstat(file_path, &prop) == -1)	continue;

		if(type == DEL_TYPE_BITMAP_DIR)
		{
			if(S_ISDIR(prop.st_mode))	// directory
			{
				memset(pp_file[i], 0, MAX_FILE_NAME);
				sprintf(pp_file[i], "%s", file->d_name);
				i++;
			}
		}
		else
		{
			if(S_ISREG(prop.st_mode))	// file
			{
				memset(pp_file[i], 0, MAX_FILE_NAME);
				sprintf(pp_file[i], "%s", file->d_name);
				i++;
			}
		}
	}

	closedir(dir_ptr);
	return 1;
}

int get_sub_bmp_list(char *path)
{
	if(path==NULL) return 0;

	DIR				*dir_ptr	= NULL;
	struct dirent	*file		= NULL;
	struct stat		prop;
	char			file_path[MAX_PATH];
	char			dir_path[MAX_PATH];
	int				i;

	free_sub_bmp_list();

	memset(dir_path, 0, MAX_PATH);
	sprintf(dir_path, "%s%s/%s", DIR_ROOT, DIR_BMP, path);

	sub_count = get_count(DEL_TYPE_BITMAP_FILE, dir_path);
	if(sub_count==0) 	return 0;

	pp_sub = (char**)malloc(sizeof(char*)*sub_count);
	if(pp_sub == NULL)	return 0;

	for(i=0; i<sub_count; i++)
	{
		pp_sub[i] = (char*)malloc(sizeof(char)*MAX_FILE_NAME);
	}

	if( (dir_ptr = opendir(dir_path)) == NULL )
	{
		fprintf(stderr, "%s directory does not exist!\n", dir_path);
		return 0;
	}

	i = 0;
	while( (file = readdir(dir_ptr)) != NULL )
	{
		if(i>=sub_count) break;

		if( !strcmp(file->d_name, ".") ) 	continue;
		if( !strcmp(file->d_name, "..") ) 	continue;

		sprintf(file_path, "%s/%s", dir_path, file->d_name);

		if(lstat(file_path, &prop) == -1)	continue;

		if(S_ISREG(prop.st_mode))	// file
		{
			memset(pp_sub[i], 0, MAX_FILE_NAME);
			sprintf(pp_sub[i], "%s", file->d_name);
			i++;
		}
	}

	closedir(dir_ptr);
	return 1;
}

void file_list_inc(void)
{
	if(file_index >= file_count-1) 	file_index = 0;
	else							file_index++;
}

void file_list_dec(void)
{
	if(file_index > 0)				file_index--;
	else							file_index = file_count - 1;
}

char *get_cur_file_name(void)
{
	if(file_count==0)
		return "NO FILE!";

	if(file_index>=0 && file_index<file_count)
		return pp_file[file_index];

	return NULL;
}

void sub_bmp_list_inc(void)
{
	if(sub_index >= sub_count-1) 	sub_index = 0;
	else							sub_index++;
}

void sub_bmp_list_dec(void)
{
	if(sub_index > 0)				sub_index--;
	else							sub_index = sub_count - 1;
}

char *get_sub_bmp_name(void)
{
	if(sub_count==0)
		return "NO FILE!";

	if(sub_index>=0 && sub_index<sub_count)
		return pp_sub[sub_index];

	return NULL;
}

int remove_dirs(char *path)
{
	DIR				*dir_ptr	= NULL;
	struct dirent	*file		= NULL;
	struct stat		prop;
	char			file_path[MAX_PATH];

	printf("path : %s\n", path);

	if( (dir_ptr = opendir(path)) == NULL )
	{
		fprintf(stderr, "%s directory does not exist!\n", path);
		return remove(path);
	}

	while( (file = readdir(dir_ptr)) != NULL )
	{
		if( !strcmp(file->d_name, ".") ) 	continue;
		if( !strcmp(file->d_name, "..") ) 	continue;

		sprintf(file_path, "%s/%s", path, file->d_name);

		if(lstat(file_path, &prop) == -1)	continue;

		if(S_ISDIR(prop.st_mode))
		{
			if(remove_dirs(file_path) == -1)
			{
				return -1;
			}
		}
		else if(S_ISREG(prop.st_mode) || S_ISLNK(prop.st_mode))
		{
			if(remove(file_path)==-1)
			{
				return -1;
			}
		}
	}

	closedir(dir_ptr);

	sync();

	return 1;
}

void free_file_list(void)
{
	int i;
	for(i=0; i<file_count; i++)
	{
		if(pp_file[i] != NULL)
		{
			free(pp_file[i]);
			pp_file[i] = NULL;
		}
	}

	if(pp_file != NULL) free(pp_file);

	pp_file 	= NULL;
	file_index 	= 0;
	file_count 	= 0;
}

void free_sub_bmp_list(void)
{
	int i;
	for(i=0; i<sub_count; i++)
	{
		if(pp_sub[i] != NULL)
		{
			free(pp_sub[i]);
			pp_sub[i] = NULL;
		}
	}

	if(pp_sub != NULL) free(pp_sub);

	pp_sub 		= NULL;
	sub_index 	= 0;
	sub_count 	= 0;
}
