#include "ddl_enc.h"

#include "ppport.h"

#define PERL_NO_GET_CONTEXT
#define INITIALIZATION_SIZE 16384
#define GROWTH_FACTOR 1.5

void ddl_destructor_hook(void *p)
{
  ddl_encoder_t *enc = (ddl_encoder_t *)p;
  /* Exception cleanup. Under normal operation, we should have
   * assigned NULL after we're done. */
  Safefree(enc->buf_start);
  Safefree(enc);
}

ddl_encoder_t *
build_encoder_struct(pTHX_ HV *opt, SV *src_data)
{
  ddl_encoder_t *enc;

  Newx(enc, 1, ddl_encoder_t);
  /* Register our structure for destruction on scope exit */
  SAVEDESTRUCTOR(&ddl_destructor_hook, (void *)enc);

  /* Init struct */
  enc->src = src_data;
  Newx(enc->buf_start, INITIALIZATION_SIZE, char);
  enc->buf_end = enc->buf_start + INITIALIZATION_SIZE;
  enc->pos = enc->buf_start;

  return enc;
}


/* buffer operations */
#define BUF_SPACE(enc) ((enc)->buf_end - (enc)->pos)
#define BUF_SIZE(enc) ((enc)->buf_end - (enc)->buf_start)
#define BUF_NEED_GROW(enc, minlen) ((size_t)BUF_SPACE(enc) <= minlen)
#define BUF_NEED_GROW_TOTAL(enc, minlen) ((size_t)BUF_SIZE(enc) <= minlen)

inline void
ddl_buf_grow_nocheck(pTHX_ ddl_encoder_t *enc, size_t minlen)
{
  const size_t cur_size = BUF_SIZE(enc);
  const size_t new_size = 100 + MAX(minlen, cur_size * GROWTH_FACTOR);
  Renew(enc->buf_start, new_size, char);
}

#define BUF_SIZE_ASSERT(enc, minlen) \
  STMT_START { \
    if (BUF_NEED_GROW(enc, minlen)) \
      ddl_buf_grow_nocheck(aTHX_ (enc), (BUF_SIZE(enc) + minlen)); \
  } STMT_END

#define BUF_SIZE_ASSERT_TOTAL(enc, minlen) \
  STMT_START { \
    if (BUF_NEED_GROW_TOTAL(enc, minlen)) \
      ddl_buf_grow_nocheck(aTHX_ (enc), (minlen)); \
  } STMT_END

inline void
ddl_buf_cat_str(pTHX_ ddl_encoder_t *enc, const char *str, size_t len)
{
  BUF_SIZE_ASSERT(enc, len);
  Copy(str, enc->pos, len, char);
  enc->pos += len;
}

inline void
ddl_buf_cat_str_nocheck(pTHX_ ddl_encoder_t *enc, const char *str, size_t len)
{
  Copy(str, enc->pos, len, char);
  enc->pos += len;
}

#define ddl_buf_cat_str_s(enc, str) ddl_buf_cat_str(aTHX_ enc, (str), strlen(str))

inline void
ddl_buf_cat_char(pTHX_ ddl_encoder_t *enc, const char c)
{
  BUF_SIZE_ASSERT(enc, 1);
  *enc->pos++ = c;
}

inline void
ddl_buf_cat_char_nocheck(pTHX_ ddl_encoder_t *enc, const char c)
{
  *enc->pos++ = c;
}

void
ddl_dump_sv(pTHX_ ddl_encoder_t *enc, SV *src)
{
  if (SvGMAGICAL(src))
    mg_get(src);

  if (!SvOK(src)) {
    ddl_buf_cat_str_s(enc, "undef");
  }
  else {
    croak("Attempting to dump unsupported or invalid SV");
  }
}


void
ddl_dump_sv_non_rv(pTHX_ ddl_encoder_t *enc, SV *src)
{
}

