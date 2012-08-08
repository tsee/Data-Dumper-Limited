#ifndef DDL_ENC_H_
#define DDL_ENC_H_

#include "EXTERN.h"
#include "perl.h"


typedef struct PTABLE * ptable_ptr;
typedef struct {
  char *buf_start;     /* ptr to "physical" start of output buffer */
  char *buf_end;       /* ptr to end of output buffer */
  char *pos;           /* ptr to current position within output buffer */
  U32 flags;           /* flag-like options: See F_* defines in ddl_enc.c */
  unsigned int depth;  /* current Perl-ref recursion depth */
  ptable_ptr seenhash; /* ptr table for avoiding circular refs */
} ddl_encoder_t;

ddl_encoder_t *build_encoder_struct(pTHX_ HV *opt, SV *src_data);

/* entry point */
void ddl_dump_sv(pTHX_ ddl_encoder_t *enc, SV *src);

#endif
