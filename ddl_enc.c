#include "ddl_enc.h"

#include "ppport.h"

#define PERL_NO_GET_CONTEXT

#define INITIALIZATION_SIZE 16384
#define MAX_DEPTH 10000
#define GROWTH_FACTOR 1.5

/* three extra for rounding, sign, and end of string */
#define IVUV_MAXCHARS (sizeof (UV) * CHAR_BIT * 28 / 93 + 3)

#define F_UNDEF_BLESSED 1UL

/* some static function declarations */
static void ddl_dump_rv(pTHX_ ddl_encoder_t *enc, SV *src);
static void ddl_dump_av(pTHX_ ddl_encoder_t *enc, AV *src);
static void ddl_dump_hv(pTHX_ ddl_encoder_t *enc, HV *src);
static void ddl_dump_hk(pTHX_ ddl_encoder_t *enc, HE *src);

void ddl_destructor_hook(void *p)
{
  ddl_encoder_t *enc = (ddl_encoder_t *)p;
  /* Exception cleanup. Under normal operation, we should have
   * assigned NULL after we're done. */
  Safefree(enc->buf_start);
  Safefree(enc);
}

/* Builds the C-level configuration and state struct.
 * Automatically freed at scope boundary. */
ddl_encoder_t *
build_encoder_struct(pTHX_ HV *opt, SV *src_data)
{
  ddl_encoder_t *enc;
  SV **svp;

  Newx(enc, 1, ddl_encoder_t);
  /* Register our structure for destruction on scope exit */
  SAVEDESTRUCTOR(&ddl_destructor_hook, (void *)enc);

  /* Init struct */
  enc->src = src_data;
  Newx(enc->buf_start, INITIALIZATION_SIZE, char);
  enc->buf_end = enc->buf_start + INITIALIZATION_SIZE;
  enc->pos = enc->buf_start;
  enc->depth = 0;
  enc->flags = 0;

  if (opt != NULL) {
    if ( (svp = hv_fetchs(opt, "undef_blessed", 0)) && SvOK(*svp) && SvIV(*svp) != 0 )
      enc->flags |= F_UNDEF_BLESSED;
  }

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
ddl_buf_cat_char_int(pTHX_ ddl_encoder_t *enc, const char c)
{
  BUF_SIZE_ASSERT(enc, 1);
  *enc->pos++ = c;
}
#define ddl_buf_cat_char(enc, c) ddl_buf_cat_char_int(aTHX_ enc, c)

inline void
ddl_buf_cat_char_nocheck_int(pTHX_ ddl_encoder_t *enc, const char c)
{
  *enc->pos++ = c;
}
#define ddl_buf_cat_char_nocheck(enc, c) ddl_buf_cat_char_nocheck_int(aTHX_ enc, c)


/* Entry point for serialization. Dumps generic SVs and delegates
 * to more specialized functions for RVs, etc. */
void
ddl_dump_sv(pTHX_ ddl_encoder_t *enc, SV *src)
{
  SvGETMAGIC(src);

  /* dump strings */
  if (SvPOKp(src)) {
    STRLEN len;
    char *str = SvPV(src, len);
    BUF_SIZE_ASSERT(enc, 2 + len);
    ddl_buf_cat_char_nocheck(enc, '"');
    /* FIXME */
    //encode_str(enc, str, len, SvUTF8(src));
    ddl_buf_cat_char_nocheck(enc, '"');
  }
  /* dump floats */
  else if (SvNOKp(src)) {
    BUF_SIZE_ASSERT(enc, NV_DIG + 32);
    Gconvert(SvNVX(src), NV_DIG, 0, enc->pos);
    enc->pos += strlen(enc->pos);
  }
  /* dump ints */
  else if (SvIOKp(src)) {
    /* we assume we can always read an IV as a UV and vice versa
     * we assume two's complement
     * we assume no aliasing issues in the union */
    if (SvIsUV(src) ? SvUVX(src) <= 59000
                    : SvIVX(src) <= 59000 && SvIVX(src) >= -59000)
    {
      /* optimise the "small number case"
       * code will likely be branchless and use only a single multiplication
       * works for numbers up to 59074 */
      I32 i = SvIVX(src);
      U32 u;
      char digit, nz = 0;

      BUF_SIZE_ASSERT(enc, 6);

      *enc->pos = '-'; enc->pos += i < 0 ? 1 : 0;
      u = i < 0 ? -i : i;

      /* convert to 4.28 fixed-point representation */
      u *= ((0xfffffff + 10000) / 10000); /* 10**5, 5 fractional digits */

      /* now output digit by digit, each time masking out the integer part
       * and multiplying by 5 while moving the decimal point one to the right,
       * resulting in a net multiplication by 10.
       * we always write the digit to memory but conditionally increment
       * the pointer, to enable the use of conditional move instructions. */
      digit = u >> 28; *enc->pos = digit + '0'; enc->pos += (nz = nz || digit); u = (u & 0xfffffffUL) * 5;
      digit = u >> 27; *enc->pos = digit + '0'; enc->pos += (nz = nz || digit); u = (u & 0x7ffffffUL) * 5;
      digit = u >> 26; *enc->pos = digit + '0'; enc->pos += (nz = nz || digit); u = (u & 0x3ffffffUL) * 5;
      digit = u >> 25; *enc->pos = digit + '0'; enc->pos += (nz = nz || digit); u = (u & 0x1ffffffUL) * 5;
      digit = u >> 24; *enc->pos = digit + '0'; enc->pos += 1; /* correctly generate '0' */
    }
    else {
      /* large integer, use the (rather slow) snprintf way. */
      BUF_SIZE_ASSERT(enc, IVUV_MAXCHARS);
      enc->pos +=
         SvIsUV(src)
            ? snprintf(enc->pos, IVUV_MAXCHARS, "%"UVuf, (UV)SvUVX(src))
            : snprintf(enc->pos, IVUV_MAXCHARS, "%"IVdf, (IV)SvIVX(src));
    }
  } /* end is an integer */
  /* undef */
  else if (!SvOK(src)) {
    ddl_buf_cat_str_s(enc, "undef");
  }
  /* dump references */
  else if (SvROK(src))
    ddl_dump_rv(aTHX_ enc, SvRV(src));
  else {
    croak("Attempting to dump unsupported or invalid SV");
  }
}

/* Dump references, delegates to more specialized functions for
 * arrays, hashes, etc. */
static void
ddl_dump_rv(pTHX_ ddl_encoder_t *enc, SV *src)
{
  svtype svt;

  if (++enc->depth > MAX_DEPTH) {
    croak("Reached maximum recursion depth of %u. Aborting", MAX_DEPTH);
  }

  SvGETMAGIC(src);
  svt = SvTYPE(src);

  if (SvOBJECT(src)) {
    if (enc->flags & F_UNDEF_BLESSED)
      ddl_buf_cat_str_s(enc, "undef");
    else
      croak("encountered object '%s', but neither allow_blessed nor convert_blessed settings are enabled",
            SvPV_nolen(sv_2mortal(newRV_inc(src))));
  }
  else if (svt == SVt_PVHV)
    ddl_dump_hv(aTHX_ enc, (HV *)src);
  else if (svt == SVt_PVAV)
    ddl_dump_av(aTHX_ enc, (AV *)src);
  else if (svt < SVt_PVAV) {
    ddl_buf_cat_char(enc, '\\');
    ddl_dump_sv(aTHX_ enc, src);
  }
  /* else if (enc->json.flags & F_ALLOW_UNKNOWN)
   *  encode_str (enc, "null", 4, 0);
   */
  else {
    croak("found %s, but it is not representable by Data::Dumper::Limited serialization",
           SvPV_nolen(sv_2mortal(newRV_inc(src))));
  }
}


static void
ddl_dump_av(pTHX_ ddl_encoder_t *enc, AV *src)
{
  UV i, n;
  SV **svp;

  n = av_len(src)+1;
  if (n == 0) {
    ddl_buf_cat_str_s(enc, "[]");
    return;
  }

  ddl_buf_cat_char(enc, '[');

  svp = av_fetch(src, 0, 0);
  if (svp == NULL)
    croak("Got NULL SV from av_fetch");
  ddl_dump_sv(aTHX_ enc, *svp);

  for (i = 1; i < n; ++i) {
    ddl_buf_cat_char(enc, ',');
    svp = av_fetch(src, i, 0);
    if (svp != NULL)
      ddl_dump_sv(aTHX_ enc, *svp);
    else
      ddl_buf_cat_str_s(enc, "undef");
  }

  ddl_buf_cat_char(enc, ']');
}


static void
ddl_dump_hv(pTHX_ ddl_encoder_t *enc, HV *src)
{
  HE *he;
  ddl_buf_cat_char(enc, '{');
  if (hv_iterinit(src) || SvMAGICAL(src)) {
    if ((he = hv_iternext(src))) {
      for (;;) {
        ddl_dump_hk(aTHX_ enc, he);
        ddl_buf_cat_char(enc, ','); /* see comments in ddl_dump_hk */
        ddl_dump_sv(aTHX_ enc, SvMAGICAL(src) ? hv_iterval(src, he) : HeVAL(he));

        if (!(he = hv_iternext(src)))
          break;

        ddl_buf_cat_char(enc, ',');
      }
    }
  }
  ddl_buf_cat_char(enc, '}');
}

static void
ddl_dump_hk(pTHX_ ddl_encoder_t *enc, HE *src)
{
  /* FIXME we could scan the string to see whether we could
   *       skip quoting the string and instead using a fat comma.
   *       But that's a lot of extra coding work, potentially slow,
   *       and a small gain.
   *       Even if that's not done, we can always use the fat comma
   *       for readability. Maybe make that configurable later? */
  ddl_buf_cat_char(enc, '"');

  if (HeKLEN(src) == HEf_SVKEY) {
    SV *sv = HeSVKEY(src);
    STRLEN len;
    char *str;

    SvGETMAGIC(sv);
    str = SvPV(sv, len);

    /* FIXME */
    /* encode_str(enc, str, len, SvUTF8(sv)); */
  }
  else {
    /* FIXME */
    /* encode_str(enc, HeKEY(src), HeKLEN(src), HeKUTF8(src)); */
  }

  ddl_buf_cat_char(enc, '"');
}

