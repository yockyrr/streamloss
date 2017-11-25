#include <stdio.h>
#include <stdlib.h>
#include "opus/include/opus.h"
#include "utils.h"

typedef struct encdec {
	OpusEncoder *enc;
	OpusDecoder *dec;
} t_encdec_struct, *t_encdec;

t_encdec create_encdec(opus_int32 samplerate, int channels, int application) {
	int err = 0;
	t_encdec ret;
	ret = NULL;
	ret = malloc(sizeof *ret);
	if(!ret) {
		sl_error("Failed to create Opus encoder and decoder", "Malloc "
			"failed", __FILE__, __LINE__);
		return NULL;
	}
	/* Create encoder */
	ret->enc = NULL;
	ret->enc = opus_encoder_create(samplerate, channels, application,
		&err);
	if(!(ret->enc) || err<0) {
		sl_error("Failed to create Opus encoder", opus_strerror(err),
			__FILE__, __LINE__);
		return NULL;
	}

	/* Create decoder */
	ret->dec = NULL;
	ret->dec = opus_decoder_create(samplerate, channels, &err);
	if(!(ret->dec) || err<0) {
		sl_error("Failed to create Opus decoder", opus_strerror(err),
				__FILE__, __LINE__);
		return NULL;
	}

	return ret;
}

void destroy_encdec(t_encdec in) {
	opus_encoder_destroy(in->enc);
	opus_decoder_destroy(in->dec);
	free(in);
	return;
}
