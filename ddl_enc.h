#ifndef DDL_ENC_H_
#define DDL_ENC_H_

#include "EXTERN.h"
#include "perl.h"


typedef struct PTABLE * ptable_ptr;
typedef struct {
  char *buf_start;
  char *buf_end;
  char *pos;
  U32 flags;
  unsigned int depth;
  ptable_ptr seenhash;
} ddl_encoder_t;

ddl_encoder_t *build_encoder_struct(pTHX_ HV *opt, SV *src_data);

/* entry point */
void ddl_dump_sv(pTHX_ ddl_encoder_t *enc, SV *src);

#endif
