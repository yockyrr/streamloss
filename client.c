#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "opus/include/opus.h"
#include "encdec.h"
#include "wavein.h"

int main(void) {
	opus_int32 samplerate;
	int channels;
	int application;

	/* Parameters */
	samplerate = 48000;
	channels = 2;
	application = OPUS_APPLICATION_VOIP;

	/* Set up encdec */
	t_encdec transport;
	transport = create_encdec(samplerate, channels, application);
	fprintf(stderr, "Created encdec.\n");

	/* List devs */
	t_waveindevs wavein_handle;
	wavein_handle = wavein_get_waveindevs();
	size_t i;
	for(i=0;i<wavein_handle->n;++i) {
		fprintf(stderr, "---\n");
		fprintf(stderr, "Device: #%lu\n", i);
		fprintf(stderr, "Manufacturer ID: %u\n",
			wavein_handle->caps[i].wMid);
		fprintf(stderr, "Product ID: %u\n",
			wavein_handle->caps[i].wPid);
		fprintf(stderr, "Version: %u.%u\n",
			wavein_handle->caps[i].vDriverVersion/256,
			wavein_handle->caps[i].vDriverVersion%256);
		fprintf(stderr, "Product Name: %s\n",
			wavein_handle->caps[i].szPname);
		fprintf(stderr, "Channels: %u\n",
			wavein_handle->caps[i].wChannels);
		fprintf(stderr, "Formats: ");
		size_t j;
		for(j=0;j<(wavein_handle->forms[i].n);++j)
			fprintf(stderr, "(%lu %hu %hu) ",
				wavein_handle->forms[i].f[j].rate,
				wavein_handle->forms[i].f[j].nchannels,
				wavein_handle->forms[i].f[j].bitdepth);
		fprintf(stderr, "\n");
	}

	wavein_free_waveindevs(wavein_handle);

	/* Open WaveIn Device */
	HWAVEIN h;
	h = wavein_open_device(0, 1, 48000, 16);

	/* Create buffers */
	t_pcmbufs wavein_bufs;
	wavein_bufs = wavein_create_pcmbufs(h, 2, 128);

	/* Begin input */
	wavein_start(h);
	fprintf(stderr, "Started input.\n");

	getchar();

	/* End input */
	wavein_stop(h);

	/* Free buffers */
	wavein_free_bufs(h, wavein_bufs);

	/* Close WaveIn Device */
	wavein_close_device(h);
	
	/* Destroy encdec */
	destroy_encdec(transport);
	fprintf(stderr, "Destroyed encdec.\n");
	return 0;
}
