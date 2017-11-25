#ifndef WAVEIN_H
#define WAVEIN_H

#include <windows.h>
#include <mmsystem.h>
typedef struct format {
	unsigned char bitdepth;
	unsigned char nchannels;
	long unsigned int rate;
} t_format;

typedef struct formats {
	size_t n;
	t_format *f;
} t_formats;

typedef struct waveindevs {
        size_t n;
        WAVEINCAPS *caps;
	t_formats *forms;
} waveindevs_struct, *t_waveindevs;

typedef struct pcmbufs {
        size_t n;
        WAVEHDR *bufs;
} pcmbufs_struct, *t_pcmbufs;

t_waveindevs wavein_get_waveindevs(void);

void wavein_free_waveindevs(t_waveindevs in);

HWAVEIN wavein_open_device(UINT deviceID, size_t channels, size_t samplerate,
	size_t bitdepth);

int wavein_close_device(HWAVEIN hWaveIn);

t_pcmbufs wavein_create_pcmbufs(HWAVEIN hWaveIn, size_t n, size_t byte_size);

int wavein_start(HWAVEIN hWaveIn);

int wavein_stop(HWAVEIN hWaveIn);

void wavein_free_bufs(HWAVEIN hWaveIn, t_pcmbufs in);

#endif
