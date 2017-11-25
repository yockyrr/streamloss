#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include "utils.h"

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

t_waveindevs wavein_get_waveindevs(void) {
	/* Allocate struct */
	t_waveindevs ret;
	ret = NULL;
	ret = malloc(sizeof *ret);
	if(!ret) {
		sl_error("Failed to allocate memory for device capabilities",
			"malloc() returned 0", __FILE__, __LINE__);
		return ret;
	}

	/* Initialise struct */
	ret->n = 0;
	ret->caps = NULL;
	ret->forms = NULL;
	UINT devs;
	devs = 0;
	devs = waveInGetNumDevs();
	if(!devs) {
		sl_error("No devices found", "waveInGetNumDevs() returned 0",
			__FILE__, __LINE__);
		return ret;
	}

	/* Get capabilities for every dev */
	UINT i;
	for(i=0;i<devs;++i) {
		WAVEINCAPS caps = { 0 };

		MMRESULT mmr;
		mmr = waveInGetDevCaps(i, &caps, sizeof caps);
		/* Handle errors */
		switch(mmr) {
			case MMSYSERR_BADDEVICEID:
				sl_warning("Could not get capabilities of devi"
					"ce", "waveInDevCaps() returned MMSYSE"
					"RR_BADDEVICEID", __FILE__, __LINE__);
				break;
			case MMSYSERR_NODRIVER:
				sl_warning("Could not get capabilities of devi"
					"ce", "waveInDevCaps() returned MMSYSE"
					"RR_NODRIVER", __FILE__, __LINE__);
				break;
			case MMSYSERR_NOMEM:
				sl_warning("Could not get capabilities of devi"
					"ce", "waveInDevCaps() returned MMSYSE"
					"RR_NOMEM", __FILE__, __LINE__);
				break;
		}
		if(mmr != MMSYSERR_NOERROR)
			continue;
		
		/* Find formats */
		t_format *f, *tmpf;
		f = NULL;
		tmpf = NULL;
		size_t nformats;
		nformats = 0;
		long unsigned int rates[16] = { 11025, 11025, 11025, 11025,
			22050, 22050, 22050, 22050, 44100, 44100, 44100, 44100,
			96000, 96000, 96000, 96000 };
		unsigned char channels[16] = { 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2,
			2, 1, 1, 2, 2 };
		unsigned char bitdepths[16] = { 8, 16, 8, 16, 8, 16, 8, 16, 8,
			16, 8, 16, 8, 16, 8, 16 };
		DWORD formats[16] = { WAVE_FORMAT_1M08, WAVE_FORMAT_1M16,
			WAVE_FORMAT_1S08, WAVE_FORMAT_1S16, WAVE_FORMAT_2M08,
			WAVE_FORMAT_2M16, WAVE_FORMAT_2S08, WAVE_FORMAT_2S16,
			WAVE_FORMAT_4M08, WAVE_FORMAT_4M16, WAVE_FORMAT_4S08,
			WAVE_FORMAT_4S16, WAVE_FORMAT_96M08, WAVE_FORMAT_96M16,
			WAVE_FORMAT_96S08, WAVE_FORMAT_96S16 };
		size_t j;
		for(j=0;j<16;++j) {
			if(((caps.dwFormats) & formats[j]) == formats[j]) {
				++nformats;
				tmpf = NULL;
				tmpf = realloc(f, nformats * sizeof *f);
				if(!tmpf) {
					sl_error("Could not get formats of dev"
						"ice", "realloc() returned NUL"
						"L", __FILE__, __LINE__);
					--(nformats);
					break;
				}
				f = tmpf;
				f[nformats - 1].bitdepth = bitdepths[j];
				f[nformats - 1].nchannels = channels[j];
				f[nformats - 1].rate = rates[j];
			}
		}
		
		/* Copy capabilities to struct */
		++(ret->n);
		WAVEINCAPS *tmp;
		tmp = NULL;
		tmp = realloc(ret->caps, ret->n * sizeof *tmp);
		if(!tmp) {
			sl_error("Failed to allocate memory for device capabil"
				"ities", "realloc() returned 0", __FILE__,
				__LINE__);
			--(ret->n);
			return ret;
		}
		ret->caps = tmp;
		ret->caps[(ret->n)-1] = caps;
		t_formats *tmpfs;
		tmpfs = NULL;
		tmpfs = realloc(ret->forms, ret->n * sizeof *tmpfs);
		if(!tmpfs) {
			sl_error("Failed to allocate memory for device formats"
				, "realloc() returned 0", __FILE__, __LINE__);
			--(ret->n);
			return ret;
		}
		ret->forms = tmpfs;
		ret->forms[(ret->n)-1].f = f;
		ret->forms[(ret->n)-1].n = nformats;
	}
	return ret;
}

void wavein_free_waveindevs(t_waveindevs in) {
	/* Free caps */
	free(in->caps);

	/* Free forms */
	size_t i;
	for(i=0;i<(in->n);++i)
		free(in->forms->f);
	free(in->forms);

	/* Free struct */
	free(in);
	return;
}

DWORD WINAPI wavein_callback(LPVOID arg) {
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0)) {
		switch(msg.message) {
			case MM_WIM_DATA:
				sl_warning("WaveIn block", "wavein_callback() "
					"received MM_WIM_DATA", __FILE__,
					__LINE__);
				fprintf(stderr, "Size: %u\n", ((WAVEHDR *)msg.lParam)->dwBytesRecorded);
				size_t i;
				for(i=0;i<((WAVEHDR*)msg.lParam)->dwBytesRecorded;++i) {
					fprintf(stderr, "%hu ", ((WAVEHDR*)msg.lParam)->lpData[i]);
				}
				fprintf(stderr, "\n");
				
				/* Requeue buffer */
				if(((WAVEHDR *)msg.lParam)->dwUser == 0x1) {
					/* User data has been set to no longer
					 * queue this buffer
					 */
					return 0;
				}
				MMRESULT mmr;
				mmr = waveInAddBuffer((HWAVEIN)msg.wParam,
					(WAVEHDR *)msg.lParam, sizeof(WAVEHDR));
				switch(mmr) {
					case MMSYSERR_INVALHANDLE:
						sl_error("Failed to requeue bu"
							"ffer", "waveInAddBuff"
							"er() returned MMSYSER"
							"R_INVALHANDLE",
							__FILE__, __LINE__);
						return 1;
						break;
					case MMSYSERR_NODRIVER:
						sl_error("Failed to requeue bu"
							"ffer", "waveInAddBuff"
							"er() returned MMSYSER"
							"R_NODRIVER", __FILE__,
							__LINE__);
						return 1;
						break;
					case MMSYSERR_NOMEM:
						sl_error("Failed to requeue bu"
							"ffer", "waveInAddBuff"
							"er() returned MMSYSER"
							"R_NOMEM", __FILE__,
							__LINE__);
						return 1;
						break;
					case WAVERR_UNPREPARED:
						sl_error("Failed to requeue bu"
							"ffer", "waveInAddBuff"
							"er() returned WAVERR_"
							"UNPREPARED", __FILE__,
							__LINE__);
						return 1;
						break;
				}
				break;
			case MM_WIM_CLOSE:
				sl_warning("WaveIn close", "wavein_callback() "
					"received MM_WIM_CLOSE", __FILE__,
					__LINE__);
				return 0;
				break;
			case MM_WIM_OPEN:
				sl_warning("WaveIn open", "wavein_callback() r"
					"eceived MM_WIM_OPEN", __FILE__,
					__LINE__);
				break;
		}
	}
	return 0;
}

HWAVEIN wavein_open_device(UINT deviceID, size_t channels, size_t samplerate,
	size_t bitdepth) {
	/* Set parameters */
	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = channels;
	format.nSamplesPerSec = samplerate;
	format.wBitsPerSample = bitdepth;
	format.cbSize = 0;
	format.nAvgBytesPerSec = samplerate * channels * (bitdepth/8);
	format.nBlockAlign = channels * (bitdepth/8);

	/* Create thread */
	HANDLE waveInThread;
	DWORD callback_threadID;
	callback_threadID = 0;
	HWAVEIN hWaveIn;
	waveInThread = NULL;
	waveInThread = CreateThread(NULL, 0, &wavein_callback,
		(LPVOID)&hWaveIn, 0, &callback_threadID);
	if(!waveInThread) {
		sl_error("Failed to create callback thread", "CreateThread() r"
			"eturned NULL", __FILE__, __LINE__);
		return NULL;
	}
	if(!CloseHandle(waveInThread)) {
		sl_error("Failed to close handle to callback thread", "CloseHa"
			"ndle() returned false", __FILE__, __LINE__);
		return NULL;
	}

	/* Open device */
	MMRESULT mmr;
	mmr = waveInOpen(&hWaveIn, deviceID, &format,
		callback_threadID, 0,
		CALLBACK_THREAD | WAVE_FORMAT_DIRECT);
	switch(mmr) {
		case MMSYSERR_ALLOCATED:
			sl_error("Selected audio device is already in use",
				"waveInOpen() returned MMSYSERR_ALLOCATED",
				__FILE__, __LINE__);
			break;
		case MMSYSERR_BADDEVICEID:
			sl_error("Selected audio device does not exist", "wave"
				"InOpen() returned MMSYSERR_BADDEVICEID",
				__FILE__, __LINE__);
			break;
		case MMSYSERR_NODRIVER:
			sl_error("Selected audio device does not have a valid "
				"driver", "waveInOpen() returned MMSYSERR_NODR"
				"IVER", __FILE__, __LINE__);
			break;
		case MMSYSERR_NOMEM:
			sl_error("Unable to open selected audio device: no mem"
				"ory", "waveInOpen() returned MMSYSERR_NOMEM",
				__FILE__, __LINE__);
			break;
		case WAVERR_BADFORMAT:
			sl_error("Unable to open selected audio device: bad au"
				"dio format selected", "waveInOpen() returned "
				"MMSYSERR_BADFORMAT", __FILE__, __LINE__);
			break;
	}
	fprintf(stderr, "Opened hwavein: %p\n", hWaveIn);
	return hWaveIn;
}

int wavein_close_device(HWAVEIN hWaveIn) {
	MMRESULT mmr;
	mmr = waveInClose(hWaveIn);
	switch(mmr) {
		case MMSYSERR_INVALHANDLE:
			sl_error("Unable to close waveIn device", "waveInClose"
				"() returned MMSYSERR_INVALHANDLE", __FILE__,
				__LINE__);
			break;
		case MMSYSERR_NODRIVER:
			sl_error("Unable to close waveIn device", "waveInClose"
				"() returned MMSYSERR_NODRIVER", __FILE__,
				__LINE__);
			break;
		case MMSYSERR_NOMEM:
			sl_error("Unable to close waveIn device", "waveInClose"
				"() returned MMSYSERR_NOMEM", __FILE__,
				__LINE__);
			break;
		case WAVERR_STILLPLAYING:
			sl_error("Unable to close waveIn device", "waveInClose"
				"() returned WAVERR_STILLPLAYING", __FILE__,
				__LINE__);
			break;
	}
	if(mmr == MMSYSERR_NOERROR)
		return 0;
	return -1;
}

t_pcmbufs wavein_create_pcmbufs(HWAVEIN hWaveIn, size_t n, size_t byte_size) {
	/* Allocate struct */
	t_pcmbufs ret;
	ret = NULL;
	ret = malloc(sizeof *ret);
	if(!ret) {
		sl_error("Failed to allocate memory for buffers", "malloc() re"
			"turned 0", __FILE__, __LINE__);
		return NULL;
	}

	/* Allocate array of headers */
	ret->n = 0;
	ret->bufs = NULL;
	ret->bufs = calloc(n, sizeof *(ret->bufs));
	if(!(ret->bufs)) {
		sl_error("Failed to allocate memory for buffers", "malloc() re"
			"turned 0", __FILE__, __LINE__);
		return NULL;
	}

	/* Allocate buffers individually */
	size_t i;
	MMRESULT mmr;
	for(i=0;i<n;++i,++(ret->n)) {
		ret->bufs[i].lpData = NULL;
		ret->bufs[i].lpData = malloc(byte_size);
		if(!(ret->bufs[i].lpData)) {
			sl_error("Failed to allocate memory for a buffer", "ma"
				"lloc() returned 0", __FILE__, __LINE__);
			return ret;
		}

		/* Prepare header */
		ret->bufs[i].dwBufferLength = byte_size;
		ret->bufs[i].dwBytesRecorded = 0;
		ret->bufs[i].dwUser = 0x0;
		ret->bufs[i].dwFlags = 0;
		ret->bufs[i].dwLoops = 0;
		mmr = waveInPrepareHeader(hWaveIn, &(ret->bufs[i]),
			sizeof ret->bufs[i]);
		switch(mmr) {
			case MMSYSERR_INVALHANDLE:
				sl_error("Failed to prepare buffer", "waveInPr"
					"epareHeader() returned MMSYSERR_INVAL"
					"HANDLE", __FILE__, __LINE__);
				break;
			case MMSYSERR_NODRIVER:
				sl_error("Failed to prepare buffer", "waveInPr"
					"epareHeader() returned MMSYSERR_NODRI"
					"VER", __FILE__, __LINE__);
				break;
			case MMSYSERR_NOMEM:
				sl_error("Failed to prepare buffer", "waveInPr"
					"epareHeader() returned MMSYSERR_NOMEM"
					, __FILE__, __LINE__);
				break;
		}
		if(mmr != MMSYSERR_NOERROR) {
			free(ret->bufs[i].lpData);
			return ret;
		}
		fprintf(stderr, "Prepared buffer: %p\n", &(ret->bufs[i]));
		mmr = waveInAddBuffer(hWaveIn, &(ret->bufs[i]),
			sizeof ret->bufs[i]);
		switch(mmr) {
			case MMSYSERR_INVALHANDLE:
				sl_error("Failed to add buffer", "waveInAddBuf"
					"fer() returned MMSYSERR_INVALHANDLE",
					__FILE__, __LINE__);
				break;
			case MMSYSERR_NODRIVER:
				sl_error("Failed to add buffer", "waveInAddBuf"
					"fer() returned MMSYSERR_NODRIVER",
					__FILE__, __LINE__);
				break;
			case MMSYSERR_NOMEM:
				sl_error("Failed to add buffer", "waveInAddBuf"
					"fer() returned MMSYSERR_NOMEM",
					__FILE__, __LINE__);
				break;
			case WAVERR_UNPREPARED:
				sl_error("Failed to add buffer", "waveInAddBuf"
					"fer() returned WAVERR_UNPREPARED",
					__FILE__, __LINE__);
				break;
		}
		if(mmr != MMSYSERR_NOERROR) {
			mmr = waveInUnprepareHeader(hWaveIn, &(ret->bufs[i]),
				sizeof ret->bufs[i]);
			switch(mmr) {
				case MMSYSERR_INVALHANDLE:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned MMSYSERR_INVALHANDLE",
						__FILE__, __LINE__);
					break;
				case MMSYSERR_NODRIVER:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned MMSYSERR_NODRIVER",
						__FILE__, __LINE__);
					break;
				case MMSYSERR_NOMEM:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned MMSYSERR_NOMEM",
						__FILE__, __LINE__);
					break;
				case WAVERR_STILLPLAYING:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned WAVERR_STILLPLAYING",
						__FILE__, __LINE__);
					break;
			}
			free(ret->bufs[i].lpData);
			return ret;
		}
	}
	return ret;
}

int wavein_start(HWAVEIN hWaveIn) {
	MMRESULT mmr;
	mmr = waveInStart(hWaveIn);
	switch(mmr) {
		case MMSYSERR_INVALHANDLE:
			sl_error("Failed to start audio device", "waveInStart("
				") returned MMSYSERR_INVALHANDLE", __FILE__,
				__LINE__);
			break;
		case MMSYSERR_NODRIVER:
			sl_error("Failed to start audio device", "waveInStart("
				") returned MMSYSERR_NODRIVER", __FILE__,
				__LINE__);
			break;
		case MMSYSERR_NOMEM:
			sl_error("Failed to start audio device", "waveInStart("
				") returned MMSYSERR_NOMEM", __FILE__,
				__LINE__);
			break;
	}
	if(mmr == MMSYSERR_NOERROR)
		return 0;
	return -1;
}

int wavein_stop(HWAVEIN hWaveIn) {
	MMRESULT mmr;
	mmr = waveInStop(hWaveIn);
	switch(mmr) {
		case MMSYSERR_INVALHANDLE:
			sl_error("Failed to stop audio device", "waveInStop() "
				"returned MMSYSERR_INVALHANDLE", __FILE__,
				__LINE__);
			break;
		case MMSYSERR_NODRIVER:
			sl_error("Failed to stop audio device", "waveInStop() "
				"returned MMSYSERR_NODRIVER", __FILE__,
				__LINE__);
			break;
		case MMSYSERR_NOMEM:
			sl_error("Failed to stop audio device", "waveInStop() "
				"returned MMSYSERR_NOMEM", __FILE__, __LINE__);
			break;
	}
	if(mmr == MMSYSERR_NOERROR)
		return 0;
	return -1;
}

void wavein_free_bufs(HWAVEIN hWaveIn, t_pcmbufs in) {
	/* Set user data that buffer should no longer be queued. */
	size_t i;
	for(i=0;i<(in->n);++i)
		in->bufs[i].dwUser = 0x1;
	for(i=0;i<(in->n);++i) {
		MMRESULT mmr;
		/* Unprepare headers individually */
		do {
			mmr = waveInUnprepareHeader(hWaveIn, &(in->bufs[i]),
				sizeof in->bufs[i]);
			switch(mmr) {
				case MMSYSERR_INVALHANDLE:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned MMSYSERR_INVALHANDLE",
						__FILE__, __LINE__);
					break;
				case MMSYSERR_NODRIVER:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned MMSYSERR_NODRIVER",
						__FILE__, __LINE__);
					break;
				case MMSYSERR_NOMEM:
					sl_error("Failed to unprepare buffer",
						"waveInUnprepareHeader() retur"
						"ned MMSYSERR_NOMEM",
						__FILE__, __LINE__);
					break;
				case WAVERR_STILLPLAYING:
					sl_warning("Failed to unprepare buffer"
						, "waveInUnprepareHeader() ret"
						"urned WAVERR_STILLPLAYING",
						__FILE__, __LINE__);
					break;
			}
		} while(mmr == WAVERR_STILLPLAYING);

		/* Free memory for buffer */
		free(in->bufs[i].lpData);
	}

	/* Free the buffer struct */
	free(in);
	return;
}
