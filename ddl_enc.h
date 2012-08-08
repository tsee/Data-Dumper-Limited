#ifndef DDL_ENC_H_
#define DDL_ENC_H_

#include "EXTERN.h"
#include "perl.h"

typedef struct {
  SV *src;
  char *buf_start;
  char *buf_end;
  char *pos;
  U32 depth;
} ddl_encoder_t;

/* Builds the C-level configuration struct.
 * Automatically freed at scope boundary. */
ddl_encoder_t *build_encoder_struct(pTHX_ HV *opt, SV *src_data);

void ddl_dump_sv(pTHX_ ddl_encoder_t *enc, SV *src);
void ddl_dump_rv(pTHX_ ddl_encoder_t *enc, SV *src);


#endif
