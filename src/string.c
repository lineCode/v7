/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "internal.h"

V7_PRIVATE val_t String_ctor(struct v7 *v7, val_t this_obj, val_t args) {
  val_t arg0 = v7_array_at(v7, args, 0);
  /* TODO(lsm): if arg0 is not a string, do type conversion */
  val_t res = v7_is_string(arg0) ? arg0 : v7_create_string(v7, "", 0, 1);

  if (v7_is_object(this_obj)) {
    v7_set_property(v7, this_obj, "", 0, V7_PROPERTY_HIDDEN, res);
    return this_obj;
  }

  return res;
}

V7_PRIVATE val_t Str_fromCharCode(struct v7 *v7, val_t this_obj, val_t args) {
  int i, num_args = v7_array_length(v7, args);
  val_t res = v7_create_string(v7, "", 0, 1);   /* Empty string */

  (void) this_obj;
  for (i = 0; i < num_args; i++) {
    char buf[10];
    val_t arg = v7_array_at(v7, args, i);
    Rune r = (Rune) v7_to_double(arg);
    int n = runetochar(buf, &r);
    val_t s = v7_create_string(v7, buf, n, 1);
    res = s_concat(v7, res, s);
  }

  return res;
}

V7_PRIVATE val_t Str_charCodeAt(struct v7 *v7, val_t this_obj, val_t args) {
  size_t i, n;
  const char *p = v7_to_string(v7, &this_obj, &n), *end = p + n;
  val_t res = v7_create_number(NAN), arg = v7_array_at(v7, args, 0);

  if (v7_is_double(arg) && v7_to_double(arg) >= 0 && v7_is_string(this_obj)) {
    Rune r;
    for (i = 0; i < (size_t) v7_to_double(arg) && p < end; i++) {
      p += runetochar((char *) p, &r);
    }
    if (p < end) {
      runetochar((char *) p, &r);
      res = v7_create_number(r);
    }
  }
  return res;
}

#if 0
V7_PRIVATE enum v7_err Str_valueOf(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  if (!is_string(cfa->this_obj)) THROW(V7_TYPE_ERROR);
  TRY(push_string(v7, cfa->this_obj->v.str.buf, cfa->this_obj->v.str.len, 1));
  return V7_OK;
#undef v7
}

static enum v7_err _charAt(struct v7_c_func_arg *cfa, const char **p) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  if (cfa->num_args > 0) {
    long len = utfnlen(cfa->this_obj->v.str.buf, cfa->this_obj->v.str.len),
         idx = _conv_to_int(v7, cfa->args[0]);
    if (idx < 0) idx = len - idx;
    if (idx >= 0 && idx < len)
      return *p = utfnshift(cfa->this_obj->v.str.buf, idx), V7_OK;
  } else
    return *p = cfa->this_obj->v.str.buf, V7_OK;
  return *p = NULL, V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_charAt(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  const char *p;
  TRY(_charAt(cfa, &p));
  TRY(push_string(v7, p, p == NULL ? 0 : 1, 1));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_concat(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long n, blen;
  struct v7_val *str;
  char *p;
  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  blen = cfa->this_obj->v.str.len;
  for (n = 0; n < cfa->num_args; n++) {
    TRY(check_str_re_conv(v7, &cfa->args[n], 0));
    blen += cfa->args[n]->v.str.len;
  }
  str = v7_push_string(v7, cfa->this_obj->v.str.buf, blen, 1);
  p = str->v.str.buf + cfa->this_obj->v.str.len;
  for (n = 0; n < cfa->num_args; n++) {
    memcpy(p, cfa->args[n]->v.str.buf, cfa->args[n]->v.str.len);
    p += cfa->args[n]->v.str.len;
  }
  *p = '\0';
  str->v.str.len = blen;
  return V7_OK;
#undef v7
}

static long _indexOf(char *pp, char *const end, char *p, long blen,
                     uint8_t last) {
  long i, idx = -1;
  if (0 == blen || end - pp == 0) return 0;
  for (i = 0; pp <= (end - blen); i++, pp = utfnshift(pp, 1))
    if (0 == memcmp(pp, p, blen)) {
      idx = i;
      if (!last) break;
    }
  return idx;
}

V7_PRIVATE enum v7_err Str_indexOf(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long idx = -1, pos = 0;
  char *p, *end;
  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  end = p + cfa->this_obj->v.str.len;
  if (cfa->num_args > 0) {
    if (V7_TYPE_UNDEF != cfa->args[0]->type &&
        V7_TYPE_NULL != cfa->args[0]->type) {
      TRY(check_str_re_conv(v7, &cfa->args[0], 0));
      if (cfa->num_args > 1) {
        p = utfnshift(p, pos = _conv_to_int(v7, cfa->args[1]));
      }
      if (p < end)
        idx = _indexOf(p, end, cfa->args[0]->v.str.buf, cfa->args[0]->v.str.len,
                       0);
    } else
      idx = 0;
  }
  if (idx >= 0) idx += pos;
  TRY(push_number(v7, idx));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_lastIndexOf(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long idx = -1;
  char *p, *end;
  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  end = p + cfa->this_obj->v.str.len;
  if (cfa->num_args > 0) {
    TRY(check_str_re_conv(v7, &cfa->args[0], 0));
    if (cfa->num_args > 1) {
      end = utfnshift(p, _conv_to_int(v7, cfa->args[1]) + 1);
    }
    idx = _indexOf(p, end, cfa->args[0]->v.str.buf, cfa->args[0]->v.str.len, 1);
  }
  TRY(push_number(v7, idx));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_localeCompare(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  struct v7_val *arg = cfa->args[0];
  long i, ln = 0, ret = 0;
  Rune s, t;
  char *ps, *pt, *end;
  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  TRY(check_str_re_conv(v7, &arg, 0));
  ps = cfa->this_obj->v.str.buf;
  pt = arg->v.str.buf;
  end = ps + cfa->this_obj->v.str.len;
  if (arg->v.str.len < cfa->this_obj->v.str.len) {
    end = ps + arg->v.str.len;
    ln = 1;
  } else if (arg->v.str.len > cfa->this_obj->v.str.len) {
    ln = -1;
  }
  for (i = 0; ps < end; i++) {
    ps += chartorune(&s, ps);
    pt += chartorune(&t, pt);
    if (s < t) {
      ret = -1;
      break;
    }
    if (s > t) {
      ret = 1;
      break;
    }
  }
  if (0 == ret) ret = ln;
  TRY(push_number(v7, ret));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_match(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  struct v7_val *arg = cfa->args[0];
  struct slre_loot sub;
  struct v7_val *arr = NULL;
  unsigned long shift = 0;

  if (cfa->num_args > 0) {
    TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
    TRY(check_str_re_conv(v7, &arg, 1));
    TRY(regex_check_prog(arg));
    do {
      if (!slre_exec(arg->v.str.prog, arg->fl.fl.re_flags,
          cfa->this_obj->v.str.buf + shift, &sub)) {
        struct slre_cap *ptok = sub.caps;
        int i;
        if (NULL == arr) {
          arr = v7_push_new_object(v7);
          v7_set_class(arr, V7_CLASS_ARRAY);
        }
        shift = ptok->end - cfa->this_obj->v.str.buf;
        for (i = 0; i < sub.num_captures; i++, ptok++)
          v7_append(v7, arr, v7_mkv(v7, V7_TYPE_STR, ptok->start,
                                  ptok->end - ptok->start, 1));
      }
    } while ((arg->fl.fl.re_flags & SLRE_FLAG_G) &&
             shift < cfa->this_obj->v.str.len);
  }
  if (arr == NULL) TRY(v7_make_and_push(v7, V7_TYPE_NULL));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_replace(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  struct v7_val *result = v7_push_new_object(v7);
  const char *out_str;
  uint8_t own = 1;
  size_t out_len;
  int old_sp = v7->sp;

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  out_str = cfa->this_obj->v.str.buf;
  out_len = cfa->this_obj->v.str.len;
  if (cfa->num_args > 1) {
    const char *const str_end =
        cfa->this_obj->v.str.buf + cfa->this_obj->v.str.len;
    char *p = cfa->this_obj->v.str.buf;
    uint32_t out_sub_num = 0;
    struct v7_val *re = cfa->args[0], *str_func = cfa->args[1], *arr = NULL;
    struct slre_cap out_sub[V7_RE_MAX_REPL_SUB], *ptok = out_sub;
    struct slre_loot loot;
    TRY(check_str_re_conv(v7, &re, 1));
    TRY(regex_check_prog(re));
    if (v7_is_class(str_func, V7_CLASS_FUNCTION)) {
      arr = v7_push_new_object(v7);
      v7_set_class(arr, V7_CLASS_ARRAY);
      TRY(v7_push(v7, str_func));
    } else
      TRY(check_str_re_conv(v7, &str_func, 0));

    out_len = 0;
    do {
      int i;
      if (slre_exec(re->v.str.prog, re->fl.fl.re_flags, p, &loot)) break;
      if (p != loot.caps->start) {
        ptok->start = p;
        ptok->end = loot.caps->start;
        ptok++;
        out_len += loot.caps->start - p;
        out_sub_num++;
      }

      if (NULL != arr) { /* replace function */
        int old_sp = v7->sp;
        struct v7_val *rez_str;
        for (i = 0; i < loot.num_captures; i++)
          TRY(push_string(v7, loot.caps[i].start,
                          loot.caps[i].end - loot.caps[i].start, 1));
        TRY(push_number(v7, utfnlen(p, loot.caps[0].start - p)));
        TRY(v7_push(v7, cfa->this_obj));
        rez_str = v7_call(v7, cfa->this_obj, loot.num_captures + 2);
        TRY(check_str_re_conv(v7, &rez_str, 0));
        if (rez_str->v.str.len) {
          ptok->start = rez_str->v.str.buf;
          ptok->end = rez_str->v.str.buf + rez_str->v.str.len;
          ptok++;
          out_len += rez_str->v.str.len;
          out_sub_num++;
          v7_append(v7, arr, rez_str);
        }
        TRY(inc_stack(v7, old_sp - v7->sp));
      } else { /* replace string */
        struct slre_loot newsub;
        slre_replace(&loot, cfa->this_obj->v.str.buf, str_func->v.str.buf,
                     &newsub);
        for (i = 0; i < newsub.num_captures; i++) {
          ptok->start = newsub.caps[i].start;
          ptok->end = newsub.caps[i].end;
          ptok++;
          out_len += newsub.caps[i].end - newsub.caps[i].start;
          out_sub_num++;
        }
      }
      p = (char *) loot.caps->end;
    } while ((re->fl.fl.re_flags & SLRE_FLAG_G) && p < str_end);
    if (p < str_end) {
      ptok->start = p;
      ptok->end = str_end;
      ptok++;
      out_len += str_end - p;
      out_sub_num++;
    }
    out_str = malloc(out_len + 1);
    CHECK(out_str, V7_OUT_OF_MEMORY);
    ptok = out_sub;
    p = (char *) out_str;
    do {
      size_t ln = ptok->end - ptok->start;
      memcpy(p, ptok->start, ln);
      p += ln;
      ptok++;
    } while (--out_sub_num);
    *p = '\0';
    own = 0;
  }
  TRY(inc_stack(v7, old_sp - v7->sp));
  v7_init_str(result, out_str, out_len, own);
  result->fl.fl.str_alloc = 1;
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_search(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  struct v7_val *arg = cfa->args[0];
  struct slre_loot sub;
  int shift = -1, utf_shift = -1;

  if (cfa->num_args > 0) {
    TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
    TRY(check_str_re_conv(v7, &arg, 1));
    TRY(regex_check_prog(arg));
    if (!slre_exec(arg->v.str.prog, arg->fl.fl.re_flags,
        cfa->this_obj->v.str.buf, &sub)) {
      shift = sub.caps[0].start - cfa->this_obj->v.str.buf;
    }
  } else
    utf_shift = 0;
  if (shift >= 0) /* calc shift for UTF-8 */
    utf_shift = utfnlen(cfa->this_obj->v.str.buf, shift);
  TRY(push_number(v7, utf_shift));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_slice(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  char *begin, *end;
  long from = 0, to = 0, len;

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  to = len = utfnlen(cfa->this_obj->v.str.buf, cfa->this_obj->v.str.len);
  begin = cfa->this_obj->v.str.buf;
  end = begin + cfa->this_obj->v.str.len;
  if (cfa->num_args > 0) {
    from = _conv_to_int(v7, cfa->args[0]);
    if (from < 0) {
      from += len;
      if (from < 0) from = 0;
    } else if (from > len)
      from = len;
    if (cfa->num_args > 1) {
      to = _conv_to_int(v7, cfa->args[1]);
      if (to < 0) {
        to += len;
        if (to < 0) to = 0;
      } else if (to > len)
        to = len;
    }
  }
  if (from > to) to = from;
  end = utfnshift(begin, to);
  begin = utfnshift(begin, from);
  TRY(v7_make_and_push(v7, V7_TYPE_STR));
  v7_init_str(v7_top_val(v7), begin, end - begin, 1);
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_split(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  struct v7_val *arg = cfa->args[0], *arr = v7_push_new_object(v7), *v;
  struct slre_loot sub;
  int limit = 1000000, elem = 0, i, len;
  unsigned long shift = 0;

  v7_set_class(arr, V7_CLASS_ARRAY);
  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  if (cfa->num_args > 0) {
    if (cfa->num_args > 1 && cfa->args[1]->type == V7_TYPE_NUM)
      limit = cfa->args[1]->v.num;
    if (V7_TYPE_UNDEF != arg->type && V7_TYPE_NULL != arg->type) {
      TRY(check_str_re_conv(v7, &arg, 1));
      TRY(regex_check_prog(arg));
      for (; elem < limit && shift < cfa->this_obj->v.str.len; elem++) {
        if (slre_exec(arg->v.str.prog, arg->fl.fl.re_flags,
            cfa->this_obj->v.str.buf + shift, &sub))
          break;

        if (sub.caps[0].end - sub.caps[0].start == 0) {
          v7_append(
              v7, arr,
              v7_mkv(v7, V7_TYPE_STR, cfa->this_obj->v.str.buf + shift,
                     1, 1));
          shift++;
          } else {
          v7_append(
              v7, arr,
              v7_mkv(v7, V7_TYPE_STR, cfa->this_obj->v.str.buf + shift,
                     sub.caps[0].start - cfa->this_obj->v.str.buf - shift, 1));
          shift = sub.caps[0].end - cfa->this_obj->v.str.buf;
        }

        for (i = 1; i < sub.num_captures; i++) {
          if (sub.caps[i].start != NULL) {
            v = v7_mkv(v7, V7_TYPE_STR, sub.caps[i].start,
                       sub.caps[i].end - sub.caps[i].start, 1);
          } else {
            v = make_value(v7, V7_TYPE_UNDEF);
          }
          v7_append(v7, arr, v);
        }
      }
    }
  }
  len = cfa->this_obj->v.str.len - shift;
  if (elem < limit && len > 0)
    v7_append(v7, arr, v7_mkv(v7, V7_TYPE_STR, cfa->this_obj->v.str.buf + shift,
                              len, 1));
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err _Str_strslice(struct v7_c_func_arg *cfa, int islen) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  char *begin, *end;
  long from = 0, to = 0, len;

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  to = len = utfnlen(cfa->this_obj->v.str.buf, cfa->this_obj->v.str.len);
  begin = cfa->this_obj->v.str.buf;
  end = begin + cfa->this_obj->v.str.len;
  if (cfa->num_args > 0) {
    from = _conv_to_int(v7, cfa->args[0]);
    if (from < 0) from = 0;
    if (from > len) from = len;

    if (cfa->num_args > 1) {
      to = _conv_to_int(v7, cfa->args[1]);
      if (islen) {
        to += from;
      }
      if (to < 0) to = 0;
      if (to > len) to = len;
    }
  }
  if (from > to) {
    long tmp = to;
    to = from;
    from = tmp;
  }
  end = utfnshift(begin, to);
  begin = utfnshift(begin, from);
  TRY(v7_make_and_push(v7, V7_TYPE_STR));
  v7_init_str(v7_top_val(v7), begin, end - begin, 1);
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_substr(struct v7_c_func_arg *cfa) {
  return _Str_strslice(cfa, 1);
}

V7_PRIVATE enum v7_err Str_substring(struct v7_c_func_arg *cfa) {
  return _Str_strslice(cfa, 0);
}


V7_PRIVATE enum v7_err Str_toLowerCase(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long n, blen = 0;
  struct v7_val *str;
  char *p, *end;
  Rune runes[500];

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  end = p + cfa->this_obj->v.str.len;
  for (n = 0; p < end; n++) {
    p += chartorune(&runes[n], p);
    runes[n] = tolowerrune(runes[n]);
    blen += runelen(runes[n]);
  }
  str = v7_push_string(v7, NULL, blen, 1);
  p = str->v.str.buf;
  end = p + blen;
  for (n = 0; p < end; n++) p += runetochar(p, &runes[n]);
  *p = '\0';
  str->v.str.len = blen;
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_toLocaleLowerCase(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long n, blen = 0;
  struct v7_val *str;
  char *p, *end;
  Rune runes[500];

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  end = p + cfa->this_obj->v.str.len;
  for (n = 0; p < end; n++) {
    p += chartorune(&runes[n], p);
    runes[n] = tolowerrune(runes[n]);
    blen += runelen(runes[n]);
  }
  str = v7_push_string(v7, NULL, blen, 1);
  p = str->v.str.buf;
  end = p + blen;
  for (n = 0; p < end; n++) p += runetochar(p, &runes[n]);
  *p = '\0';
  str->v.str.len = blen;
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_toUpperCase(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long n, blen = 0;
  struct v7_val *str;
  char *p, *end;
  Rune runes[500];

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  end = p + cfa->this_obj->v.str.len;
  for (n = 0; p < end; n++) {
    p += chartorune(&runes[n], p);
    runes[n] = toupperrune(runes[n]);
    blen += runelen(runes[n]);
  }
  str = v7_push_string(v7, NULL, blen, 1);
  p = str->v.str.buf;
  end = p + blen;
  for (n = 0; p < end; n++) p += runetochar(p, &runes[n]);
  *p = '\0';
  str->v.str.len = blen;
  return V7_OK;
#undef v7
}

V7_PRIVATE enum v7_err Str_toLocaleUpperCase(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  long n, blen = 0;
  struct v7_val *str;
  char *p, *end;
  Rune runes[500];

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  end = p + cfa->this_obj->v.str.len;
  for (n = 0; p < end; n++) {
    p += chartorune(&runes[n], p);
    runes[n] = toupperrune(runes[n]);
    blen += runelen(runes[n]);
  }
  str = v7_push_string(v7, NULL, blen, 1);
  p = str->v.str.buf;
  end = p + blen;
  for (n = 0; p < end; n++) p += runetochar(p, &runes[n]);
  *p = '\0';
  str->v.str.len = blen;
  return V7_OK;
#undef v7
}

static int _isspase(Rune c) { return isspacerune(c) || isnewline(c); }

V7_PRIVATE enum v7_err Str_trim(struct v7_c_func_arg *cfa) {
#define v7 (cfa->v7) /* Needed for TRY() macro below */
  char *p, *begin = NULL, *end = NULL, *pend;
  Rune rune = ' ';

  TRY(check_str_re_conv(v7, &cfa->this_obj, 0));
  p = cfa->this_obj->v.str.buf;
  pend = p + cfa->this_obj->v.str.len;
  while (p < pend) {
    char *prevp = p;
    Rune prevrune = rune;
    p += chartorune(&rune, p);
    if (!_isspase(rune)) {
      end = NULL;
      if (_isspase(prevrune))
        if (NULL == begin) begin = prevp;
    } else if (!_isspase(prevrune))
      end = prevp;
  }
  if (NULL == end) end = cfa->this_obj->v.str.buf + cfa->this_obj->v.str.len;
  TRY(v7_make_and_push(v7, V7_TYPE_STR));
  v7_init_str(v7_top_val(v7), begin, end - begin, 1);
  return V7_OK;
#undef v7
}

V7_PRIVATE void Str_length(struct v7_val *this_obj, struct v7_val *arg,
                           struct v7_val *result) {
  if (NULL == result || arg) return;
  v7_init_num(result, utfnlen(this_obj->v.str.buf, this_obj->v.str.len));
}
#endif

V7_PRIVATE void init_string(struct v7 *v7) {
  val_t str = v7_create_cfunction(String_ctor);
  v7_set_property(v7, v7->global_object, "String", 6, 0, str);

  set_cfunc_prop(v7, v7->string_prototype, "charCodeAt", Str_charCodeAt);
  set_cfunc_prop(v7, v7->string_prototype, "fromCharCode", Str_fromCharCode);

#if 0
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "valueOf", Str_valueOf);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "charAt", Str_charAt);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "concat", Str_concat);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "indexOf", Str_indexOf);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "lastIndexOf", Str_lastIndexOf);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "localeCompare", Str_localeCompare);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "match", Str_match);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "replace", Str_replace);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "search", Str_search);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "slice", Str_slice);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "split", Str_split);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "substring", Str_substring);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "substr", Str_substr);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "toLowerCase", Str_toLowerCase);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "toLocaleLowerCase",
             Str_toLocaleLowerCase);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "toUpperCase", Str_toUpperCase);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "toLocaleUpperCase",
             Str_toLocaleUpperCase);
  SET_METHOD(s_prototypes[V7_CLASS_STRING], "trim", Str_trim);

  SET_PROP_FUNC(s_prototypes[V7_CLASS_STRING], "length", Str_length);

  SET_RO_PROP_V(s_global, "String", s_constructors[V7_CLASS_STRING]);
#endif
}
