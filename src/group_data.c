/*
 * group_data.c
 *
 *  Created on: Sep 21, 2017
 *      Author: root
 */

#include <group_data.h>
#include <model_data.h>
#include <pattern_control.h>

#ifdef GROUP_SELECT
int group_list_load(void)
{
	DIR				*dir;
	struct dirent	*dir_entry;
	char			path[MAX_PATH], ext[MAX_PATH];
	int				cnt = 0;

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/", DIR_ROOT, DIR_GROUP);

	if( (dir=opendir(path)) == NULL )
	{
		return 0;
	}

	memset(group_list, 0, MAX_GROUP_CNT*MAX_GROUP_NAME);

	while( (dir_entry = readdir(dir)) )
	{
		if( !strcmp(dir_entry->d_name, ".") ) continue;
		if( !strcmp(dir_entry->d_name, "..") ) continue;
		if( !strchr(dir_entry->d_name, '.') ) continue;

		memset(ext, 0, sizeof(ext));
		strcpy(ext, strrchr(dir_entry->d_name, '.'));
		if( !strcmp(ext, EXT_GROUP) )
		{
//			strncpy(group_list[cnt], dir_entry->d_name, strlen(dir_entry->d_name));
			strcpy(group_list[cnt], strtok(dir_entry->d_name, "."));
			printf("group[%d] : %s\n", cnt, group_list[cnt]);

			if(cnt++ > MAX_GROUP_CNT) break;
		}
	}
	printf("\n");

	closedir(dir);

	return cnt;
}

int group_init(void)
{
	FILE 	*fp;
	int		i;
	int		ret = ACK;
	char	path[MAX_PATH];

	group_cnt = group_list_load();
//	printf("group count: %d\n", group_cnt);
	if(group_cnt==0)
	{
		fprintf(stderr, "group_list_load() error!\n");
		rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");
//		group_default();
		return ret=NACK;
	}

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, CURRENT_GROUP_NAME);
	if( 0 == access(path, F_OK))
	{
		fp = fopen(path, "r");
		fgets(group_name, MAX_GROUP_NAME-1, fp);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", CURRENT_GROUP_NAME);
		strcpy(group_name, group_list[0]);
		fp = fopen(path, "w");
		fputs(group_name, fp);
		fclose(fp);
		sync();
	}

	// search current group
	for(i=0; i<group_cnt; i++)
	{
		if(!strcmp(group_name, group_list[i]))
		{
			group_idx = i;
			printf("group index: %d\n", group_idx);
			break;
		}
	}

	return ret;
}




int group_select(char* name)
{
	FILE 	*fp;
	int		i;
	int		ret=ACK;
	char	path[MAX_PATH];

	gp.model_change=1;

	inspect_hdd_usage();

	if(model_data.use_dvi)
	{
		set_opmode(READY);
		set_group_init_status(ACK);
		delay_us(1000000);
		rcb_ready_screen(ACK);
		gp.model_change=0;
		return ACK;
	}


	rcb_write(rcb_fd, RCB_LINE1, model_name);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_GROUP, name, EXT_GROUP);
	fp = fopen(path, "r");
	if(fp)
	{
		memset(&group_data, 0, sizeof(group_data_t));
		fread(&group_data.hdr, sizeof(group_head_t), 1, fp);
		pattern_cnt = group_data.hdr.count;

		for(i=0; i<pattern_cnt; i++)
		{
			fread(&group_data.pat[i], sizeof(pattern_data_t), 1, fp);
			memset(&pattern_list[i], 0, MAX_PAT_NAME);
			memcpy(&pattern_list[i], group_data.pat[i].name, MAX_PAT_NAME);
			printf("pattern[%d] : %s\n", i, pattern_list[i]);
		}
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "[%s] group select failed!\n", name);
		group_selection_end=NACK;
		return NACK;
	}

	// reset curgroup.txt
	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, CURRENT_GROUP_NAME);
	fp = fopen(path, "w");
	if(fp)
	{
		printf("current group : %s\n", name);
		memset(group_name, 0, MAX_GROUP_NAME);
		sprintf(group_name, "%s", name);
		fputs(group_name, fp);
		fclose(fp);
		sync();
	}
	else
	{
		fprintf(stderr, "[%s] %s failed!\n", name, CURRENT_GROUP_NAME);
		group_selection_end=NACK;
		return NACK;
	}

	player_stop();

	ret = memory_preload();
	if(ACK != ret)
	{
		rcb_ready_screen(ret);
		usleep(2000000);
		return ret;
	}

	set_opmode(READY);
	set_group_init_status(ret);
	delay_us(1000000);
	rcb_ready_screen(ACK);
	gp.model_change=0;

//	set_pattern_index(0);
//	pattern_change(get_pattern_index());

	return ACK;
}

void group_update(void)
{
	group_cnt = group_list_load();
}

void set_group_init_status(int status)
{
	group_init_status = status;
}

int get_group_init_status(void)
{
	return group_init_status;
}

void set_curgroup_idx(char *name)
{
	int i;
	for(i=0; i<MAX_GROUP_CNT; i++)
	{
		if(0==strcmp(group_list[i], name))
		{
			group_idx = i;
			break;
		}
	}
}
#endif

