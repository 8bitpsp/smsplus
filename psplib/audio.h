#ifndef _PSP_AUDIO_H
#define _PSP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_VOLUME_MAX 0x8000

typedef struct 
{
  short Left;
  short Right;
} PspSample;

typedef void (*pspAudioCallback)(void *buf, unsigned int *length, void *userdata);

int pspAudioInit(int sample_count);
void pspAudioSetVolume(int channel, int left, int right);
void pspAudioShutdown();
void pspAudioSetChannelCallback(int channel, pspAudioCallback callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif // _PSP_AUDIO_H

