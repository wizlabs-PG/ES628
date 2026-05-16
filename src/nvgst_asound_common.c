/*
 * Copyright (c) 2015 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#include "nvgst_asound_common.h"

char *
nvgst_asound_get_device ()
{
  int card_num = -1, device_num = -1;
  char ctl_name[8];
  char dev_name[16];
  snd_ctl_t *ctl;
  snd_pcm_t *handle;

  if (snd_pcm_open (&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) == 0) {
    snd_pcm_close (handle);
    return strdup ("default");
  }

  while (snd_card_next (&card_num) == 0 && card_num > -1) {
    sprintf (ctl_name, "hw:%d", card_num);
    if (snd_ctl_open (&ctl, ctl_name, 0) < 0)
        continue;
    device_num = -1;
    while (snd_ctl_pcm_next_device (ctl, &device_num) == 0 && device_num > -1) {
      sprintf (dev_name, "hw:%d,%d", card_num, device_num);
      if (snd_pcm_open (&handle, dev_name, SND_PCM_STREAM_PLAYBACK, 0) == 0) {
        snd_pcm_close (handle);
        snd_ctl_close (ctl);
        return strdup (dev_name);
      }
    }
    snd_ctl_close (ctl);
  }

  return NULL;
}

