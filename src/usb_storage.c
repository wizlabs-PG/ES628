/*
 * usb_storage.c
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */


#include <usb_storage.h>
#include <rcb.h>
#include <model_data.h>

static char *usb_title[] 	= { "USB:MODEL", "USB:GROUP","USB:PATTERN", "USB:BITMAP", "USB:IMAGE-PNG", "USB:SCHEDULE", "USB:F/W", "USB:FPGA", "USB:ETC", "USB:REMOVE", NULL };
static char *file_ext[]  	= { EXT_MODEL, EXT_GROUP, EXT_PATTERN, EXT_BMP, EXT_PNG, EXT_SCHEDULE, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext2[]  	= { EXT_MODEL2, EXT_GROUP2, EXT_PATTERN2, EXT_BMP2, EXT_PNG2, EXT_SCHEDULE2, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext3[]  	= { EXT_MODEL3, EXT_GROUP3, EXT_PATTERN3, EXT_BMP3, EXT_PNG3, EXT_SCHEDULE3, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext4[]  	= { EXT_MODEL4, EXT_GROUP4, EXT_PATTERN4, EXT_BMP4, EXT_PNG4, EXT_SCHEDULE4, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext5[]  	= { EXT_MODEL5, EXT_GROUP5, EXT_PATTERN5, EXT_BMP5, EXT_PNG5, EXT_SCHEDULE5, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext6[]  	= { EXT_MODEL6, EXT_GROUP6, EXT_PATTERN6, EXT_BMP6, EXT_PNG6, EXT_SCHEDULE6, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext7[]  	= { EXT_MODEL7, EXT_GROUP7, EXT_PATTERN7, EXT_BMP7, EXT_PNG7, EXT_SCHEDULE7, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *file_ext8[]  	= { EXT_MODEL8, EXT_GROUP8, EXT_PATTERN8, EXT_BMP8, EXT_PNG8, EXT_SCHEDULE8, EXT_FW, EXT_FPGA, NULL, NULL, NULL };
static char *dir_name[]		= { DIR_MODEL, DIR_GROUP, DIR_PATTERN, DIR_BMP, DIR_PNG, DIR_SCHEDULE, DIR_FW, DIR_FPGA, DIR_ETC, NULL, NULL };

void usb_default(void)
{
	usb_title_idx 	= COPY_TYPE_MODEL;
	usb_file_cnt	= 0;

	rcb_write(rcb_fd, RCB_LINE1, usb_title[usb_title_idx]);
	rcb_write(rcb_fd, RCB_LINE2, " ");
	set_opmode(USB_DETECTED);
}

char *replace_all(char *s, const char *olds, const char *news)
{
	char *result, *sr;
	size_t i, count = 0;
	size_t oldlen = strlen(olds); if(oldlen<1) return s;
	size_t newlen = strlen(news);

	if(newlen != oldlen)
	{
		for(i=0; s[i]!='\0';)
		{
			if(memcmp(&s[i], olds, oldlen)==0)
			{
				count++;
				i += oldlen;
			}
			else i++;
		}
	}
	else i = strlen(s);

	result = (char*)malloc(i+1+count*(newlen-oldlen));
	if(result==NULL) return NULL;

	sr = result;
	while(*s)
	{
		if(memcmp(s, olds, oldlen)==0)
		{
			memcpy(sr, news, newlen);
			sr += newlen;
			s  += oldlen;
		}
		else *sr++ = *s++;
	}
	*sr = '\0';

	return result;
}

int usb_detect_monitoring(void)
{
	FILE 	*fp = NULL;
	char	buf[MAX_PATH];
	char	*result, *src;
	int		ret = NACK;
	int		i, count;

	struct dirent **filelist;

	if(get_opmode() == USB_DETECTED)
		return NACK;
/*
	if( 0 != access(DIR_USB_STORAGE, F_OK) )
	{
		return NACK;
	}
*/
	if( (count = scandir(DIR_USB_MEDIA, &filelist, NULL, alphasort))== -1)				// 20-05-07	for Jetson Nano (Modified by Gi seung-eun)
	{
		fprintf(stderr, "%s open failed! %s\n", DIR_USB_MEDIA, strerror(errno));
		return NACK;
	}


	for(i=0;i<count;i++) {
		free(filelist[i]);
	}

	free(filelist);

	if(count<3)	return ACK;		// exclude ".", ".."



	fp = fopen(MOUNTS_FILE_NAME, "r");
	if( NULL == fp )
	{
		return NACK;
	}

	while(1)
	{
		memset(buf, 0, MAX_PATH);
		if(NULL == fgets(buf, MAX_PATH, fp)) break;

		// /dev/sda1 /media/root/BIGCHOI vfat .....
		result = strtok(buf, " ");		// get device name (ex) /dev/sda1
		if( (result!=NULL) && (strcmp(result, USB_DEV_NAME)==0) )
		{
			result = strtok(NULL, " ");	// get mount path (ex) /root/media/BIGCHOI
			if(result!=NULL)
			{
				src = replace_all(result, "\\040", " ");

				memset(usb_dir, 0, MAX_PATH);
				strcpy(usb_dir, src);
				printf("usb directory : %s \n", usb_dir);
				ret = ACK;
				free(src);
				break;
			}
		}
	}
	fclose(fp);

	if(ret == ACK)
	{
		usb_default();
	}

	return ret;
}

void usb_umount(void)
{
	char	str[MAX_BUF];

	rcb_write(rcb_fd, RCB_LINE2, "USB REMOVING...");

	memset(str, 0, MAX_BUF);
	sprintf(str, "umount %s", USB_DEV_NAME);
	if(0==system(str)){}

	rcb_write(rcb_fd, RCB_LINE2, "USB REMOVED!");
	delay_us(1000000);

	rcb_model_change();
}

void usb_title_change(int flag)
{
	if(flag)	// decrease
	{
		usb_title_idx--;

		if( 0 > usb_title_idx )
		{
			while(1)
			{
				if( NULL == usb_title[++usb_title_idx] )
				{
					usb_title_idx--;
					break;
				}
			}
		}
	}
	else		// increase
	{
		usb_title_idx++;
		if( NULL == usb_title[usb_title_idx] )
			usb_title_idx = 0;
	}

	rcb_write(rcb_fd, RCB_LINE1, usb_title[usb_title_idx]);
}

static int copy_file(char *src, char *dest)
{
	FILE	*fp_src, *fp_dest;
	char 	buf[MAX_COPY_BUF];
	size_t	readn;

	fp_src = fopen(src, "r");
	if( NULL == fp_src )
	{
		fprintf(stderr, "[USB COPY] %s open failed! %s\n", src, strerror(errno));
		return NACK;
	}

	fp_dest = fopen(dest, "w");
	if( NULL == fp_dest )
	{
		fprintf(stderr, "[USB COPY] %s open failed! %s\n", dest, strerror(errno));
		return NACK;
	}

	printf("src:%s, dest:%s\n", src, dest);

	while ( 0 < (readn = fread(buf, 1, MAX_COPY_BUF, fp_src)) )
	{
		fwrite(buf, 1, readn, fp_dest);
	}

	fclose(fp_src);
	fclose(fp_dest);
	sync();

	return ACK;
}

int copy_dirs(const char *path, int is_err_stop)
{
	DIR				*dir_ptr	= NULL;
	struct dirent	*file		= NULL;
	struct stat		prop;
	char			src_path[MAX_PATH];
	char 			dest_path[MAX_PATH];

	if( (dir_ptr = opendir(path)) == NULL )
	{
		rcb_write(rcb_fd, RCB_LINE2, "NO SOURCE!");
		fprintf(stderr, "[USB COPY] %s source directory does not exist!\n", path);
		return NACK;
	}

	while( (file = readdir(dir_ptr)) != NULL )
	{
		if( !strcmp(file->d_name, ".") ) 	continue;
		if( !strcmp(file->d_name, "..") ) 	continue;

		sprintf(src_path, "%s/%s", path, file->d_name);

		if(lstat(src_path, &prop) == -1)	continue;

		if(S_ISDIR(prop.st_mode))	// directory
		{
			memset(dest_path, 0, MAX_PATH);
			sprintf(dest_path, "%s%s/%s", DIR_ROOT, dir_name[usb_title_idx], file->d_name);

			if( 0 != access(dest_path, F_OK) )	// if directory is not exist, create a directory.
			{
				printf("mkdir : %s\n", dest_path);
				if( mkdir(dest_path, 0776)==-1 && errno != EEXIST )
				{
					rcb_write(rcb_fd, RCB_LINE2, "DIR CREATE ERR");
					fprintf(stderr, "directory create error : %s \n", strerror(errno));
					return NACK;
				}
			}

			if( (copy_dirs(src_path, is_err_stop) == NACK) && is_err_stop )
			{
				return NACK;
			}
		}
		else if( S_ISREG(prop.st_mode) || S_ISLNK(prop.st_mode) )	// file or link
		{
			rcb_write(rcb_fd, RCB_LINE1, "COPYING...");

			if(usb_title_idx == COPY_TYPE_FW)
			{
				memset(dest_path, 0, MAX_PATH);
				sprintf(dest_path, "%s%s/%s", DIR_ROOT, dir_name[usb_title_idx], file->d_name);

				rcb_write(rcb_fd, RCB_LINE2, file->d_name);

				if( (copy_file(src_path, dest_path)==NACK) && is_err_stop )
				{
					return NACK;
				}
			}
			else if(usb_title_idx == COPY_TYPE_ETC)
			{
				memset(dest_path, 0, MAX_PATH);
				if(!strcmp(file->d_name, KERNEL_IMAGE_NAME) || !strcmp(file->d_name, KERNEL_ZIMAGE_NAME))	// kernel image
					sprintf(dest_path, "%s/%s", DIR_BOOT, file->d_name);
				else if(!strcmp(file->d_name, FONT_FILE_NAME))
					sprintf(dest_path, "%s%s/%s", DIR_ROOT, DIR_FONT, file->d_name);

				rcb_write(rcb_fd, RCB_LINE2, file->d_name);

				if( (copy_file(src_path, dest_path)==NACK) && is_err_stop )
				{
					return NACK;
				}
			}
			else
			{
				if( (0==strcmp(strrchr(file->d_name, '.'), file_ext[usb_title_idx])) || (0==strcmp(strrchr(file->d_name, '.'), file_ext2[usb_title_idx])) ||
						(0==strcmp(strrchr(file->d_name, '.'), file_ext3[usb_title_idx])) || (0==strcmp(strrchr(file->d_name, '.'), file_ext4[usb_title_idx])) ||
						(0==strcmp(strrchr(file->d_name, '.'), file_ext5[usb_title_idx])) || (0==strcmp(strrchr(file->d_name, '.'), file_ext6[usb_title_idx])) ||
						(0==strcmp(strrchr(file->d_name, '.'), file_ext7[usb_title_idx])) || (0==strcmp(strrchr(file->d_name, '.'), file_ext8[usb_title_idx])) )
				{
					memset(dest_path, 0, MAX_PATH);
					if( (usb_title_idx == COPY_TYPE_BITMAP) || (usb_title_idx == COPY_TYPE_IMAGE_PNG) )
						sprintf(dest_path, "%s%s%s/%s", DIR_ROOT, dir_name[usb_title_idx], strrchr(path, '/'), file->d_name);
					else
						sprintf(dest_path, "%s%s/%s", DIR_ROOT, dir_name[usb_title_idx], file->d_name);

					rcb_write(rcb_fd, RCB_LINE2, file->d_name);

					if( (copy_file(src_path, dest_path)==NACK) && is_err_stop )
					{
						return NACK;
					}

				}
			}
		}
	}

	closedir(dir_ptr);

	return ACK;
}

int usb_operation(void)
{
	if(usb_title_idx == USB_REMOVE)
	{
		usb_umount();
		return ACK;
	}

	if(strlen(usb_dir)==0)
	{
		rcb_write(rcb_fd, RCB_LINE2, "NO TARGET!");
		fprintf(stderr, "[USB COPY] usb directory cannot find!\n");
		return NACK;
	}

	char 	src_path[MAX_PATH];
	int		ret;

	memset(src_path, 0, MAX_PATH);
	sprintf(src_path, "%s%s", usb_dir, dir_name[usb_title_idx]);

	ret = copy_dirs(src_path, 1);

	rcb_write(rcb_fd, RCB_LINE1, usb_title[usb_title_idx]);
	if(ret==ACK)
	{
		rcb_write(rcb_fd, RCB_LINE2, "COPY COMPLETED!");
		if(usb_title_idx == COPY_TYPE_FW || usb_title_idx == COPY_TYPE_FPGA || usb_title_idx == COPY_TYPE_ETC)
		{
			rcb_write(rcb_fd, RCB_LINE1, "SYSTEM UPDATED");
			rcb_write(rcb_fd, RCB_LINE2, "REBOOT........5");
			sleep(1);
			rcb_write(rcb_fd, RCB_LINE2, "REBOOT........4");
			sleep(1);
			rcb_write(rcb_fd, RCB_LINE2, "REBOOT........3");
			sleep(1);
			rcb_write(rcb_fd, RCB_LINE2, "REBOOT........2");
			sleep(1);
			rcb_write(rcb_fd, RCB_LINE2, "REBOOT........1");
			set_reboot(1);
			rcb_write(rcb_fd, RCB_LINE2, "REBOOTING......");
		}
		else if(usb_title_idx == COPY_TYPE_MODEL)
		{
			model_init();
		}
	}
	else
	{
		rcb_write(rcb_fd, RCB_LINE2, "COPY ERROR!");
	}

	return ret;
}







