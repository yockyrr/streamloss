#ifndef ENCDEC_H
#define ENCDEC_H

typedef struct encdec {
	OpusEncoder *enc;
	OpusDecoder *dec;
} t_encdec_struct, *t_encdec;

t_encdec create_encdec(opus_int32 samplerate, int channels, int application);

void destroy_encdec(t_encdec in);

#endif
