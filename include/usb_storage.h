/*
 * usb_storage.h
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */

#ifndef USB_STORAGE_H_
#define USB_STORAGE_H_

#include <global.h>

#define DIR_USB_STORAGE		"/proc/scsi/usb-storage"
#define	DIR_USB_MEDIA		"/media/root"
#define MOUNTS_FILE_NAME	"/proc/mounts"
#define USB_DEV_NAME		"/dev/sda1"
#define MAX_COPY_BUF		1024

typedef enum
{
	COPY_TYPE_MODEL,
	COPY_TYPE_GROUP,
	COPY_TYPE_PATTERN,
	COPY_TYPE_BITMAP,
	COPY_TYPE_IMAGE_PNG,
	COPY_TYPE_SCHEDULE,
	COPY_TYPE_FW,
	COPY_TYPE_FPGA,
	COPY_TYPE_ETC,
	USB_REMOVE
} enum_copy_type_t;

int			usb_title_idx;
int			usb_file_cnt;
char		usb_dir[MAX_PATH];

extern void usb_default(void);
extern int 	usb_detect_monitoring(void);
extern void usb_umount(void);
extern void usb_title_change(int flag);
extern int 	usb_operation(void);

#endif /* USB_STORAGE_H_ */
