#include "fci_arm.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

enum {
  WLC_OPTIONAL,
  WLC_REPEATED,
  WLC_PACKED,
  WLC_BOOL,
  WLC_U8,
  WLC_U16,
  WLC_U32,
  WLC_U64,
  WLC_I8,
  WLC_I16,
  WLC_I32,
  WLC_I64,
  WLC_F32,
  WLC_F64,
  WLC_FLOAT32,
  WLC_FLOAT64,
  WLC_BYTES,
  WLC_STRING,
  WLC_ENUM,
  WLC_MESSAGE
};
typedef struct wlc_desc wlc_desc_t;
typedef struct {
  uint16_t number;
  uint8_t card, kind, required, wire, key_size;
  size_t value, has, count, capacity, element, packed_count;
  uint16_t max_length;
  uint8_t key[3];
  int64_t signed_default;
  uint64_t unsigned_default;
  const char *string_default;
  const wlc_desc_t *nested;
} wlc_field_t;
enum { WLC_LOOKUP_LINEAR, WLC_LOOKUP_DENSE, WLC_LOOKUP_BINARY };
struct wlc_desc { const wlc_field_t *fields; size_t count; uint8_t lookup; };

static inline wl_codec_status_t wlc_add(size_t *a, size_t b) {
  if (b > SIZE_MAX - *a) return WL_CODEC_ERR_OVERFLOW;
  *a += b;
  return WL_CODEC_OK;
}
static inline size_t wlc_vsize(uint64_t v) {
  size_t n = 1U;
  while (v >= 128U) { v >>= 7U; ++n; }
  return n;
}
static inline void wlc_putv(uint8_t **p, uint64_t v) {
  while (v >= 128U) { *(*p)++ = (uint8_t)(v | 128U); v >>= 7U; }
  *(*p)++ = (uint8_t)v;
}
static wl_codec_status_t wlc_getv(const uint8_t *in, size_t length, size_t *at,
                                  uint64_t *out) {
  size_t start = *at, n = 0U;
  uint64_t v = 0U;
  while (*at < length && n < 10U) {
    uint8_t b = in[(*at)++];
    if (n == 9U && b > 1U) return WL_CODEC_ERR_OVERFLOW;
    v |= (uint64_t)(b & 127U) << (7U * n++);
    if ((b & 128U) == 0U) {
      if (wlc_vsize(v) != *at - start) return WL_CODEC_ERR_MALFORMED;
      *out = v;
      return WL_CODEC_OK;
    }
  }
  return *at == length ? WL_CODEC_ERR_MALFORMED : WL_CODEC_ERR_OVERFLOW;
}
static bool wlc_utf8(const uint8_t *s, size_t n) {
  size_t i = 0U;
  while (i < n) {
    uint8_t a = s[i++];
    if (a < 0x80U) continue;
    size_t need;
    uint32_t v;
    if (a >= 0xC2U && a <= 0xDFU) { need = 1U; v = a & 0x1FU; }
    else if (a >= 0xE0U && a <= 0xEFU) { need = 2U; v = a & 0x0FU; }
    else if (a >= 0xF0U && a <= 0xF4U) { need = 3U; v = a & 0x07U; }
    else return false;
    if (need > n - i) return false;
    while (need-- != 0U) {
      uint8_t b = s[i++];
      if ((b & 0xC0U) != 0x80U) return false;
      v = (v << 6U) | (b & 0x3FU);
    }
    if ((a == 0xE0U && v < 0x800U) ||
        (a == 0xEDU && v >= 0xD800U) ||
        (a == 0xF0U && v < 0x10000U) ||
        (a == 0xF4U && v > 0x10FFFFU)) return false;
  }
  return true;
}
static inline uint8_t wlc_wire(const wlc_field_t *f) {
  return f->wire;
}
static inline uint64_t wlc_z32(int32_t v) {
  return ((uint32_t)v << 1U) ^ (uint32_t)-(uint32_t)(v < 0);
}
static inline uint64_t wlc_z64(int64_t v) {
  return ((uint64_t)v << 1U) ^ (uint64_t)-(uint64_t)(v < 0);
}
static inline int32_t wlc_uz32(uint32_t v) {
  return (int32_t)((v >> 1U) ^ (uint32_t)-(v & 1U));
}
static inline int64_t wlc_uz64(uint64_t v) {
  return (int64_t)((v >> 1U) ^ (uint64_t)-(v & 1U));
}
static wl_codec_status_t wlc_measure(const wlc_desc_t *, const void *, size_t *);
static wl_codec_status_t wlc_measure_impl(const wlc_desc_t *, const void *, size_t *, bool);
static void wlc_clear(const wlc_desc_t *, void *);
static wl_codec_status_t wlc_decode(const wlc_desc_t *, const uint8_t *, size_t,
                                    void *);
static wl_codec_status_t wlc_emit_fields(const wlc_desc_t *, const void *,
                                         uint8_t **);

static wl_codec_status_t wlc_packed_bytes(const wlc_field_t *f, size_t *bytes) {
  if (f->packed_count != 0U && f->element > SIZE_MAX / f->packed_count)
    return WL_CODEC_ERR_OVERFLOW;
  *bytes = f->element * f->packed_count;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_body(const wlc_field_t *f, const void *p,
                                  size_t *n, bool validated) {
  *n = 0U;
  switch (f->kind) {
    case WLC_BOOL: {
      bool v = *(const bool *)p;
      if (v != false && v != true) return WL_CODEC_ERR_INVALID_VALUE;
      *n = 1U;
      return WL_CODEC_OK;
    }
    case WLC_U8: *n = wlc_vsize(*(const uint8_t *)p); return WL_CODEC_OK;
    case WLC_U16: *n = wlc_vsize(*(const uint16_t *)p); return WL_CODEC_OK;
    case WLC_U32: *n = wlc_vsize(*(const uint32_t *)p); return WL_CODEC_OK;
    case WLC_U64: *n = wlc_vsize(*(const uint64_t *)p); return WL_CODEC_OK;
    case WLC_I8: *n = wlc_vsize(wlc_z32(*(const int8_t *)p)); return WL_CODEC_OK;
    case WLC_I16: *n = wlc_vsize(wlc_z32(*(const int16_t *)p)); return WL_CODEC_OK;
    case WLC_I32:
    case WLC_ENUM: *n = wlc_vsize(wlc_z32(*(const int32_t *)p)); return WL_CODEC_OK;
    case WLC_I64: *n = wlc_vsize(wlc_z64(*(const int64_t *)p)); return WL_CODEC_OK;
    case WLC_F32:
    case WLC_FLOAT32: *n = 4U; return WL_CODEC_OK;
    case WLC_F64:
    case WLC_FLOAT64: *n = 8U; return WL_CODEC_OK;
    case WLC_BYTES: {
      const wl_codec_bytes_t *v = p;
      if (f->max_length != 0U && v->length > (size_t)f->max_length)
        return WL_CODEC_ERR_INVALID_VALUE;
      if (v->length != 0U && v->data == NULL) return WL_CODEC_ERR_INVALID_VALUE;
      if (wlc_add(n, wlc_vsize(v->length)) != WL_CODEC_OK)
        return WL_CODEC_ERR_OVERFLOW;
      return wlc_add(n, v->length);
    }
    case WLC_STRING: {
      const wl_codec_string_t *v = p;
      if (f->max_length != 0U && v->length > (size_t)f->max_length)
        return WL_CODEC_ERR_INVALID_VALUE;
      if (v->length != 0U && v->data == NULL) return WL_CODEC_ERR_INVALID_VALUE;
      if (!validated && !wlc_utf8((const uint8_t *)v->data, v->length)) return WL_CODEC_ERR_UTF8;
      if (wlc_add(n, wlc_vsize(v->length)) != WL_CODEC_OK)
        return WL_CODEC_ERR_OVERFLOW;
      return wlc_add(n, v->length);
    }
    case WLC_MESSAGE: {
      size_t child;
      wl_codec_status_t s = wlc_measure_impl(f->nested, p, &child, validated);
      if (s != WL_CODEC_OK) return s;
      *n = wlc_vsize(child);
      return wlc_add(n, child);
    }
    default: return WL_CODEC_ERR_INVALID_VALUE;
  }
}
static wl_codec_status_t wlc_measure_impl(const wlc_desc_t *d, const void *value,
                                     size_t *out, bool validated) {
  size_t n = 0U;
  if (d == NULL || value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  for (size_t i = 0U; i < d->count; ++i) {
    const wlc_field_t *f = &d->fields[i];
    const uint8_t *base = value;
    if (f->card == WLC_PACKED) {
      size_t bytes;
      wl_codec_status_t s;
      if (!*(const bool *)(base + f->has)) {
        if (f->required != 0U) return WL_CODEC_ERR_MISSING_REQUIRED_FIELD;
        continue;
      }
      if ((s = wlc_packed_bytes(f, &bytes)) != WL_CODEC_OK) return s;
      if ((s = wlc_add(&n, f->key_size)) != WL_CODEC_OK ||
          (s = wlc_add(&n, wlc_vsize(bytes))) != WL_CODEC_OK ||
          (s = wlc_add(&n, bytes)) != WL_CODEC_OK) return s;
      continue;
    }
    size_t count = 1U;
    if (f->card == WLC_OPTIONAL) {
      if (!*(const bool *)(base + f->has)) {
        if (f->required != 0U) return WL_CODEC_ERR_MISSING_REQUIRED_FIELD;
        continue;
      }
    } else {
      count = *(const size_t *)(base + f->count);
      if ((count != 0U && *(void *const *)(base + f->value) == NULL) ||
          count > *(const size_t *)(base + f->capacity))
        return WL_CODEC_ERR_INVALID_VALUE;
    }
    for (size_t j = 0U; j < count; ++j) {
      size_t body;
      const void *p = f->card == WLC_REPEATED
                          ? *(const uint8_t *const *)(base + f->value) + j * f->element
                          : base + f->value;
      wl_codec_status_t s = wlc_body(f, p, &body, validated);
      if (s != WL_CODEC_OK) return s;
      if ((s = wlc_add(&n, f->key_size)) != WL_CODEC_OK ||
          (s = wlc_add(&n, body)) != WL_CODEC_OK) return s;
    }
  }
  *out = n;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_measure(const wlc_desc_t *d, const void *value, size_t *out) {
  return wlc_measure_impl(d, value, out, false);
}
static wl_codec_status_t wlc_measure_validated(const wlc_desc_t *d, const void *value, size_t *out) {
  return wlc_measure_impl(d, value, out, true);
}
static void wlc_clear(const wlc_desc_t *d, void *value) {
  uint8_t *base = value;
  for (size_t i = 0U; i < d->count; ++i) {
    const wlc_field_t *f = &d->fields[i];
    void *p = base + f->value;
    if (f->card == WLC_REPEATED) {
      *(size_t *)(base + f->count) = 0U;
      continue;
    }
    *(bool *)(base + f->has) = false;
    if (f->card == WLC_PACKED) {
      memset(p, 0, f->element * f->packed_count);
      continue;
    }
    if (f->kind == WLC_MESSAGE) { wlc_clear(f->nested, p); continue; }
    if (f->kind == WLC_STRING) {
      *(wl_codec_string_t *)p =
          (wl_codec_string_t){f->string_default, (size_t)f->unsigned_default};
      continue;
    }
    if (f->kind == WLC_BYTES) {
      *(wl_codec_bytes_t *)p = (wl_codec_bytes_t){NULL, 0U};
      continue;
    }
    if (f->kind == WLC_FLOAT32 || f->kind == WLC_FLOAT64) {
      memset(p, 0, f->element);
      continue;
    }
    if (f->kind == WLC_BOOL) *(bool *)p = f->unsigned_default != 0U;
    else if (f->kind == WLC_I8) *(int8_t *)p = (int8_t)f->signed_default;
    else if (f->kind == WLC_I16) *(int16_t *)p = (int16_t)f->signed_default;
    else if (f->kind == WLC_I32 || f->kind == WLC_ENUM)
      *(int32_t *)p = (int32_t)f->signed_default;
    else if (f->kind == WLC_I64) *(int64_t *)p = f->signed_default;
    else if (f->kind == WLC_U8) *(uint8_t *)p = (uint8_t)f->unsigned_default;
    else if (f->kind == WLC_U16) *(uint16_t *)p = (uint16_t)f->unsigned_default;
    else if (f->kind == WLC_U32 || f->kind == WLC_F32)
      *(uint32_t *)p = (uint32_t)f->unsigned_default;
    else if (f->kind == WLC_U64 || f->kind == WLC_F64)
      *(uint64_t *)p = f->unsigned_default;
  }
}
static inline void wlc_put32(uint8_t **p, uint32_t v) {
  *(*p)++ = (uint8_t)(v >> 24U);
  *(*p)++ = (uint8_t)(v >> 16U);
  *(*p)++ = (uint8_t)(v >> 8U);
  *(*p)++ = (uint8_t)v;
}
static inline void wlc_put64(uint8_t **p, uint64_t v) {
  wlc_put32(p, (uint32_t)(v >> 32U));
  wlc_put32(p, (uint32_t)v);
}
static inline void wlc_copy_span(uint8_t **out, const void *data, size_t length) {
  if (length != 0U) { memcpy(*out, data, length); *out += length; }
}
static inline void wlc_put_key(uint8_t **out, const wlc_field_t *f) {
  *(*out)++ = f->key[0];
  if (f->key_size > 1U) {
    *(*out)++ = f->key[1];
    if (f->key_size > 2U) *(*out)++ = f->key[2];
  }
}

static wl_codec_status_t wlc_emit_fixed(uint8_t kind, const void *value,
                                        uint8_t **out) {
  if (kind == WLC_F32) wlc_put32(out, *(const uint32_t *)value);
  else if (kind == WLC_F64) wlc_put64(out, *(const uint64_t *)value);
  else if (kind == WLC_FLOAT32) {
    uint32_t bits32;
    memcpy(&bits32, value, sizeof(bits32));
    wlc_put32(out, bits32);
  } else if (kind == WLC_FLOAT64) {
    uint64_t bits;
    memcpy(&bits, value, sizeof(bits));
    wlc_put64(out, bits);
  } else return WL_CODEC_ERR_INVALID_VALUE;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_emit_value(const wlc_field_t *f, const void *p,
                                        uint8_t **out) {
  switch (f->kind) {
    case WLC_BOOL: wlc_putv(out, *(const bool *)p); return WL_CODEC_OK;
    case WLC_U8: wlc_putv(out, *(const uint8_t *)p); return WL_CODEC_OK;
    case WLC_U16: wlc_putv(out, *(const uint16_t *)p); return WL_CODEC_OK;
    case WLC_U32: wlc_putv(out, *(const uint32_t *)p); return WL_CODEC_OK;
    case WLC_U64: wlc_putv(out, *(const uint64_t *)p); return WL_CODEC_OK;
    case WLC_I8: wlc_putv(out, wlc_z32(*(const int8_t *)p)); return WL_CODEC_OK;
    case WLC_I16: wlc_putv(out, wlc_z32(*(const int16_t *)p)); return WL_CODEC_OK;
    case WLC_I32:
    case WLC_ENUM: wlc_putv(out, wlc_z32(*(const int32_t *)p)); return WL_CODEC_OK;
    case WLC_I64: wlc_putv(out, wlc_z64(*(const int64_t *)p)); return WL_CODEC_OK;
    case WLC_F32:
    case WLC_F64:
    case WLC_FLOAT32:
    case WLC_FLOAT64: return wlc_emit_fixed(f->kind, p, out);
    case WLC_BYTES: {
      const wl_codec_bytes_t *v = p;
      wlc_putv(out, v->length);
      wlc_copy_span(out, v->data, v->length);
      return WL_CODEC_OK;
    }
    case WLC_STRING: {
      const wl_codec_string_t *v = p;
      wlc_putv(out, v->length);
      wlc_copy_span(out, v->data, v->length);
      return WL_CODEC_OK;
    }
    case WLC_MESSAGE: {
      size_t child;
      wl_codec_status_t s = wlc_measure(f->nested, p, &child);
      if (s != WL_CODEC_OK) return s;
      wlc_putv(out, child);
      return wlc_emit_fields(f->nested, p, out);
    }
    default: return WL_CODEC_ERR_INVALID_VALUE;
  }
}
static wl_codec_status_t wlc_emit_packed(const wlc_field_t *f, const void *p,
                                         uint8_t **out) {
  size_t bytes;
  wl_codec_status_t s = wlc_packed_bytes(f, &bytes);
  if (s != WL_CODEC_OK) return s;
  wlc_putv(out, bytes);
  if (f->kind == WLC_F32 || f->kind == WLC_FLOAT32) {
    for (size_t j = 0U; j < f->packed_count; ++j) {
      uint32_t bits;
      memcpy(&bits, (const uint8_t *)p + j * f->element, sizeof(bits));
      wlc_put32(out, bits);
    }
  } else if (f->kind == WLC_F64 || f->kind == WLC_FLOAT64) {
    for (size_t j = 0U; j < f->packed_count; ++j) {
      uint64_t bits;
      memcpy(&bits, (const uint8_t *)p + j * f->element, sizeof(bits));
      wlc_put64(out, bits);
    }
  } else {
    return WL_CODEC_ERR_INVALID_VALUE;
  }
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_emit_fields(const wlc_desc_t *d, const void *value,
                                         uint8_t **out) {
  for (size_t i = 0U; i < d->count; ++i) {
    const wlc_field_t *f = &d->fields[i];
    const uint8_t *base = value;
    if (f->card == WLC_PACKED) {
      wl_codec_status_t s;
      if (!*(const bool *)(base + f->has)) continue;
      wlc_put_key(out, f);
      if ((s = wlc_emit_packed(f, base + f->value, out)) != WL_CODEC_OK) return s;
      continue;
    }
    size_t count = f->card == WLC_OPTIONAL
                       ? (*(const bool *)(base + f->has) ? 1U : 0U)
                       : *(const size_t *)(base + f->count);
    for (size_t j = 0U; j < count; ++j) {
      const void *p = f->card == WLC_REPEATED
                          ? *(const uint8_t *const *)(base + f->value) + j * f->element
                          : base + f->value;
      wl_codec_status_t s;
      wlc_put_key(out, f);
      if ((s = wlc_emit_value(f, p, out)) != WL_CODEC_OK) return s;
    }
  }
  return WL_CODEC_OK;
}

static inline wl_codec_status_t wlc_encode(const wlc_desc_t *d, const void *value,
                                    uint8_t *out, size_t cap, size_t *length) {
  size_t n;
  wl_codec_status_t s = wlc_measure(d, value, &n);
  if (s != WL_CODEC_OK || length == NULL || (n != 0U && out == NULL))
    return s == WL_CODEC_OK ? WL_CODEC_ERR_INVALID_VALUE : s;
  if (cap < n) return WL_CODEC_ERR_CAPACITY;
  uint8_t *p = out;
  if ((s = wlc_emit_fields(d, value, &p)) != WL_CODEC_OK) return s;
  *length = n;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_skip(uint8_t wire, const uint8_t *in, size_t n,
                                  size_t *at) {
  uint64_t length;
  wl_codec_status_t s;
  if (wire == 0U) return wlc_getv(in, n, at, &length);
  if (wire == 1U) {
    if (n - *at < 8U) return WL_CODEC_ERR_MALFORMED;
    *at += 8U;
    return WL_CODEC_OK;
  }
  if (wire == 5U) {
    if (n - *at < 4U) return WL_CODEC_ERR_MALFORMED;
    *at += 4U;
    return WL_CODEC_OK;
  }
  if (wire != 2U) return WL_CODEC_ERR_MALFORMED;
  if ((s = wlc_getv(in, n, at, &length)) != WL_CODEC_OK) return s;
  if (length > n - *at) return WL_CODEC_ERR_MALFORMED;
  *at += (size_t)length;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_read_fixed(uint8_t kind, const uint8_t *in,
                                        size_t n, size_t *at, void *out) {
  size_t bytes = (kind == WLC_F32 || kind == WLC_FLOAT32) ? 4U : 8U;
  if (n - *at < bytes) return WL_CODEC_ERR_MALFORMED;
  uint64_t bits = 0U;
  for (size_t i = 0U; i < bytes; ++i) bits = (bits << 8U) | in[(*at)++];
  if (kind == WLC_F32) *(uint32_t *)out = (uint32_t)bits;
  else if (kind == WLC_F64) *(uint64_t *)out = bits;
  else if (kind == WLC_FLOAT32) {
    uint32_t bits32 = (uint32_t)bits;
    memcpy(out, &bits32, sizeof(bits32));
  } else if (kind == WLC_FLOAT64) memcpy(out, &bits, sizeof(bits));
  else return WL_CODEC_ERR_INVALID_VALUE;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_read_value(const wlc_field_t *f, const uint8_t *in,
                                        size_t n, size_t *at, void *out) {
  uint64_t v;
  wl_codec_status_t s;
  if (f->kind == WLC_F32 || f->kind == WLC_F64 ||
      f->kind == WLC_FLOAT32 || f->kind == WLC_FLOAT64)
    return wlc_read_fixed(f->kind, in, n, at, out);
  if (f->kind == WLC_BYTES || f->kind == WLC_STRING || f->kind == WLC_MESSAGE) {
    if ((s = wlc_getv(in, n, at, &v)) != WL_CODEC_OK) return s;
    if ((f->kind == WLC_BYTES || f->kind == WLC_STRING) &&
        f->max_length != 0U && v > (uint64_t)f->max_length)
      return WL_CODEC_ERR_INVALID_VALUE;
    if (v > n - *at) return WL_CODEC_ERR_MALFORMED;
    size_t bytes = (size_t)v;
    if (f->kind == WLC_BYTES)
      *(wl_codec_bytes_t *)out = (wl_codec_bytes_t){in + *at, bytes};
    else if (f->kind == WLC_STRING) {
      if (!wlc_utf8(in + *at, bytes)) return WL_CODEC_ERR_UTF8;
      *(wl_codec_string_t *)out =
          (wl_codec_string_t){(const char *)(in + *at), bytes};
    } else {
      s = wlc_decode(f->nested, in + *at, bytes, out);
      if (s != WL_CODEC_OK) return s;
    }
    *at += bytes;
    return WL_CODEC_OK;
  }
  if ((s = wlc_getv(in, n, at, &v)) != WL_CODEC_OK) return s;
  if (f->kind == WLC_BOOL) {
    if (v > 1U) return WL_CODEC_ERR_INVALID_VALUE;
    *(bool *)out = v != 0U;
  } else if (f->kind == WLC_U8) {
    if (v > UINT8_MAX) return WL_CODEC_ERR_OVERFLOW;
    *(uint8_t *)out = (uint8_t)v;
  } else if (f->kind == WLC_U16) {
    if (v > UINT16_MAX) return WL_CODEC_ERR_OVERFLOW;
    *(uint16_t *)out = (uint16_t)v;
  } else if (f->kind == WLC_U32) {
    if (v > UINT32_MAX) return WL_CODEC_ERR_OVERFLOW;
    *(uint32_t *)out = (uint32_t)v;
  } else if (f->kind == WLC_U64) *(uint64_t *)out = v;
  else if (f->kind == WLC_I8) {
    if (v > UINT8_MAX) return WL_CODEC_ERR_OVERFLOW;
    *(int8_t *)out = (int8_t)wlc_uz32((uint32_t)v);
  } else if (f->kind == WLC_I16) {
    if (v > UINT16_MAX) return WL_CODEC_ERR_OVERFLOW;
    *(int16_t *)out = (int16_t)wlc_uz32((uint32_t)v);
  } else if (f->kind == WLC_I32 || f->kind == WLC_ENUM) {
    if (v > UINT32_MAX) return WL_CODEC_ERR_OVERFLOW;
    *(int32_t *)out = wlc_uz32((uint32_t)v);
  } else if (f->kind == WLC_I64) *(int64_t *)out = wlc_uz64(v);
  else return WL_CODEC_ERR_INVALID_VALUE;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_read_packed(const wlc_field_t *f,
                                         const uint8_t *in, size_t n,
                                         size_t *at, void *out) {
  uint64_t encoded_bytes;
  size_t expected_bytes;
  wl_codec_status_t s;
  if ((s = wlc_getv(in, n, at, &encoded_bytes)) != WL_CODEC_OK) return s;
  if ((s = wlc_packed_bytes(f, &expected_bytes)) != WL_CODEC_OK) return s;
  if (encoded_bytes != expected_bytes || expected_bytes > n - *at)
    return WL_CODEC_ERR_MALFORMED;
  if (f->kind == WLC_F32 || f->kind == WLC_FLOAT32) {
    for (size_t j = 0U; j < f->packed_count; ++j) {
      uint32_t bits = ((uint32_t)in[*at] << 24U) |
                      ((uint32_t)in[*at + 1U] << 16U) |
                      ((uint32_t)in[*at + 2U] << 8U) |
                      (uint32_t)in[*at + 3U];
      memcpy((uint8_t *)out + j * f->element, &bits, sizeof(bits));
      *at += 4U;
    }
  } else if (f->kind == WLC_F64 || f->kind == WLC_FLOAT64) {
    for (size_t j = 0U; j < f->packed_count; ++j) {
      uint64_t bits = 0U;
      for (size_t i = 0U; i < 8U; ++i) bits = (bits << 8U) | in[(*at)++];
      memcpy((uint8_t *)out + j * f->element, &bits, sizeof(bits));
    }
  } else {
    return WL_CODEC_ERR_INVALID_VALUE;
  }
  return WL_CODEC_OK;
}
static const wlc_field_t *wlc_find_field(const wlc_desc_t *d, uint16_t number) {
  if (d->lookup == WLC_LOOKUP_DENSE) {
    size_t index = (size_t)number - (size_t)d->fields[0].number;
    return index < d->count ? &d->fields[index] : NULL;
  }
  if (d->lookup == WLC_LOOKUP_BINARY) {
    size_t lo = 0U, hi = d->count;
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2U;
      uint16_t candidate = d->fields[mid].number;
      if (candidate == number) return &d->fields[mid];
      if (candidate < number) lo = mid + 1U;
      else hi = mid;
    }
    return NULL;
  }
  for (size_t i = 0U; i < d->count; ++i)
    if (d->fields[i].number == number) return &d->fields[i];
  return NULL;
}
static wl_codec_status_t wlc_decode(const wlc_desc_t *d, const uint8_t *in,
                                    size_t n, void *out) {
  if (d == NULL || out == NULL || (n != 0U && in == NULL))
    return WL_CODEC_ERR_INVALID_VALUE;
  wlc_clear(d, out);
  // A hint only: missing, unknown or reordered fields still use exact lookup.
  // Each recursive decode owns its cursor; empty schemas avoid NULL arithmetic.
  const wlc_field_t *next = d->fields;
  const wlc_field_t *end = d->count == 0U ? next : next + d->count;
  for (size_t at = 0U; at < n;) {
    uint64_t key;
    wl_codec_status_t s = wlc_getv(in, n, &at, &key);
    if (s != WL_CODEC_OK) return s;
    uint64_t raw_number = key >> 3U;
    uint8_t wire = (uint8_t)(key & 7U);
    if (raw_number == 0U || raw_number > 65535U ||
        (wire != 0U && wire != 1U && wire != 2U && wire != 5U))
      return WL_CODEC_ERR_MALFORMED;
    uint16_t number = (uint16_t)raw_number;
    const wlc_field_t *f = next != end && next->number == number
                               ? next : wlc_find_field(d, number);
    if (f == NULL) {
      if ((s = wlc_skip(wire, in, n, &at)) != WL_CODEC_OK) return s;
      continue;
    }
    if (wire != wlc_wire(f)) return WL_CODEC_ERR_WIRE_TYPE;
    next = f->card == WLC_REPEATED ? f : f + 1;
    uint8_t *base = out;
    if (f->card == WLC_PACKED) {
      if (*(bool *)(base + f->has)) return WL_CODEC_ERR_DUPLICATE_FIELD;
      if ((s = wlc_read_packed(f, in, n, &at, base + f->value)) != WL_CODEC_OK)
        return s;
      *(bool *)(base + f->has) = true;
      continue;
    }
    void *p;
    if (f->card == WLC_OPTIONAL) {
      if (*(bool *)(base + f->has)) return WL_CODEC_ERR_DUPLICATE_FIELD;
      p = base + f->value;
    } else {
      size_t count = *(size_t *)(base + f->count);
      if (count >= *(size_t *)(base + f->capacity)) return WL_CODEC_ERR_CAPACITY;
      void *storage = *(void **)(base + f->value);
      if (storage == NULL) return WL_CODEC_ERR_INVALID_VALUE;
      p = (uint8_t *)storage + count * f->element;
    }
    if ((s = wlc_read_value(f, in, n, &at, p)) != WL_CODEC_OK) return s;
    if (f->card == WLC_OPTIONAL) *(bool *)(base + f->has) = true;
    else ++*(size_t *)(base + f->count);
  }
  for (size_t i = 0U; i < d->count; ++i) {
    const wlc_field_t *f = &d->fields[i];
    if (f->required != 0U && !*(const bool *)((uint8_t *)out + f->has))
      return WL_CODEC_ERR_MISSING_REQUIRED_FIELD;
  }
  return WL_CODEC_OK;
}
/* Generator-private canonical sink. Only successful decode results enter here.
 * The shared emitter orders known fields, drops unknowns, and preserves presence.
 * Scalar bytes use the ordinary endian/varint writers; no raw-frame hashing. */
typedef struct {
  uint64_t hash;
  size_t length;
  wl_codec_status_t status;
} wlc_hash_state_t;
static wl_codec_status_t wlc_hash_emit_fields(const wlc_desc_t *, const void *, wlc_hash_state_t *);
static inline void wlc_hash_copy_span(wlc_hash_state_t *out, const void *data, size_t length) {
  const uint8_t *bytes = data;
  if (out->status != WL_CODEC_OK) return;
  out->status = wlc_add(&out->length, length);
  if (out->status != WL_CODEC_OK) return;
  uint64_t hash = out->hash;
  for (size_t i = 0U; i < length; ++i)
    hash = (hash ^ bytes[i]) * UINT64_C(0x100000001b3);
  out->hash = hash;
}
static inline void wlc_hash_putv(wlc_hash_state_t *out, uint64_t value) {
  uint8_t bytes[10], *cursor = bytes;
  wlc_putv(&cursor, value);
  wlc_hash_copy_span(out, bytes, (size_t)(cursor - bytes));
}
static inline void wlc_hash_put_key(wlc_hash_state_t *out, const wlc_field_t *f) {
  wlc_hash_copy_span(out, f->key, f->key_size);
}
static inline void wlc_hash_put32(wlc_hash_state_t *out, uint32_t value) {
  uint8_t bytes[4], *cursor = bytes;
  wlc_put32(&cursor, value);
  wlc_hash_copy_span(out, bytes, sizeof(bytes));
}
static inline void wlc_hash_put64(wlc_hash_state_t *out, uint64_t value) {
  uint8_t bytes[8], *cursor = bytes;
  wlc_put64(&cursor, value);
  wlc_hash_copy_span(out, bytes, sizeof(bytes));
}

static wl_codec_status_t wlc_hash_emit_fixed(uint8_t kind, const void *value,
                                        wlc_hash_state_t *out) {
  if (kind == WLC_F32) wlc_hash_put32(out, *(const uint32_t *)value);
  else if (kind == WLC_F64) wlc_hash_put64(out, *(const uint64_t *)value);
  else if (kind == WLC_FLOAT32) {
    uint32_t bits32;
    memcpy(&bits32, value, sizeof(bits32));
    wlc_hash_put32(out, bits32);
  } else if (kind == WLC_FLOAT64) {
    uint64_t bits;
    memcpy(&bits, value, sizeof(bits));
    wlc_hash_put64(out, bits);
  } else return WL_CODEC_ERR_INVALID_VALUE;
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_hash_emit_value(const wlc_field_t *f, const void *p,
                                        wlc_hash_state_t *out) {
  switch (f->kind) {
    case WLC_BOOL: wlc_hash_putv(out, *(const bool *)p); return WL_CODEC_OK;
    case WLC_U8: wlc_hash_putv(out, *(const uint8_t *)p); return WL_CODEC_OK;
    case WLC_U16: wlc_hash_putv(out, *(const uint16_t *)p); return WL_CODEC_OK;
    case WLC_U32: wlc_hash_putv(out, *(const uint32_t *)p); return WL_CODEC_OK;
    case WLC_U64: wlc_hash_putv(out, *(const uint64_t *)p); return WL_CODEC_OK;
    case WLC_I8: wlc_hash_putv(out, wlc_z32(*(const int8_t *)p)); return WL_CODEC_OK;
    case WLC_I16: wlc_hash_putv(out, wlc_z32(*(const int16_t *)p)); return WL_CODEC_OK;
    case WLC_I32:
    case WLC_ENUM: wlc_hash_putv(out, wlc_z32(*(const int32_t *)p)); return WL_CODEC_OK;
    case WLC_I64: wlc_hash_putv(out, wlc_z64(*(const int64_t *)p)); return WL_CODEC_OK;
    case WLC_F32:
    case WLC_F64:
    case WLC_FLOAT32:
    case WLC_FLOAT64: return wlc_hash_emit_fixed(f->kind, p, out);
    case WLC_BYTES: {
      const wl_codec_bytes_t *v = p;
      wlc_hash_putv(out, v->length);
      wlc_hash_copy_span(out, v->data, v->length);
      return WL_CODEC_OK;
    }
    case WLC_STRING: {
      const wl_codec_string_t *v = p;
      wlc_hash_putv(out, v->length);
      wlc_hash_copy_span(out, v->data, v->length);
      return WL_CODEC_OK;
    }
    case WLC_MESSAGE: {
      size_t child;
      wl_codec_status_t s = wlc_measure_validated(f->nested, p, &child);
      if (s != WL_CODEC_OK) return s;
      wlc_hash_putv(out, child);
      return wlc_hash_emit_fields(f->nested, p, out);
    }
    default: return WL_CODEC_ERR_INVALID_VALUE;
  }
}
static wl_codec_status_t wlc_hash_emit_packed(const wlc_field_t *f, const void *p,
                                         wlc_hash_state_t *out) {
  size_t bytes;
  wl_codec_status_t s = wlc_packed_bytes(f, &bytes);
  if (s != WL_CODEC_OK) return s;
  wlc_hash_putv(out, bytes);
  if (f->kind == WLC_F32 || f->kind == WLC_FLOAT32) {
    for (size_t j = 0U; j < f->packed_count; ++j) {
      uint32_t bits;
      memcpy(&bits, (const uint8_t *)p + j * f->element, sizeof(bits));
      wlc_hash_put32(out, bits);
    }
  } else if (f->kind == WLC_F64 || f->kind == WLC_FLOAT64) {
    for (size_t j = 0U; j < f->packed_count; ++j) {
      uint64_t bits;
      memcpy(&bits, (const uint8_t *)p + j * f->element, sizeof(bits));
      wlc_hash_put64(out, bits);
    }
  } else {
    return WL_CODEC_ERR_INVALID_VALUE;
  }
  return WL_CODEC_OK;
}
static wl_codec_status_t wlc_hash_emit_fields(const wlc_desc_t *d, const void *value,
                                         wlc_hash_state_t *out) {
  for (size_t i = 0U; i < d->count; ++i) {
    const wlc_field_t *f = &d->fields[i];
    const uint8_t *base = value;
    if (f->card == WLC_PACKED) {
      wl_codec_status_t s;
      if (!*(const bool *)(base + f->has)) continue;
      wlc_hash_put_key(out, f);
      if ((s = wlc_hash_emit_packed(f, base + f->value, out)) != WL_CODEC_OK) return s;
      continue;
    }
    size_t count = f->card == WLC_OPTIONAL
                       ? (*(const bool *)(base + f->has) ? 1U : 0U)
                       : *(const size_t *)(base + f->count);
    for (size_t j = 0U; j < count; ++j) {
      const void *p = f->card == WLC_REPEATED
                          ? *(const uint8_t *const *)(base + f->value) + j * f->element
                          : base + f->value;
      wl_codec_status_t s;
      wlc_hash_put_key(out, f);
      if ((s = wlc_hash_emit_value(f, p, out)) != WL_CODEC_OK) return s;
    }
  }
  return WL_CODEC_OK;
}
static const wlc_desc_t semantic_version_desc;
static const wlc_desc_t device_info_desc;
static const wlc_desc_t device_settings_desc;
static const wlc_desc_t arm_status_desc;
static const wlc_desc_t motor_feedback_desc;
static const wlc_desc_t arm_diagnostics_desc;
static const wlc_desc_t set_zero_request_desc;
static const wlc_desc_t set_zero_response_desc;
static const wlc_desc_t clear_error_request_desc;
static const wlc_desc_t clear_error_response_desc;
static const wlc_desc_t home_request_desc;
static const wlc_desc_t home_response_desc;
static const wlc_desc_t clear_faults_request_desc;
static const wlc_desc_t clear_faults_response_desc;
static const wlc_desc_t acquire_control_lease_request_desc;
static const wlc_desc_t acquire_control_lease_response_desc;
static const wlc_desc_t release_control_lease_request_desc;
static const wlc_desc_t release_control_lease_response_desc;
static const wlc_desc_t get_motor_feedback_request_desc;
static const wlc_desc_t get_motor_feedback_response_desc;
static const wlc_desc_t get_device_info_request_desc;
static const wlc_desc_t get_device_info_response_desc;
static const wlc_desc_t set_device_info_request_desc;
static const wlc_desc_t set_device_info_response_desc;
static const wlc_desc_t set_arm_control_mode_request_desc;
static const wlc_desc_t set_arm_control_mode_response_desc;
static const wlc_desc_t set_gripper_control_mode_request_desc;
static const wlc_desc_t set_gripper_control_mode_response_desc;
static const wlc_desc_t motor_register_read_request_desc;
static const wlc_desc_t motor_register_read_response_desc;
static const wlc_desc_t motor_register_write_request_desc;
static const wlc_desc_t motor_register_write_response_desc;
static const wlc_desc_t motor_store_parameters_request_desc;
static const wlc_desc_t motor_store_parameters_response_desc;
static const wlc_desc_t motor_set_zero_request_desc;
static const wlc_desc_t motor_set_zero_response_desc;
static const wlc_desc_t set_arm_mode_request_desc;
static const wlc_desc_t set_arm_mode_response_desc;
static const wlc_desc_t get_device_settings_request_desc;
static const wlc_desc_t get_device_settings_response_desc;
static const wlc_desc_t set_device_settings_request_desc;
static const wlc_desc_t set_device_settings_response_desc;
static const wlc_desc_t joint_mit_command_desc;
static const wlc_desc_t emergency_stop_request_desc;
static const wlc_desc_t emergency_stop_response_desc;
static const wlc_desc_t gripper_mit_command_desc;
static const wlc_desc_t joint_position_velocity_command_desc;
static const wlc_desc_t joint_velocity_command_desc;
static const wlc_desc_t joint_pvt_command_desc;
static const wlc_desc_t cartesian_pose_command_desc;
static const wlc_desc_t cartesian_velocity_command_desc;
static const wlc_desc_t gripper_position_velocity_command_desc;
static const wlc_desc_t gripper_velocity_command_desc;
static const wlc_desc_t gripper_pvt_command_desc;

static const wlc_field_t semantic_version_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(semantic_version_t, major), offsetof(semantic_version_t, has_major), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(semantic_version_t, minor), offsetof(semantic_version_t, has_minor), 0, 0, sizeof(uint32_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(semantic_version_t, patch), offsetof(semantic_version_t, has_patch), 0, 0, sizeof(uint32_t), 0U, 0U, { 24U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t semantic_version_desc = { semantic_version_fields, sizeof(semantic_version_fields) / sizeof(semantic_version_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t device_info_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_MESSAGE, 1, 2U, 1U, offsetof(device_info_t, protocol_version), offsetof(device_info_t, has_protocol_version), 0, 0, sizeof(semantic_version_t), 0U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, &semantic_version_desc },
  { 2U, WLC_OPTIONAL, WLC_MESSAGE, 1, 2U, 1U, offsetof(device_info_t, firmware_version), offsetof(device_info_t, has_firmware_version), 0, 0, sizeof(semantic_version_t), 0U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, &semantic_version_desc },
  { 3U, WLC_OPTIONAL, WLC_STRING, 1, 2U, 1U, offsetof(device_info_t, board_name), offsetof(device_info_t, has_board_name), 0, 0, sizeof(wl_codec_string_t), 0U, 31U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_STRING, 1, 2U, 1U, offsetof(device_info_t, custom_name), offsetof(device_info_t, has_custom_name), 0, 0, sizeof(wl_codec_string_t), 0U, 31U, { 34U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(device_info_t, firmware_type), offsetof(device_info_t, has_firmware_type), 0, 0, sizeof(firmware_type_t), 0U, 0U, { 40U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_STRING, 1, 2U, 1U, offsetof(device_info_t, serial), offsetof(device_info_t, has_serial), 0, 0, sizeof(wl_codec_string_t), 0U, 31U, { 50U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_F64, 0, 1U, 1U, offsetof(device_info_t, command_capabilities), offsetof(device_info_t, has_command_capabilities), 0, 0, sizeof(uint64_t), 0U, 0U, { 57U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t device_info_desc = { device_info_fields, sizeof(device_info_fields) / sizeof(device_info_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t device_settings_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(device_settings_t, firmware_dt_us), offsetof(device_settings_t, has_firmware_dt_us), 0, 0, sizeof(uint32_t), 0U, 0U, { 13U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, gravity_scale), offsetof(device_settings_t, has_gravity_scale), 0, 0, sizeof(float), 6U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, torque_continuous), offsetof(device_settings_t, has_torque_continuous), 0, 0, sizeof(float), 7U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, torque_peak), offsetof(device_settings_t, has_torque_peak), 0, 0, sizeof(float), 7U, 0U, { 34U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, thermal_capacity), offsetof(device_settings_t, has_thermal_capacity), 0, 0, sizeof(float), 7U, 0U, { 42U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, torque_ramp_rate), offsetof(device_settings_t, has_torque_ramp_rate), 0, 0, sizeof(float), 7U, 0U, { 50U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, joint_limit_min), offsetof(device_settings_t, has_joint_limit_min), 0, 0, sizeof(float), 6U, 0U, { 58U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(device_settings_t, joint_limit_max), offsetof(device_settings_t, has_joint_limit_max), 0, 0, sizeof(float), 6U, 0U, { 66U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t device_settings_desc = { device_settings_fields, sizeof(device_settings_fields) / sizeof(device_settings_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t arm_status_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(arm_status_t, mode), offsetof(arm_status_t, has_mode), 0, 0, sizeof(arm_mode_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(arm_status_t, sequence), offsetof(arm_status_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(arm_status_t, timestamp_us), offsetof(arm_status_t, has_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 25U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_status_t, joint_position), offsetof(arm_status_t, has_joint_position), 0, 0, sizeof(float), 6U, 0U, { 34U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_status_t, joint_velocity), offsetof(arm_status_t, has_joint_velocity), 0, 0, sizeof(float), 6U, 0U, { 42U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_status_t, joint_torque), offsetof(arm_status_t, has_joint_torque), 0, 0, sizeof(float), 6U, 0U, { 50U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_status_t, base_gravity), offsetof(arm_status_t, has_base_gravity), 0, 0, sizeof(float), 3U, 0U, { 58U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(arm_status_t, gripper_position), offsetof(arm_status_t, has_gripper_position), 0, 0, sizeof(float), 0U, 0U, { 69U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 9U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(arm_status_t, gripper_velocity), offsetof(arm_status_t, has_gripper_velocity), 0, 0, sizeof(float), 0U, 0U, { 77U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 10U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(arm_status_t, gripper_torque), offsetof(arm_status_t, has_gripper_torque), 0, 0, sizeof(float), 0U, 0U, { 85U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 11U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_status_t, end_effector_transform), offsetof(arm_status_t, has_end_effector_transform), 0, 0, sizeof(float), 16U, 0U, { 90U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 12U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_status_t, external_wrench), offsetof(arm_status_t, has_external_wrench), 0, 0, sizeof(float), 6U, 0U, { 98U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 13U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(arm_status_t, error_flags), offsetof(arm_status_t, has_error_flags), 0, 0, sizeof(uint32_t), 0U, 0U, { 109U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 14U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(arm_status_t, last_sdk_timestamp_us), offsetof(arm_status_t, has_last_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 113U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t arm_status_desc = { arm_status_fields, sizeof(arm_status_fields) / sizeof(arm_status_fields[0]), WLC_LOOKUP_DENSE };

static const wlc_field_t motor_feedback_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(motor_feedback_t, position_rad), offsetof(motor_feedback_t, has_position_rad), 0, 0, sizeof(float), 7U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(motor_feedback_t, velocity_rad_s), offsetof(motor_feedback_t, has_velocity_rad_s), 0, 0, sizeof(float), 7U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(motor_feedback_t, torque_nm), offsetof(motor_feedback_t, has_torque_nm), 0, 0, sizeof(float), 7U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(motor_feedback_t, temperature_c), offsetof(motor_feedback_t, has_temperature_c), 0, 0, sizeof(float), 7U, 0U, { 34U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(motor_feedback_t, device_status_bits), offsetof(motor_feedback_t, has_device_status_bits), 0, 0, sizeof(uint32_t), 0U, 0U, { 45U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_feedback_t, enabled_mask), offsetof(motor_feedback_t, has_enabled_mask), 0, 0, sizeof(uint8_t), 0U, 0U, { 48U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_feedback_desc = { motor_feedback_fields, sizeof(motor_feedback_fields) / sizeof(motor_feedback_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t arm_diagnostics_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(arm_diagnostics_t, uptime_s), offsetof(arm_diagnostics_t, has_uptime_s), 0, 0, sizeof(uint32_t), 0U, 0U, { 13U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(arm_diagnostics_t, tick_count), offsetof(arm_diagnostics_t, has_tick_count), 0, 0, sizeof(uint32_t), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(arm_diagnostics_t, mode_entry_ms), offsetof(arm_diagnostics_t, has_mode_entry_ms), 0, 0, sizeof(uint32_t), 0U, 0U, { 29U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_BOOL, 1, 0U, 1U, offsetof(arm_diagnostics_t, bus_healthy), offsetof(arm_diagnostics_t, has_bus_healthy), 0, 0, sizeof(bool), 0U, 0U, { 32U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(arm_diagnostics_t, bus_state), offsetof(arm_diagnostics_t, has_bus_state), 0, 0, sizeof(uint8_t), 0U, 0U, { 40U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_U16, 1, 0U, 1U, offsetof(arm_diagnostics_t, tx_error_count), offsetof(arm_diagnostics_t, has_tx_error_count), 0, 0, sizeof(uint16_t), 0U, 0U, { 48U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_U16, 1, 0U, 1U, offsetof(arm_diagnostics_t, rx_error_count), offsetof(arm_diagnostics_t, has_rx_error_count), 0, 0, sizeof(uint16_t), 0U, 0U, { 56U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(arm_diagnostics_t, joint_healthy_mask), offsetof(arm_diagnostics_t, has_joint_healthy_mask), 0, 0, sizeof(uint8_t), 0U, 0U, { 64U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 9U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(arm_diagnostics_t, joint_temperature_c), offsetof(arm_diagnostics_t, has_joint_temperature_c), 0, 0, sizeof(float), 6U, 0U, { 74U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 10U, WLC_OPTIONAL, WLC_BOOL, 1, 0U, 1U, offsetof(arm_diagnostics_t, gripper_healthy), offsetof(arm_diagnostics_t, has_gripper_healthy), 0, 0, sizeof(bool), 0U, 0U, { 80U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 11U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(arm_diagnostics_t, gripper_temperature_c), offsetof(arm_diagnostics_t, has_gripper_temperature_c), 0, 0, sizeof(float), 0U, 0U, { 93U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 12U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(arm_diagnostics_t, overheat_mask), offsetof(arm_diagnostics_t, has_overheat_mask), 0, 0, sizeof(uint8_t), 0U, 0U, { 96U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t arm_diagnostics_desc = { arm_diagnostics_fields, sizeof(arm_diagnostics_fields) / sizeof(arm_diagnostics_fields[0]), WLC_LOOKUP_DENSE };

static const wlc_field_t set_zero_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_zero_request_t, operation_id), offsetof(set_zero_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(set_zero_request_t, joint_id), offsetof(set_zero_request_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_zero_request_desc = { set_zero_request_fields, sizeof(set_zero_request_fields) / sizeof(set_zero_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_zero_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_zero_response_t, operation_id), offsetof(set_zero_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_zero_response_t, status), offsetof(set_zero_response_t, has_status), 0, 0, sizeof(fault_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_zero_response_desc = { set_zero_response_fields, sizeof(set_zero_response_fields) / sizeof(set_zero_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t clear_error_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(clear_error_request_t, operation_id), offsetof(clear_error_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(clear_error_request_t, joint_id), offsetof(clear_error_request_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t clear_error_request_desc = { clear_error_request_fields, sizeof(clear_error_request_fields) / sizeof(clear_error_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t clear_error_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(clear_error_response_t, operation_id), offsetof(clear_error_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(clear_error_response_t, status), offsetof(clear_error_response_t, has_status), 0, 0, sizeof(fault_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t clear_error_response_desc = { clear_error_response_fields, sizeof(clear_error_response_fields) / sizeof(clear_error_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t home_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(home_request_t, operation_id), offsetof(home_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t home_request_desc = { home_request_fields, sizeof(home_request_fields) / sizeof(home_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t home_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(home_response_t, operation_id), offsetof(home_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(home_response_t, status), offsetof(home_response_t, has_status), 0, 0, sizeof(home_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t home_response_desc = { home_response_fields, sizeof(home_response_fields) / sizeof(home_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t clear_faults_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(clear_faults_request_t, operation_id), offsetof(clear_faults_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t clear_faults_request_desc = { clear_faults_request_fields, sizeof(clear_faults_request_fields) / sizeof(clear_faults_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t clear_faults_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(clear_faults_response_t, operation_id), offsetof(clear_faults_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(clear_faults_response_t, status), offsetof(clear_faults_response_t, has_status), 0, 0, sizeof(fault_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t clear_faults_response_desc = { clear_faults_response_fields, sizeof(clear_faults_response_fields) / sizeof(clear_faults_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t acquire_control_lease_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(acquire_control_lease_request_t, operation_id), offsetof(acquire_control_lease_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(acquire_control_lease_request_t, requested_timeout_ms), offsetof(acquire_control_lease_request_t, has_requested_timeout_ms), 0, 0, sizeof(uint32_t), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F64, 0, 1U, 1U, offsetof(acquire_control_lease_request_t, current_token), offsetof(acquire_control_lease_request_t, has_current_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 25U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t acquire_control_lease_request_desc = { acquire_control_lease_request_fields, sizeof(acquire_control_lease_request_fields) / sizeof(acquire_control_lease_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t acquire_control_lease_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(acquire_control_lease_response_t, operation_id), offsetof(acquire_control_lease_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(acquire_control_lease_response_t, status), offsetof(acquire_control_lease_response_t, has_status), 0, 0, sizeof(control_lease_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F64, 0, 1U, 1U, offsetof(acquire_control_lease_response_t, lease_token), offsetof(acquire_control_lease_response_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 25U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F32, 0, 5U, 1U, offsetof(acquire_control_lease_response_t, granted_timeout_ms), offsetof(acquire_control_lease_response_t, has_granted_timeout_ms), 0, 0, sizeof(uint32_t), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t acquire_control_lease_response_desc = { acquire_control_lease_response_fields, sizeof(acquire_control_lease_response_fields) / sizeof(acquire_control_lease_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t release_control_lease_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(release_control_lease_request_t, operation_id), offsetof(release_control_lease_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(release_control_lease_request_t, lease_token), offsetof(release_control_lease_request_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 17U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t release_control_lease_request_desc = { release_control_lease_request_fields, sizeof(release_control_lease_request_fields) / sizeof(release_control_lease_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t release_control_lease_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(release_control_lease_response_t, operation_id), offsetof(release_control_lease_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(release_control_lease_response_t, status), offsetof(release_control_lease_response_t, has_status), 0, 0, sizeof(control_lease_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t release_control_lease_response_desc = { release_control_lease_response_fields, sizeof(release_control_lease_response_fields) / sizeof(release_control_lease_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t get_motor_feedback_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(get_motor_feedback_request_t, operation_id), offsetof(get_motor_feedback_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t get_motor_feedback_request_desc = { get_motor_feedback_request_fields, sizeof(get_motor_feedback_request_fields) / sizeof(get_motor_feedback_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t get_motor_feedback_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(get_motor_feedback_response_t, operation_id), offsetof(get_motor_feedback_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(get_motor_feedback_response_t, status), offsetof(get_motor_feedback_response_t, has_status), 0, 0, sizeof(motor_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_MESSAGE, 0, 2U, 1U, offsetof(get_motor_feedback_response_t, feedback), offsetof(get_motor_feedback_response_t, has_feedback), 0, 0, sizeof(motor_feedback_t), 0U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, &motor_feedback_desc },
};
static const wlc_desc_t get_motor_feedback_response_desc = { get_motor_feedback_response_fields, sizeof(get_motor_feedback_response_fields) / sizeof(get_motor_feedback_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t get_device_info_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(get_device_info_request_t, operation_id), offsetof(get_device_info_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t get_device_info_request_desc = { get_device_info_request_fields, sizeof(get_device_info_request_fields) / sizeof(get_device_info_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t get_device_info_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(get_device_info_response_t, operation_id), offsetof(get_device_info_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(get_device_info_response_t, status), offsetof(get_device_info_response_t, has_status), 0, 0, sizeof(device_info_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_MESSAGE, 0, 2U, 1U, offsetof(get_device_info_response_t, info), offsetof(get_device_info_response_t, has_info), 0, 0, sizeof(device_info_t), 0U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, &device_info_desc },
};
static const wlc_desc_t get_device_info_response_desc = { get_device_info_response_fields, sizeof(get_device_info_response_fields) / sizeof(get_device_info_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_device_info_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_device_info_request_t, operation_id), offsetof(set_device_info_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_STRING, 1, 2U, 1U, offsetof(set_device_info_request_t, custom_name), offsetof(set_device_info_request_t, has_custom_name), 0, 0, sizeof(wl_codec_string_t), 0U, 31U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_device_info_request_desc = { set_device_info_request_fields, sizeof(set_device_info_request_fields) / sizeof(set_device_info_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_device_info_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_device_info_response_t, operation_id), offsetof(set_device_info_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_device_info_response_t, status), offsetof(set_device_info_response_t, has_status), 0, 0, sizeof(device_info_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_device_info_response_desc = { set_device_info_response_fields, sizeof(set_device_info_response_fields) / sizeof(set_device_info_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_arm_control_mode_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_arm_control_mode_request_t, operation_id), offsetof(set_arm_control_mode_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_arm_control_mode_request_t, mode), offsetof(set_arm_control_mode_request_t, has_mode), 0, 0, sizeof(motor_control_mode_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_arm_control_mode_request_desc = { set_arm_control_mode_request_fields, sizeof(set_arm_control_mode_request_fields) / sizeof(set_arm_control_mode_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_arm_control_mode_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_arm_control_mode_response_t, operation_id), offsetof(set_arm_control_mode_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_arm_control_mode_response_t, status), offsetof(set_arm_control_mode_response_t, has_status), 0, 0, sizeof(mode_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_arm_control_mode_response_desc = { set_arm_control_mode_response_fields, sizeof(set_arm_control_mode_response_fields) / sizeof(set_arm_control_mode_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_gripper_control_mode_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_gripper_control_mode_request_t, operation_id), offsetof(set_gripper_control_mode_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_gripper_control_mode_request_t, mode), offsetof(set_gripper_control_mode_request_t, has_mode), 0, 0, sizeof(motor_control_mode_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_gripper_control_mode_request_desc = { set_gripper_control_mode_request_fields, sizeof(set_gripper_control_mode_request_fields) / sizeof(set_gripper_control_mode_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_gripper_control_mode_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_gripper_control_mode_response_t, operation_id), offsetof(set_gripper_control_mode_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_gripper_control_mode_response_t, status), offsetof(set_gripper_control_mode_response_t, has_status), 0, 0, sizeof(mode_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_gripper_control_mode_response_desc = { set_gripper_control_mode_response_fields, sizeof(set_gripper_control_mode_response_fields) / sizeof(set_gripper_control_mode_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_register_read_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_register_read_request_t, operation_id), offsetof(motor_register_read_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_register_read_request_t, joint_id), offsetof(motor_register_read_request_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_register_read_request_t, register_id), offsetof(motor_register_read_request_t, has_register_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 24U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_register_read_request_desc = { motor_register_read_request_fields, sizeof(motor_register_read_request_fields) / sizeof(motor_register_read_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_register_read_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_register_read_response_t, operation_id), offsetof(motor_register_read_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(motor_register_read_response_t, status), offsetof(motor_register_read_response_t, has_status), 0, 0, sizeof(motor_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_U8, 0, 0U, 1U, offsetof(motor_register_read_response_t, joint_id), offsetof(motor_register_read_response_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 24U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_U8, 0, 0U, 1U, offsetof(motor_register_read_response_t, register_id), offsetof(motor_register_read_response_t, has_register_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 32U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_FLOAT32, 0, 5U, 1U, offsetof(motor_register_read_response_t, value), offsetof(motor_register_read_response_t, has_value), 0, 0, sizeof(float), 0U, 0U, { 45U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_register_read_response_desc = { motor_register_read_response_fields, sizeof(motor_register_read_response_fields) / sizeof(motor_register_read_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_register_write_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_register_write_request_t, operation_id), offsetof(motor_register_write_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_register_write_request_t, joint_id), offsetof(motor_register_write_request_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_register_write_request_t, register_id), offsetof(motor_register_write_request_t, has_register_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 24U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(motor_register_write_request_t, value), offsetof(motor_register_write_request_t, has_value), 0, 0, sizeof(float), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_register_write_request_desc = { motor_register_write_request_fields, sizeof(motor_register_write_request_fields) / sizeof(motor_register_write_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_register_write_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_register_write_response_t, operation_id), offsetof(motor_register_write_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(motor_register_write_response_t, status), offsetof(motor_register_write_response_t, has_status), 0, 0, sizeof(motor_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_register_write_response_desc = { motor_register_write_response_fields, sizeof(motor_register_write_response_fields) / sizeof(motor_register_write_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_store_parameters_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_store_parameters_request_t, operation_id), offsetof(motor_store_parameters_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_store_parameters_request_t, joint_id), offsetof(motor_store_parameters_request_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_store_parameters_request_desc = { motor_store_parameters_request_fields, sizeof(motor_store_parameters_request_fields) / sizeof(motor_store_parameters_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_store_parameters_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_store_parameters_response_t, operation_id), offsetof(motor_store_parameters_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(motor_store_parameters_response_t, status), offsetof(motor_store_parameters_response_t, has_status), 0, 0, sizeof(motor_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_store_parameters_response_desc = { motor_store_parameters_response_fields, sizeof(motor_store_parameters_response_fields) / sizeof(motor_store_parameters_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_set_zero_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_set_zero_request_t, operation_id), offsetof(motor_set_zero_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(motor_set_zero_request_t, joint_id), offsetof(motor_set_zero_request_t, has_joint_id), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_set_zero_request_desc = { motor_set_zero_request_fields, sizeof(motor_set_zero_request_fields) / sizeof(motor_set_zero_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t motor_set_zero_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(motor_set_zero_response_t, operation_id), offsetof(motor_set_zero_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(motor_set_zero_response_t, status), offsetof(motor_set_zero_response_t, has_status), 0, 0, sizeof(motor_operation_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t motor_set_zero_response_desc = { motor_set_zero_response_fields, sizeof(motor_set_zero_response_fields) / sizeof(motor_set_zero_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_arm_mode_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_arm_mode_request_t, operation_id), offsetof(set_arm_mode_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_arm_mode_request_t, mode), offsetof(set_arm_mode_request_t, has_mode), 0, 0, sizeof(arm_mode_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_arm_mode_request_desc = { set_arm_mode_request_fields, sizeof(set_arm_mode_request_fields) / sizeof(set_arm_mode_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_arm_mode_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_arm_mode_response_t, operation_id), offsetof(set_arm_mode_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_arm_mode_response_t, status), offsetof(set_arm_mode_response_t, has_status), 0, 0, sizeof(mode_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t set_arm_mode_response_desc = { set_arm_mode_response_fields, sizeof(set_arm_mode_response_fields) / sizeof(set_arm_mode_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t get_device_settings_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(get_device_settings_request_t, operation_id), offsetof(get_device_settings_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t get_device_settings_request_desc = { get_device_settings_request_fields, sizeof(get_device_settings_request_fields) / sizeof(get_device_settings_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t get_device_settings_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(get_device_settings_response_t, operation_id), offsetof(get_device_settings_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(get_device_settings_response_t, status), offsetof(get_device_settings_response_t, has_status), 0, 0, sizeof(device_settings_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_MESSAGE, 0, 2U, 1U, offsetof(get_device_settings_response_t, settings), offsetof(get_device_settings_response_t, has_settings), 0, 0, sizeof(device_settings_t), 0U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, &device_settings_desc },
};
static const wlc_desc_t get_device_settings_response_desc = { get_device_settings_response_fields, sizeof(get_device_settings_response_fields) / sizeof(get_device_settings_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_device_settings_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_device_settings_request_t, operation_id), offsetof(set_device_settings_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_MESSAGE, 1, 2U, 1U, offsetof(set_device_settings_request_t, settings), offsetof(set_device_settings_request_t, has_settings), 0, 0, sizeof(device_settings_t), 0U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, &device_settings_desc },
};
static const wlc_desc_t set_device_settings_request_desc = { set_device_settings_request_fields, sizeof(set_device_settings_request_fields) / sizeof(set_device_settings_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t set_device_settings_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(set_device_settings_response_t, operation_id), offsetof(set_device_settings_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(set_device_settings_response_t, status), offsetof(set_device_settings_response_t, has_status), 0, 0, sizeof(device_settings_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_MESSAGE, 0, 2U, 1U, offsetof(set_device_settings_response_t, settings), offsetof(set_device_settings_response_t, has_settings), 0, 0, sizeof(device_settings_t), 0U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, &device_settings_desc },
};
static const wlc_desc_t set_device_settings_response_desc = { set_device_settings_response_fields, sizeof(set_device_settings_response_fields) / sizeof(set_device_settings_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t joint_mit_command_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_mit_command_t, position), offsetof(joint_mit_command_t, has_position), 0, 0, sizeof(float), 6U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_mit_command_t, velocity), offsetof(joint_mit_command_t, has_velocity), 0, 0, sizeof(float), 6U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_mit_command_t, torque), offsetof(joint_mit_command_t, has_torque), 0, 0, sizeof(float), 6U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_mit_command_t, kp), offsetof(joint_mit_command_t, has_kp), 0, 0, sizeof(float), 6U, 0U, { 34U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_mit_command_t, kd), offsetof(joint_mit_command_t, has_kd), 0, 0, sizeof(float), 6U, 0U, { 42U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(joint_mit_command_t, dt_us), offsetof(joint_mit_command_t, has_dt_us), 0, 0, sizeof(uint32_t), 0U, 0U, { 53U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(joint_mit_command_t, sequence), offsetof(joint_mit_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 61U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_OPTIONAL, WLC_BOOL, 1, 0U, 1U, offsetof(joint_mit_command_t, gravity_compensation), offsetof(joint_mit_command_t, has_gravity_compensation), 0, 0, sizeof(bool), 0U, 0U, { 64U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 9U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_mit_command_t, sdk_timestamp_us), offsetof(joint_mit_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 73U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 10U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_mit_command_t, lease_token), offsetof(joint_mit_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 81U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t joint_mit_command_desc = { joint_mit_command_fields, sizeof(joint_mit_command_fields) / sizeof(joint_mit_command_fields[0]), WLC_LOOKUP_DENSE };

static const wlc_field_t emergency_stop_request_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(emergency_stop_request_t, operation_id), offsetof(emergency_stop_request_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t emergency_stop_request_desc = { emergency_stop_request_fields, sizeof(emergency_stop_request_fields) / sizeof(emergency_stop_request_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t emergency_stop_response_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_U32, 1, 0U, 1U, offsetof(emergency_stop_response_t, operation_id), offsetof(emergency_stop_response_t, has_operation_id), 0, 0, sizeof(uint32_t), 0U, 0U, { 8U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_ENUM, 1, 0U, 1U, offsetof(emergency_stop_response_t, status), offsetof(emergency_stop_response_t, has_status), 0, 0, sizeof(emergency_stop_status_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t emergency_stop_response_desc = { emergency_stop_response_fields, sizeof(emergency_stop_response_fields) / sizeof(emergency_stop_response_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t gripper_mit_command_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_mit_command_t, position), offsetof(gripper_mit_command_t, has_position), 0, 0, sizeof(float), 0U, 0U, { 13U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_mit_command_t, velocity), offsetof(gripper_mit_command_t, has_velocity), 0, 0, sizeof(float), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_mit_command_t, torque), offsetof(gripper_mit_command_t, has_torque), 0, 0, sizeof(float), 0U, 0U, { 29U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_mit_command_t, kp), offsetof(gripper_mit_command_t, has_kp), 0, 0, sizeof(float), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_mit_command_t, kd), offsetof(gripper_mit_command_t, has_kd), 0, 0, sizeof(float), 0U, 0U, { 45U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(gripper_mit_command_t, dt_us), offsetof(gripper_mit_command_t, has_dt_us), 0, 0, sizeof(uint32_t), 0U, 0U, { 53U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(gripper_mit_command_t, sequence), offsetof(gripper_mit_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 61U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_OPTIONAL, WLC_BOOL, 1, 0U, 1U, offsetof(gripper_mit_command_t, gravity_compensation), offsetof(gripper_mit_command_t, has_gravity_compensation), 0, 0, sizeof(bool), 0U, 0U, { 64U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 9U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_mit_command_t, sdk_timestamp_us), offsetof(gripper_mit_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 73U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 10U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_mit_command_t, lease_token), offsetof(gripper_mit_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 81U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t gripper_mit_command_desc = { gripper_mit_command_fields, sizeof(gripper_mit_command_fields) / sizeof(gripper_mit_command_fields[0]), WLC_LOOKUP_DENSE };

static const wlc_field_t joint_position_velocity_command_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_position_velocity_command_t, position), offsetof(joint_position_velocity_command_t, has_position), 0, 0, sizeof(float), 6U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_position_velocity_command_t, velocity), offsetof(joint_position_velocity_command_t, has_velocity), 0, 0, sizeof(float), 6U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(joint_position_velocity_command_t, enabled_mask), offsetof(joint_position_velocity_command_t, has_enabled_mask), 0, 0, sizeof(uint8_t), 0U, 0U, { 24U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(joint_position_velocity_command_t, sequence), offsetof(joint_position_velocity_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_position_velocity_command_t, sdk_timestamp_us), offsetof(joint_position_velocity_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 41U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_position_velocity_command_t, lease_token), offsetof(joint_position_velocity_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 49U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t joint_position_velocity_command_desc = { joint_position_velocity_command_fields, sizeof(joint_position_velocity_command_fields) / sizeof(joint_position_velocity_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t joint_velocity_command_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_velocity_command_t, velocity), offsetof(joint_velocity_command_t, has_velocity), 0, 0, sizeof(float), 6U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(joint_velocity_command_t, enabled_mask), offsetof(joint_velocity_command_t, has_enabled_mask), 0, 0, sizeof(uint8_t), 0U, 0U, { 16U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(joint_velocity_command_t, sequence), offsetof(joint_velocity_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 29U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_velocity_command_t, sdk_timestamp_us), offsetof(joint_velocity_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 33U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_velocity_command_t, lease_token), offsetof(joint_velocity_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 41U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t joint_velocity_command_desc = { joint_velocity_command_fields, sizeof(joint_velocity_command_fields) / sizeof(joint_velocity_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t joint_pvt_command_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_pvt_command_t, position), offsetof(joint_pvt_command_t, has_position), 0, 0, sizeof(float), 6U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_pvt_command_t, velocity_limit), offsetof(joint_pvt_command_t, has_velocity_limit), 0, 0, sizeof(float), 6U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(joint_pvt_command_t, current_limit_normalized), offsetof(joint_pvt_command_t, has_current_limit_normalized), 0, 0, sizeof(float), 6U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_U8, 1, 0U, 1U, offsetof(joint_pvt_command_t, enabled_mask), offsetof(joint_pvt_command_t, has_enabled_mask), 0, 0, sizeof(uint8_t), 0U, 0U, { 32U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(joint_pvt_command_t, sequence), offsetof(joint_pvt_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 45U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_pvt_command_t, sdk_timestamp_us), offsetof(joint_pvt_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 49U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(joint_pvt_command_t, lease_token), offsetof(joint_pvt_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 57U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t joint_pvt_command_desc = { joint_pvt_command_fields, sizeof(joint_pvt_command_fields) / sizeof(joint_pvt_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t cartesian_pose_command_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(cartesian_pose_command_t, transform), offsetof(cartesian_pose_command_t, has_transform), 0, 0, sizeof(float), 16U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(cartesian_pose_command_t, kp), offsetof(cartesian_pose_command_t, has_kp), 0, 0, sizeof(float), 6U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(cartesian_pose_command_t, kd), offsetof(cartesian_pose_command_t, has_kd), 0, 0, sizeof(float), 6U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(cartesian_pose_command_t, dt_us), offsetof(cartesian_pose_command_t, has_dt_us), 0, 0, sizeof(uint32_t), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(cartesian_pose_command_t, sequence), offsetof(cartesian_pose_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 45U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_BOOL, 1, 0U, 1U, offsetof(cartesian_pose_command_t, gravity_compensation), offsetof(cartesian_pose_command_t, has_gravity_compensation), 0, 0, sizeof(bool), 0U, 0U, { 48U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(cartesian_pose_command_t, sdk_timestamp_us), offsetof(cartesian_pose_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 57U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(cartesian_pose_command_t, lease_token), offsetof(cartesian_pose_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 65U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t cartesian_pose_command_desc = { cartesian_pose_command_fields, sizeof(cartesian_pose_command_fields) / sizeof(cartesian_pose_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t cartesian_velocity_command_fields[] = {
  { 1U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(cartesian_velocity_command_t, twist), offsetof(cartesian_velocity_command_t, has_twist), 0, 0, sizeof(float), 6U, 0U, { 10U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(cartesian_velocity_command_t, kp), offsetof(cartesian_velocity_command_t, has_kp), 0, 0, sizeof(float), 6U, 0U, { 18U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_PACKED, WLC_FLOAT32, 1, 2U, 1U, offsetof(cartesian_velocity_command_t, kd), offsetof(cartesian_velocity_command_t, has_kd), 0, 0, sizeof(float), 6U, 0U, { 26U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(cartesian_velocity_command_t, dt_us), offsetof(cartesian_velocity_command_t, has_dt_us), 0, 0, sizeof(uint32_t), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(cartesian_velocity_command_t, sequence), offsetof(cartesian_velocity_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 45U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_BOOL, 1, 0U, 1U, offsetof(cartesian_velocity_command_t, gravity_compensation), offsetof(cartesian_velocity_command_t, has_gravity_compensation), 0, 0, sizeof(bool), 0U, 0U, { 48U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 7U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(cartesian_velocity_command_t, sdk_timestamp_us), offsetof(cartesian_velocity_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 57U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 8U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(cartesian_velocity_command_t, lease_token), offsetof(cartesian_velocity_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 65U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t cartesian_velocity_command_desc = { cartesian_velocity_command_fields, sizeof(cartesian_velocity_command_fields) / sizeof(cartesian_velocity_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t gripper_position_velocity_command_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_position_velocity_command_t, position), offsetof(gripper_position_velocity_command_t, has_position), 0, 0, sizeof(float), 0U, 0U, { 13U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_position_velocity_command_t, velocity), offsetof(gripper_position_velocity_command_t, has_velocity), 0, 0, sizeof(float), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(gripper_position_velocity_command_t, sequence), offsetof(gripper_position_velocity_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 29U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_position_velocity_command_t, sdk_timestamp_us), offsetof(gripper_position_velocity_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 33U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_position_velocity_command_t, lease_token), offsetof(gripper_position_velocity_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 41U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t gripper_position_velocity_command_desc = { gripper_position_velocity_command_fields, sizeof(gripper_position_velocity_command_fields) / sizeof(gripper_position_velocity_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t gripper_velocity_command_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_velocity_command_t, velocity), offsetof(gripper_velocity_command_t, has_velocity), 0, 0, sizeof(float), 0U, 0U, { 13U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(gripper_velocity_command_t, sequence), offsetof(gripper_velocity_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_velocity_command_t, sdk_timestamp_us), offsetof(gripper_velocity_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 25U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_velocity_command_t, lease_token), offsetof(gripper_velocity_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 33U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t gripper_velocity_command_desc = { gripper_velocity_command_fields, sizeof(gripper_velocity_command_fields) / sizeof(gripper_velocity_command_fields[0]), WLC_LOOKUP_LINEAR };

static const wlc_field_t gripper_pvt_command_fields[] = {
  { 1U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_pvt_command_t, position), offsetof(gripper_pvt_command_t, has_position), 0, 0, sizeof(float), 0U, 0U, { 13U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 2U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_pvt_command_t, velocity_limit), offsetof(gripper_pvt_command_t, has_velocity_limit), 0, 0, sizeof(float), 0U, 0U, { 21U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 3U, WLC_OPTIONAL, WLC_FLOAT32, 1, 5U, 1U, offsetof(gripper_pvt_command_t, current_limit_normalized), offsetof(gripper_pvt_command_t, has_current_limit_normalized), 0, 0, sizeof(float), 0U, 0U, { 29U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 4U, WLC_OPTIONAL, WLC_F32, 1, 5U, 1U, offsetof(gripper_pvt_command_t, sequence), offsetof(gripper_pvt_command_t, has_sequence), 0, 0, sizeof(uint32_t), 0U, 0U, { 37U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 5U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_pvt_command_t, sdk_timestamp_us), offsetof(gripper_pvt_command_t, has_sdk_timestamp_us), 0, 0, sizeof(uint64_t), 0U, 0U, { 41U, 0U, 0U }, 0, 0ULL, NULL, NULL },
  { 6U, WLC_OPTIONAL, WLC_F64, 1, 1U, 1U, offsetof(gripper_pvt_command_t, lease_token), offsetof(gripper_pvt_command_t, has_lease_token), 0, 0, sizeof(uint64_t), 0U, 0U, { 49U, 0U, 0U }, 0, 0ULL, NULL, NULL },
};
static const wlc_desc_t gripper_pvt_command_desc = { gripper_pvt_command_fields, sizeof(gripper_pvt_command_fields) / sizeof(gripper_pvt_command_fields[0]), WLC_LOOKUP_LINEAR };

void semantic_version_clear(semantic_version_t *value) { if (value != NULL) wlc_clear(&semantic_version_desc, value); }
size_t semantic_version_encoded_size(const semantic_version_t *value) { size_t size; return wlc_measure(&semantic_version_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t semantic_version_encode(const semantic_version_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&semantic_version_desc, value, out, cap, length); }
wl_codec_status_t semantic_version_decode(const uint8_t *input, size_t length, semantic_version_t *out) { return wlc_decode(&semantic_version_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t semantic_version_wlc_detail_fingerprint(const semantic_version_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&semantic_version_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void device_info_clear(device_info_t *value) { if (value != NULL) wlc_clear(&device_info_desc, value); }
size_t device_info_encoded_size(const device_info_t *value) { size_t size; return wlc_measure(&device_info_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t device_info_encode(const device_info_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&device_info_desc, value, out, cap, length); }
wl_codec_status_t device_info_decode(const uint8_t *input, size_t length, device_info_t *out) { return wlc_decode(&device_info_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t device_info_wlc_detail_fingerprint(const device_info_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&device_info_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void device_settings_clear(device_settings_t *value) { if (value != NULL) wlc_clear(&device_settings_desc, value); }
size_t device_settings_encoded_size(const device_settings_t *value) { size_t size; return wlc_measure(&device_settings_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t device_settings_encode(const device_settings_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&device_settings_desc, value, out, cap, length); }
wl_codec_status_t device_settings_decode(const uint8_t *input, size_t length, device_settings_t *out) { return wlc_decode(&device_settings_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t device_settings_wlc_detail_fingerprint(const device_settings_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&device_settings_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void arm_status_clear(arm_status_t *value) { if (value != NULL) wlc_clear(&arm_status_desc, value); }
size_t arm_status_encoded_size(const arm_status_t *value) { size_t size; return wlc_measure(&arm_status_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t arm_status_encode(const arm_status_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&arm_status_desc, value, out, cap, length); }
wl_codec_status_t arm_status_decode(const uint8_t *input, size_t length, arm_status_t *out) { return wlc_decode(&arm_status_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t arm_status_wlc_detail_fingerprint(const arm_status_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&arm_status_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_feedback_clear(motor_feedback_t *value) { if (value != NULL) wlc_clear(&motor_feedback_desc, value); }
size_t motor_feedback_encoded_size(const motor_feedback_t *value) { size_t size; return wlc_measure(&motor_feedback_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_feedback_encode(const motor_feedback_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_feedback_desc, value, out, cap, length); }
wl_codec_status_t motor_feedback_decode(const uint8_t *input, size_t length, motor_feedback_t *out) { return wlc_decode(&motor_feedback_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_feedback_wlc_detail_fingerprint(const motor_feedback_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_feedback_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void arm_diagnostics_clear(arm_diagnostics_t *value) { if (value != NULL) wlc_clear(&arm_diagnostics_desc, value); }
size_t arm_diagnostics_encoded_size(const arm_diagnostics_t *value) { size_t size; return wlc_measure(&arm_diagnostics_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t arm_diagnostics_encode(const arm_diagnostics_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&arm_diagnostics_desc, value, out, cap, length); }
wl_codec_status_t arm_diagnostics_decode(const uint8_t *input, size_t length, arm_diagnostics_t *out) { return wlc_decode(&arm_diagnostics_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t arm_diagnostics_wlc_detail_fingerprint(const arm_diagnostics_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&arm_diagnostics_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_zero_request_clear(set_zero_request_t *value) { if (value != NULL) wlc_clear(&set_zero_request_desc, value); }
size_t set_zero_request_encoded_size(const set_zero_request_t *value) { size_t size; return wlc_measure(&set_zero_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_zero_request_encode(const set_zero_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_zero_request_desc, value, out, cap, length); }
wl_codec_status_t set_zero_request_decode(const uint8_t *input, size_t length, set_zero_request_t *out) { return wlc_decode(&set_zero_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_zero_request_wlc_detail_fingerprint(const set_zero_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_zero_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_zero_response_clear(set_zero_response_t *value) { if (value != NULL) wlc_clear(&set_zero_response_desc, value); }
size_t set_zero_response_encoded_size(const set_zero_response_t *value) { size_t size; return wlc_measure(&set_zero_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_zero_response_encode(const set_zero_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_zero_response_desc, value, out, cap, length); }
wl_codec_status_t set_zero_response_decode(const uint8_t *input, size_t length, set_zero_response_t *out) { return wlc_decode(&set_zero_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_zero_response_wlc_detail_fingerprint(const set_zero_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_zero_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void clear_error_request_clear(clear_error_request_t *value) { if (value != NULL) wlc_clear(&clear_error_request_desc, value); }
size_t clear_error_request_encoded_size(const clear_error_request_t *value) { size_t size; return wlc_measure(&clear_error_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t clear_error_request_encode(const clear_error_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&clear_error_request_desc, value, out, cap, length); }
wl_codec_status_t clear_error_request_decode(const uint8_t *input, size_t length, clear_error_request_t *out) { return wlc_decode(&clear_error_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t clear_error_request_wlc_detail_fingerprint(const clear_error_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&clear_error_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void clear_error_response_clear(clear_error_response_t *value) { if (value != NULL) wlc_clear(&clear_error_response_desc, value); }
size_t clear_error_response_encoded_size(const clear_error_response_t *value) { size_t size; return wlc_measure(&clear_error_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t clear_error_response_encode(const clear_error_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&clear_error_response_desc, value, out, cap, length); }
wl_codec_status_t clear_error_response_decode(const uint8_t *input, size_t length, clear_error_response_t *out) { return wlc_decode(&clear_error_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t clear_error_response_wlc_detail_fingerprint(const clear_error_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&clear_error_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void home_request_clear(home_request_t *value) { if (value != NULL) wlc_clear(&home_request_desc, value); }
size_t home_request_encoded_size(const home_request_t *value) { size_t size; return wlc_measure(&home_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t home_request_encode(const home_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&home_request_desc, value, out, cap, length); }
wl_codec_status_t home_request_decode(const uint8_t *input, size_t length, home_request_t *out) { return wlc_decode(&home_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t home_request_wlc_detail_fingerprint(const home_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&home_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void home_response_clear(home_response_t *value) { if (value != NULL) wlc_clear(&home_response_desc, value); }
size_t home_response_encoded_size(const home_response_t *value) { size_t size; return wlc_measure(&home_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t home_response_encode(const home_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&home_response_desc, value, out, cap, length); }
wl_codec_status_t home_response_decode(const uint8_t *input, size_t length, home_response_t *out) { return wlc_decode(&home_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t home_response_wlc_detail_fingerprint(const home_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&home_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void clear_faults_request_clear(clear_faults_request_t *value) { if (value != NULL) wlc_clear(&clear_faults_request_desc, value); }
size_t clear_faults_request_encoded_size(const clear_faults_request_t *value) { size_t size; return wlc_measure(&clear_faults_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t clear_faults_request_encode(const clear_faults_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&clear_faults_request_desc, value, out, cap, length); }
wl_codec_status_t clear_faults_request_decode(const uint8_t *input, size_t length, clear_faults_request_t *out) { return wlc_decode(&clear_faults_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t clear_faults_request_wlc_detail_fingerprint(const clear_faults_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&clear_faults_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void clear_faults_response_clear(clear_faults_response_t *value) { if (value != NULL) wlc_clear(&clear_faults_response_desc, value); }
size_t clear_faults_response_encoded_size(const clear_faults_response_t *value) { size_t size; return wlc_measure(&clear_faults_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t clear_faults_response_encode(const clear_faults_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&clear_faults_response_desc, value, out, cap, length); }
wl_codec_status_t clear_faults_response_decode(const uint8_t *input, size_t length, clear_faults_response_t *out) { return wlc_decode(&clear_faults_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t clear_faults_response_wlc_detail_fingerprint(const clear_faults_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&clear_faults_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void acquire_control_lease_request_clear(acquire_control_lease_request_t *value) { if (value != NULL) wlc_clear(&acquire_control_lease_request_desc, value); }
size_t acquire_control_lease_request_encoded_size(const acquire_control_lease_request_t *value) { size_t size; return wlc_measure(&acquire_control_lease_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t acquire_control_lease_request_encode(const acquire_control_lease_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&acquire_control_lease_request_desc, value, out, cap, length); }
wl_codec_status_t acquire_control_lease_request_decode(const uint8_t *input, size_t length, acquire_control_lease_request_t *out) { return wlc_decode(&acquire_control_lease_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t acquire_control_lease_request_wlc_detail_fingerprint(const acquire_control_lease_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&acquire_control_lease_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void acquire_control_lease_response_clear(acquire_control_lease_response_t *value) { if (value != NULL) wlc_clear(&acquire_control_lease_response_desc, value); }
size_t acquire_control_lease_response_encoded_size(const acquire_control_lease_response_t *value) { size_t size; return wlc_measure(&acquire_control_lease_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t acquire_control_lease_response_encode(const acquire_control_lease_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&acquire_control_lease_response_desc, value, out, cap, length); }
wl_codec_status_t acquire_control_lease_response_decode(const uint8_t *input, size_t length, acquire_control_lease_response_t *out) { return wlc_decode(&acquire_control_lease_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t acquire_control_lease_response_wlc_detail_fingerprint(const acquire_control_lease_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&acquire_control_lease_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void release_control_lease_request_clear(release_control_lease_request_t *value) { if (value != NULL) wlc_clear(&release_control_lease_request_desc, value); }
size_t release_control_lease_request_encoded_size(const release_control_lease_request_t *value) { size_t size; return wlc_measure(&release_control_lease_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t release_control_lease_request_encode(const release_control_lease_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&release_control_lease_request_desc, value, out, cap, length); }
wl_codec_status_t release_control_lease_request_decode(const uint8_t *input, size_t length, release_control_lease_request_t *out) { return wlc_decode(&release_control_lease_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t release_control_lease_request_wlc_detail_fingerprint(const release_control_lease_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&release_control_lease_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void release_control_lease_response_clear(release_control_lease_response_t *value) { if (value != NULL) wlc_clear(&release_control_lease_response_desc, value); }
size_t release_control_lease_response_encoded_size(const release_control_lease_response_t *value) { size_t size; return wlc_measure(&release_control_lease_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t release_control_lease_response_encode(const release_control_lease_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&release_control_lease_response_desc, value, out, cap, length); }
wl_codec_status_t release_control_lease_response_decode(const uint8_t *input, size_t length, release_control_lease_response_t *out) { return wlc_decode(&release_control_lease_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t release_control_lease_response_wlc_detail_fingerprint(const release_control_lease_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&release_control_lease_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void get_motor_feedback_request_clear(get_motor_feedback_request_t *value) { if (value != NULL) wlc_clear(&get_motor_feedback_request_desc, value); }
size_t get_motor_feedback_request_encoded_size(const get_motor_feedback_request_t *value) { size_t size; return wlc_measure(&get_motor_feedback_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t get_motor_feedback_request_encode(const get_motor_feedback_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&get_motor_feedback_request_desc, value, out, cap, length); }
wl_codec_status_t get_motor_feedback_request_decode(const uint8_t *input, size_t length, get_motor_feedback_request_t *out) { return wlc_decode(&get_motor_feedback_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t get_motor_feedback_request_wlc_detail_fingerprint(const get_motor_feedback_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&get_motor_feedback_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void get_motor_feedback_response_clear(get_motor_feedback_response_t *value) { if (value != NULL) wlc_clear(&get_motor_feedback_response_desc, value); }
size_t get_motor_feedback_response_encoded_size(const get_motor_feedback_response_t *value) { size_t size; return wlc_measure(&get_motor_feedback_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t get_motor_feedback_response_encode(const get_motor_feedback_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&get_motor_feedback_response_desc, value, out, cap, length); }
wl_codec_status_t get_motor_feedback_response_decode(const uint8_t *input, size_t length, get_motor_feedback_response_t *out) { return wlc_decode(&get_motor_feedback_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t get_motor_feedback_response_wlc_detail_fingerprint(const get_motor_feedback_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&get_motor_feedback_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void get_device_info_request_clear(get_device_info_request_t *value) { if (value != NULL) wlc_clear(&get_device_info_request_desc, value); }
size_t get_device_info_request_encoded_size(const get_device_info_request_t *value) { size_t size; return wlc_measure(&get_device_info_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t get_device_info_request_encode(const get_device_info_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&get_device_info_request_desc, value, out, cap, length); }
wl_codec_status_t get_device_info_request_decode(const uint8_t *input, size_t length, get_device_info_request_t *out) { return wlc_decode(&get_device_info_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t get_device_info_request_wlc_detail_fingerprint(const get_device_info_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&get_device_info_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void get_device_info_response_clear(get_device_info_response_t *value) { if (value != NULL) wlc_clear(&get_device_info_response_desc, value); }
size_t get_device_info_response_encoded_size(const get_device_info_response_t *value) { size_t size; return wlc_measure(&get_device_info_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t get_device_info_response_encode(const get_device_info_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&get_device_info_response_desc, value, out, cap, length); }
wl_codec_status_t get_device_info_response_decode(const uint8_t *input, size_t length, get_device_info_response_t *out) { return wlc_decode(&get_device_info_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t get_device_info_response_wlc_detail_fingerprint(const get_device_info_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&get_device_info_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_device_info_request_clear(set_device_info_request_t *value) { if (value != NULL) wlc_clear(&set_device_info_request_desc, value); }
size_t set_device_info_request_encoded_size(const set_device_info_request_t *value) { size_t size; return wlc_measure(&set_device_info_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_device_info_request_encode(const set_device_info_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_device_info_request_desc, value, out, cap, length); }
wl_codec_status_t set_device_info_request_decode(const uint8_t *input, size_t length, set_device_info_request_t *out) { return wlc_decode(&set_device_info_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_device_info_request_wlc_detail_fingerprint(const set_device_info_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_device_info_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_device_info_response_clear(set_device_info_response_t *value) { if (value != NULL) wlc_clear(&set_device_info_response_desc, value); }
size_t set_device_info_response_encoded_size(const set_device_info_response_t *value) { size_t size; return wlc_measure(&set_device_info_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_device_info_response_encode(const set_device_info_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_device_info_response_desc, value, out, cap, length); }
wl_codec_status_t set_device_info_response_decode(const uint8_t *input, size_t length, set_device_info_response_t *out) { return wlc_decode(&set_device_info_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_device_info_response_wlc_detail_fingerprint(const set_device_info_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_device_info_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_arm_control_mode_request_clear(set_arm_control_mode_request_t *value) { if (value != NULL) wlc_clear(&set_arm_control_mode_request_desc, value); }
size_t set_arm_control_mode_request_encoded_size(const set_arm_control_mode_request_t *value) { size_t size; return wlc_measure(&set_arm_control_mode_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_arm_control_mode_request_encode(const set_arm_control_mode_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_arm_control_mode_request_desc, value, out, cap, length); }
wl_codec_status_t set_arm_control_mode_request_decode(const uint8_t *input, size_t length, set_arm_control_mode_request_t *out) { return wlc_decode(&set_arm_control_mode_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_arm_control_mode_request_wlc_detail_fingerprint(const set_arm_control_mode_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_arm_control_mode_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_arm_control_mode_response_clear(set_arm_control_mode_response_t *value) { if (value != NULL) wlc_clear(&set_arm_control_mode_response_desc, value); }
size_t set_arm_control_mode_response_encoded_size(const set_arm_control_mode_response_t *value) { size_t size; return wlc_measure(&set_arm_control_mode_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_arm_control_mode_response_encode(const set_arm_control_mode_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_arm_control_mode_response_desc, value, out, cap, length); }
wl_codec_status_t set_arm_control_mode_response_decode(const uint8_t *input, size_t length, set_arm_control_mode_response_t *out) { return wlc_decode(&set_arm_control_mode_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_arm_control_mode_response_wlc_detail_fingerprint(const set_arm_control_mode_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_arm_control_mode_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_gripper_control_mode_request_clear(set_gripper_control_mode_request_t *value) { if (value != NULL) wlc_clear(&set_gripper_control_mode_request_desc, value); }
size_t set_gripper_control_mode_request_encoded_size(const set_gripper_control_mode_request_t *value) { size_t size; return wlc_measure(&set_gripper_control_mode_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_gripper_control_mode_request_encode(const set_gripper_control_mode_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_gripper_control_mode_request_desc, value, out, cap, length); }
wl_codec_status_t set_gripper_control_mode_request_decode(const uint8_t *input, size_t length, set_gripper_control_mode_request_t *out) { return wlc_decode(&set_gripper_control_mode_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_gripper_control_mode_request_wlc_detail_fingerprint(const set_gripper_control_mode_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_gripper_control_mode_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_gripper_control_mode_response_clear(set_gripper_control_mode_response_t *value) { if (value != NULL) wlc_clear(&set_gripper_control_mode_response_desc, value); }
size_t set_gripper_control_mode_response_encoded_size(const set_gripper_control_mode_response_t *value) { size_t size; return wlc_measure(&set_gripper_control_mode_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_gripper_control_mode_response_encode(const set_gripper_control_mode_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_gripper_control_mode_response_desc, value, out, cap, length); }
wl_codec_status_t set_gripper_control_mode_response_decode(const uint8_t *input, size_t length, set_gripper_control_mode_response_t *out) { return wlc_decode(&set_gripper_control_mode_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_gripper_control_mode_response_wlc_detail_fingerprint(const set_gripper_control_mode_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_gripper_control_mode_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_register_read_request_clear(motor_register_read_request_t *value) { if (value != NULL) wlc_clear(&motor_register_read_request_desc, value); }
size_t motor_register_read_request_encoded_size(const motor_register_read_request_t *value) { size_t size; return wlc_measure(&motor_register_read_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_register_read_request_encode(const motor_register_read_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_register_read_request_desc, value, out, cap, length); }
wl_codec_status_t motor_register_read_request_decode(const uint8_t *input, size_t length, motor_register_read_request_t *out) { return wlc_decode(&motor_register_read_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_register_read_request_wlc_detail_fingerprint(const motor_register_read_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_register_read_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_register_read_response_clear(motor_register_read_response_t *value) { if (value != NULL) wlc_clear(&motor_register_read_response_desc, value); }
size_t motor_register_read_response_encoded_size(const motor_register_read_response_t *value) { size_t size; return wlc_measure(&motor_register_read_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_register_read_response_encode(const motor_register_read_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_register_read_response_desc, value, out, cap, length); }
wl_codec_status_t motor_register_read_response_decode(const uint8_t *input, size_t length, motor_register_read_response_t *out) { return wlc_decode(&motor_register_read_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_register_read_response_wlc_detail_fingerprint(const motor_register_read_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_register_read_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_register_write_request_clear(motor_register_write_request_t *value) { if (value != NULL) wlc_clear(&motor_register_write_request_desc, value); }
size_t motor_register_write_request_encoded_size(const motor_register_write_request_t *value) { size_t size; return wlc_measure(&motor_register_write_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_register_write_request_encode(const motor_register_write_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_register_write_request_desc, value, out, cap, length); }
wl_codec_status_t motor_register_write_request_decode(const uint8_t *input, size_t length, motor_register_write_request_t *out) { return wlc_decode(&motor_register_write_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_register_write_request_wlc_detail_fingerprint(const motor_register_write_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_register_write_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_register_write_response_clear(motor_register_write_response_t *value) { if (value != NULL) wlc_clear(&motor_register_write_response_desc, value); }
size_t motor_register_write_response_encoded_size(const motor_register_write_response_t *value) { size_t size; return wlc_measure(&motor_register_write_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_register_write_response_encode(const motor_register_write_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_register_write_response_desc, value, out, cap, length); }
wl_codec_status_t motor_register_write_response_decode(const uint8_t *input, size_t length, motor_register_write_response_t *out) { return wlc_decode(&motor_register_write_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_register_write_response_wlc_detail_fingerprint(const motor_register_write_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_register_write_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_store_parameters_request_clear(motor_store_parameters_request_t *value) { if (value != NULL) wlc_clear(&motor_store_parameters_request_desc, value); }
size_t motor_store_parameters_request_encoded_size(const motor_store_parameters_request_t *value) { size_t size; return wlc_measure(&motor_store_parameters_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_store_parameters_request_encode(const motor_store_parameters_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_store_parameters_request_desc, value, out, cap, length); }
wl_codec_status_t motor_store_parameters_request_decode(const uint8_t *input, size_t length, motor_store_parameters_request_t *out) { return wlc_decode(&motor_store_parameters_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_store_parameters_request_wlc_detail_fingerprint(const motor_store_parameters_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_store_parameters_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_store_parameters_response_clear(motor_store_parameters_response_t *value) { if (value != NULL) wlc_clear(&motor_store_parameters_response_desc, value); }
size_t motor_store_parameters_response_encoded_size(const motor_store_parameters_response_t *value) { size_t size; return wlc_measure(&motor_store_parameters_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_store_parameters_response_encode(const motor_store_parameters_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_store_parameters_response_desc, value, out, cap, length); }
wl_codec_status_t motor_store_parameters_response_decode(const uint8_t *input, size_t length, motor_store_parameters_response_t *out) { return wlc_decode(&motor_store_parameters_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_store_parameters_response_wlc_detail_fingerprint(const motor_store_parameters_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_store_parameters_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_set_zero_request_clear(motor_set_zero_request_t *value) { if (value != NULL) wlc_clear(&motor_set_zero_request_desc, value); }
size_t motor_set_zero_request_encoded_size(const motor_set_zero_request_t *value) { size_t size; return wlc_measure(&motor_set_zero_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_set_zero_request_encode(const motor_set_zero_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_set_zero_request_desc, value, out, cap, length); }
wl_codec_status_t motor_set_zero_request_decode(const uint8_t *input, size_t length, motor_set_zero_request_t *out) { return wlc_decode(&motor_set_zero_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_set_zero_request_wlc_detail_fingerprint(const motor_set_zero_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_set_zero_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void motor_set_zero_response_clear(motor_set_zero_response_t *value) { if (value != NULL) wlc_clear(&motor_set_zero_response_desc, value); }
size_t motor_set_zero_response_encoded_size(const motor_set_zero_response_t *value) { size_t size; return wlc_measure(&motor_set_zero_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t motor_set_zero_response_encode(const motor_set_zero_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&motor_set_zero_response_desc, value, out, cap, length); }
wl_codec_status_t motor_set_zero_response_decode(const uint8_t *input, size_t length, motor_set_zero_response_t *out) { return wlc_decode(&motor_set_zero_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t motor_set_zero_response_wlc_detail_fingerprint(const motor_set_zero_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&motor_set_zero_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_arm_mode_request_clear(set_arm_mode_request_t *value) { if (value != NULL) wlc_clear(&set_arm_mode_request_desc, value); }
size_t set_arm_mode_request_encoded_size(const set_arm_mode_request_t *value) { size_t size; return wlc_measure(&set_arm_mode_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_arm_mode_request_encode(const set_arm_mode_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_arm_mode_request_desc, value, out, cap, length); }
wl_codec_status_t set_arm_mode_request_decode(const uint8_t *input, size_t length, set_arm_mode_request_t *out) { return wlc_decode(&set_arm_mode_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_arm_mode_request_wlc_detail_fingerprint(const set_arm_mode_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_arm_mode_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_arm_mode_response_clear(set_arm_mode_response_t *value) { if (value != NULL) wlc_clear(&set_arm_mode_response_desc, value); }
size_t set_arm_mode_response_encoded_size(const set_arm_mode_response_t *value) { size_t size; return wlc_measure(&set_arm_mode_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_arm_mode_response_encode(const set_arm_mode_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_arm_mode_response_desc, value, out, cap, length); }
wl_codec_status_t set_arm_mode_response_decode(const uint8_t *input, size_t length, set_arm_mode_response_t *out) { return wlc_decode(&set_arm_mode_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_arm_mode_response_wlc_detail_fingerprint(const set_arm_mode_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_arm_mode_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void get_device_settings_request_clear(get_device_settings_request_t *value) { if (value != NULL) wlc_clear(&get_device_settings_request_desc, value); }
size_t get_device_settings_request_encoded_size(const get_device_settings_request_t *value) { size_t size; return wlc_measure(&get_device_settings_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t get_device_settings_request_encode(const get_device_settings_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&get_device_settings_request_desc, value, out, cap, length); }
wl_codec_status_t get_device_settings_request_decode(const uint8_t *input, size_t length, get_device_settings_request_t *out) { return wlc_decode(&get_device_settings_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t get_device_settings_request_wlc_detail_fingerprint(const get_device_settings_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&get_device_settings_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void get_device_settings_response_clear(get_device_settings_response_t *value) { if (value != NULL) wlc_clear(&get_device_settings_response_desc, value); }
size_t get_device_settings_response_encoded_size(const get_device_settings_response_t *value) { size_t size; return wlc_measure(&get_device_settings_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t get_device_settings_response_encode(const get_device_settings_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&get_device_settings_response_desc, value, out, cap, length); }
wl_codec_status_t get_device_settings_response_decode(const uint8_t *input, size_t length, get_device_settings_response_t *out) { return wlc_decode(&get_device_settings_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t get_device_settings_response_wlc_detail_fingerprint(const get_device_settings_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&get_device_settings_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_device_settings_request_clear(set_device_settings_request_t *value) { if (value != NULL) wlc_clear(&set_device_settings_request_desc, value); }
size_t set_device_settings_request_encoded_size(const set_device_settings_request_t *value) { size_t size; return wlc_measure(&set_device_settings_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_device_settings_request_encode(const set_device_settings_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_device_settings_request_desc, value, out, cap, length); }
wl_codec_status_t set_device_settings_request_decode(const uint8_t *input, size_t length, set_device_settings_request_t *out) { return wlc_decode(&set_device_settings_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_device_settings_request_wlc_detail_fingerprint(const set_device_settings_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_device_settings_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void set_device_settings_response_clear(set_device_settings_response_t *value) { if (value != NULL) wlc_clear(&set_device_settings_response_desc, value); }
size_t set_device_settings_response_encoded_size(const set_device_settings_response_t *value) { size_t size; return wlc_measure(&set_device_settings_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t set_device_settings_response_encode(const set_device_settings_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&set_device_settings_response_desc, value, out, cap, length); }
wl_codec_status_t set_device_settings_response_decode(const uint8_t *input, size_t length, set_device_settings_response_t *out) { return wlc_decode(&set_device_settings_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t set_device_settings_response_wlc_detail_fingerprint(const set_device_settings_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&set_device_settings_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void joint_mit_command_clear(joint_mit_command_t *value) { if (value != NULL) wlc_clear(&joint_mit_command_desc, value); }
size_t joint_mit_command_encoded_size(const joint_mit_command_t *value) { size_t size; return wlc_measure(&joint_mit_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t joint_mit_command_encode(const joint_mit_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&joint_mit_command_desc, value, out, cap, length); }
wl_codec_status_t joint_mit_command_decode(const uint8_t *input, size_t length, joint_mit_command_t *out) { return wlc_decode(&joint_mit_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t joint_mit_command_wlc_detail_fingerprint(const joint_mit_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&joint_mit_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void emergency_stop_request_clear(emergency_stop_request_t *value) { if (value != NULL) wlc_clear(&emergency_stop_request_desc, value); }
size_t emergency_stop_request_encoded_size(const emergency_stop_request_t *value) { size_t size; return wlc_measure(&emergency_stop_request_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t emergency_stop_request_encode(const emergency_stop_request_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&emergency_stop_request_desc, value, out, cap, length); }
wl_codec_status_t emergency_stop_request_decode(const uint8_t *input, size_t length, emergency_stop_request_t *out) { return wlc_decode(&emergency_stop_request_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t emergency_stop_request_wlc_detail_fingerprint(const emergency_stop_request_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&emergency_stop_request_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void emergency_stop_response_clear(emergency_stop_response_t *value) { if (value != NULL) wlc_clear(&emergency_stop_response_desc, value); }
size_t emergency_stop_response_encoded_size(const emergency_stop_response_t *value) { size_t size; return wlc_measure(&emergency_stop_response_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t emergency_stop_response_encode(const emergency_stop_response_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&emergency_stop_response_desc, value, out, cap, length); }
wl_codec_status_t emergency_stop_response_decode(const uint8_t *input, size_t length, emergency_stop_response_t *out) { return wlc_decode(&emergency_stop_response_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t emergency_stop_response_wlc_detail_fingerprint(const emergency_stop_response_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&emergency_stop_response_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void gripper_mit_command_clear(gripper_mit_command_t *value) { if (value != NULL) wlc_clear(&gripper_mit_command_desc, value); }
size_t gripper_mit_command_encoded_size(const gripper_mit_command_t *value) { size_t size; return wlc_measure(&gripper_mit_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t gripper_mit_command_encode(const gripper_mit_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&gripper_mit_command_desc, value, out, cap, length); }
wl_codec_status_t gripper_mit_command_decode(const uint8_t *input, size_t length, gripper_mit_command_t *out) { return wlc_decode(&gripper_mit_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t gripper_mit_command_wlc_detail_fingerprint(const gripper_mit_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&gripper_mit_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void joint_position_velocity_command_clear(joint_position_velocity_command_t *value) { if (value != NULL) wlc_clear(&joint_position_velocity_command_desc, value); }
size_t joint_position_velocity_command_encoded_size(const joint_position_velocity_command_t *value) { size_t size; return wlc_measure(&joint_position_velocity_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t joint_position_velocity_command_encode(const joint_position_velocity_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&joint_position_velocity_command_desc, value, out, cap, length); }
wl_codec_status_t joint_position_velocity_command_decode(const uint8_t *input, size_t length, joint_position_velocity_command_t *out) { return wlc_decode(&joint_position_velocity_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t joint_position_velocity_command_wlc_detail_fingerprint(const joint_position_velocity_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&joint_position_velocity_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void joint_velocity_command_clear(joint_velocity_command_t *value) { if (value != NULL) wlc_clear(&joint_velocity_command_desc, value); }
size_t joint_velocity_command_encoded_size(const joint_velocity_command_t *value) { size_t size; return wlc_measure(&joint_velocity_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t joint_velocity_command_encode(const joint_velocity_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&joint_velocity_command_desc, value, out, cap, length); }
wl_codec_status_t joint_velocity_command_decode(const uint8_t *input, size_t length, joint_velocity_command_t *out) { return wlc_decode(&joint_velocity_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t joint_velocity_command_wlc_detail_fingerprint(const joint_velocity_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&joint_velocity_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void joint_pvt_command_clear(joint_pvt_command_t *value) { if (value != NULL) wlc_clear(&joint_pvt_command_desc, value); }
size_t joint_pvt_command_encoded_size(const joint_pvt_command_t *value) { size_t size; return wlc_measure(&joint_pvt_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t joint_pvt_command_encode(const joint_pvt_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&joint_pvt_command_desc, value, out, cap, length); }
wl_codec_status_t joint_pvt_command_decode(const uint8_t *input, size_t length, joint_pvt_command_t *out) { return wlc_decode(&joint_pvt_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t joint_pvt_command_wlc_detail_fingerprint(const joint_pvt_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&joint_pvt_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void cartesian_pose_command_clear(cartesian_pose_command_t *value) { if (value != NULL) wlc_clear(&cartesian_pose_command_desc, value); }
size_t cartesian_pose_command_encoded_size(const cartesian_pose_command_t *value) { size_t size; return wlc_measure(&cartesian_pose_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t cartesian_pose_command_encode(const cartesian_pose_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&cartesian_pose_command_desc, value, out, cap, length); }
wl_codec_status_t cartesian_pose_command_decode(const uint8_t *input, size_t length, cartesian_pose_command_t *out) { return wlc_decode(&cartesian_pose_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t cartesian_pose_command_wlc_detail_fingerprint(const cartesian_pose_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&cartesian_pose_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void cartesian_velocity_command_clear(cartesian_velocity_command_t *value) { if (value != NULL) wlc_clear(&cartesian_velocity_command_desc, value); }
size_t cartesian_velocity_command_encoded_size(const cartesian_velocity_command_t *value) { size_t size; return wlc_measure(&cartesian_velocity_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t cartesian_velocity_command_encode(const cartesian_velocity_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&cartesian_velocity_command_desc, value, out, cap, length); }
wl_codec_status_t cartesian_velocity_command_decode(const uint8_t *input, size_t length, cartesian_velocity_command_t *out) { return wlc_decode(&cartesian_velocity_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t cartesian_velocity_command_wlc_detail_fingerprint(const cartesian_velocity_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&cartesian_velocity_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void gripper_position_velocity_command_clear(gripper_position_velocity_command_t *value) { if (value != NULL) wlc_clear(&gripper_position_velocity_command_desc, value); }
size_t gripper_position_velocity_command_encoded_size(const gripper_position_velocity_command_t *value) { size_t size; return wlc_measure(&gripper_position_velocity_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t gripper_position_velocity_command_encode(const gripper_position_velocity_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&gripper_position_velocity_command_desc, value, out, cap, length); }
wl_codec_status_t gripper_position_velocity_command_decode(const uint8_t *input, size_t length, gripper_position_velocity_command_t *out) { return wlc_decode(&gripper_position_velocity_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t gripper_position_velocity_command_wlc_detail_fingerprint(const gripper_position_velocity_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&gripper_position_velocity_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void gripper_velocity_command_clear(gripper_velocity_command_t *value) { if (value != NULL) wlc_clear(&gripper_velocity_command_desc, value); }
size_t gripper_velocity_command_encoded_size(const gripper_velocity_command_t *value) { size_t size; return wlc_measure(&gripper_velocity_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t gripper_velocity_command_encode(const gripper_velocity_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&gripper_velocity_command_desc, value, out, cap, length); }
wl_codec_status_t gripper_velocity_command_decode(const uint8_t *input, size_t length, gripper_velocity_command_t *out) { return wlc_decode(&gripper_velocity_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t gripper_velocity_command_wlc_detail_fingerprint(const gripper_velocity_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&gripper_velocity_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

void gripper_pvt_command_clear(gripper_pvt_command_t *value) { if (value != NULL) wlc_clear(&gripper_pvt_command_desc, value); }
size_t gripper_pvt_command_encoded_size(const gripper_pvt_command_t *value) { size_t size; return wlc_measure(&gripper_pvt_command_desc, value, &size) == WL_CODEC_OK ? size : SIZE_MAX; }
wl_codec_status_t gripper_pvt_command_encode(const gripper_pvt_command_t *value, uint8_t *out, size_t cap, size_t *length) { return wlc_encode(&gripper_pvt_command_desc, value, out, cap, length); }
wl_codec_status_t gripper_pvt_command_decode(const uint8_t *input, size_t length, gripper_pvt_command_t *out) { return wlc_decode(&gripper_pvt_command_desc, input, length, out); }

/* Generator-private: value must be the unmodified result of successful decode.
 * hash supplies the caller domain seed; codec has no RPC identity policy. */
wl_codec_status_t gripper_pvt_command_wlc_detail_fingerprint(const gripper_pvt_command_t *value, uint64_t *hash, size_t *length) {
  wlc_hash_state_t state = {*hash, 0U, WL_CODEC_OK};
  wl_codec_status_t status = wlc_hash_emit_fields(&gripper_pvt_command_desc, value, &state);
  if (status != WL_CODEC_OK) return status;
  if (state.status != WL_CODEC_OK) return state.status;
  *hash = state.hash;
  *length = state.length;
  return WL_CODEC_OK;
}

static void semantic_version_value_defaults(semantic_version_value_t *out) {
  (void)out;
  out->major = UINT32_C(0);
  out->minor = UINT32_C(0);
  out->patch = UINT32_C(0);
}

static void semantic_version_value_copy_fields(const semantic_version_t *view, semantic_version_value_t *out) {
  out->has_major = view->has_major;
  out->major = view->has_major ? view->major : UINT32_C(0);
  out->has_minor = view->has_minor;
  out->minor = view->has_minor ? view->minor : UINT32_C(0);
  out->has_patch = view->has_patch;
  out->patch = view->has_patch ? view->patch : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void semantic_version_wlc_detail_value_copy(const semantic_version_t *view, semantic_version_value_t *out) {
  memset(out, 0, sizeof(*out));
  semantic_version_value_copy_fields(view, out);
}

void semantic_version_value_clear(semantic_version_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  semantic_version_value_defaults(value);
}

wl_codec_status_t semantic_version_value_from_view(const semantic_version_t *view, semantic_version_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&semantic_version_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  semantic_version_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t semantic_version_value_borrow(const semantic_version_value_t *value, semantic_version_t *out) {
  semantic_version_clear(out);
  out->has_major = value->has_major;
  if (value->has_major) {
    out->major = value->major;
  }
  out->has_minor = value->has_minor;
  if (value->has_minor) {
    out->minor = value->minor;
  }
  out->has_patch = value->has_patch;
  if (value->has_patch) {
    out->patch = value->patch;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t semantic_version_value_to_view(const semantic_version_value_t *value, semantic_version_t *out) {
  semantic_version_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = semantic_version_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&semantic_version_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t semantic_version_value_encoded_size(const semantic_version_value_t *value) {
  semantic_version_t view;
  if (value == NULL || semantic_version_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return semantic_version_encoded_size(&view);
}

wl_codec_status_t semantic_version_value_encode(const semantic_version_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  semantic_version_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = semantic_version_value_borrow(value, &view);
  return status == WL_CODEC_OK ? semantic_version_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t semantic_version_value_decode(const uint8_t *input, size_t length, semantic_version_value_t *out) {
  semantic_version_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = semantic_version_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  semantic_version_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void device_info_value_defaults(device_info_value_t *out) {
  (void)out;
  semantic_version_value_defaults(&out->protocol_version);
  semantic_version_value_defaults(&out->firmware_version);
  out->firmware_type = 0;
  out->command_capabilities = UINT64_C(0);
}

static void device_info_value_copy_fields(const device_info_t *view, device_info_value_t *out) {
  out->has_protocol_version = view->has_protocol_version;
  if (view->has_protocol_version) semantic_version_value_copy_fields(&view->protocol_version, &out->protocol_version);
  else semantic_version_value_defaults(&out->protocol_version);
  out->has_firmware_version = view->has_firmware_version;
  if (view->has_firmware_version) semantic_version_value_copy_fields(&view->firmware_version, &out->firmware_version);
  else semantic_version_value_defaults(&out->firmware_version);
  out->has_board_name = view->has_board_name;
  if (view->has_board_name) {
    out->board_name.length = view->board_name.length;
    if (view->board_name.length != 0U) memcpy(out->board_name.data, view->board_name.data, view->board_name.length);
  } else {
  }
  out->has_custom_name = view->has_custom_name;
  if (view->has_custom_name) {
    out->custom_name.length = view->custom_name.length;
    if (view->custom_name.length != 0U) memcpy(out->custom_name.data, view->custom_name.data, view->custom_name.length);
  } else {
  }
  out->has_firmware_type = view->has_firmware_type;
  out->firmware_type = view->has_firmware_type ? view->firmware_type : 0;
  out->has_serial = view->has_serial;
  if (view->has_serial) {
    out->serial.length = view->serial.length;
    if (view->serial.length != 0U) memcpy(out->serial.data, view->serial.data, view->serial.length);
  } else {
  }
  out->has_command_capabilities = view->has_command_capabilities;
  out->command_capabilities = view->has_command_capabilities ? view->command_capabilities : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void device_info_wlc_detail_value_copy(const device_info_t *view, device_info_value_t *out) {
  memset(out, 0, sizeof(*out));
  device_info_value_copy_fields(view, out);
}

void device_info_value_clear(device_info_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  device_info_value_defaults(value);
}

wl_codec_status_t device_info_value_from_view(const device_info_t *view, device_info_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&device_info_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  device_info_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t device_info_value_borrow(const device_info_value_t *value, device_info_t *out) {
  device_info_clear(out);
  out->has_protocol_version = value->has_protocol_version;
  if (value->has_protocol_version) {
    wl_codec_status_t status = semantic_version_value_borrow(&value->protocol_version, &out->protocol_version);
    if (status != WL_CODEC_OK) return status;
  }
  out->has_firmware_version = value->has_firmware_version;
  if (value->has_firmware_version) {
    wl_codec_status_t status = semantic_version_value_borrow(&value->firmware_version, &out->firmware_version);
    if (status != WL_CODEC_OK) return status;
  }
  out->has_board_name = value->has_board_name;
  if (value->has_board_name) {
    if (value->board_name.length > 31U) return WL_CODEC_ERR_INVALID_VALUE;
    out->board_name.length = value->board_name.length;
    out->board_name.data = value->board_name.data;
  }
  out->has_custom_name = value->has_custom_name;
  if (value->has_custom_name) {
    if (value->custom_name.length > 31U) return WL_CODEC_ERR_INVALID_VALUE;
    out->custom_name.length = value->custom_name.length;
    out->custom_name.data = value->custom_name.data;
  }
  out->has_firmware_type = value->has_firmware_type;
  if (value->has_firmware_type) {
    out->firmware_type = value->firmware_type;
  }
  out->has_serial = value->has_serial;
  if (value->has_serial) {
    if (value->serial.length > 31U) return WL_CODEC_ERR_INVALID_VALUE;
    out->serial.length = value->serial.length;
    out->serial.data = value->serial.data;
  }
  out->has_command_capabilities = value->has_command_capabilities;
  if (value->has_command_capabilities) {
    out->command_capabilities = value->command_capabilities;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t device_info_value_to_view(const device_info_value_t *value, device_info_t *out) {
  device_info_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = device_info_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&device_info_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t device_info_value_encoded_size(const device_info_value_t *value) {
  device_info_t view;
  if (value == NULL || device_info_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return device_info_encoded_size(&view);
}

wl_codec_status_t device_info_value_encode(const device_info_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  device_info_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = device_info_value_borrow(value, &view);
  return status == WL_CODEC_OK ? device_info_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t device_info_value_decode(const uint8_t *input, size_t length, device_info_value_t *out) {
  device_info_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = device_info_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  device_info_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void device_settings_value_defaults(device_settings_value_t *out) {
  (void)out;
  out->firmware_dt_us = UINT32_C(0);
}

static void device_settings_value_copy_fields(const device_settings_t *view, device_settings_value_t *out) {
  out->has_firmware_dt_us = view->has_firmware_dt_us;
  out->firmware_dt_us = view->has_firmware_dt_us ? view->firmware_dt_us : UINT32_C(0);
  out->has_gravity_scale = view->has_gravity_scale;
  if (view->has_gravity_scale) memcpy(out->gravity_scale, view->gravity_scale, sizeof(out->gravity_scale));
  out->has_torque_continuous = view->has_torque_continuous;
  if (view->has_torque_continuous) memcpy(out->torque_continuous, view->torque_continuous, sizeof(out->torque_continuous));
  out->has_torque_peak = view->has_torque_peak;
  if (view->has_torque_peak) memcpy(out->torque_peak, view->torque_peak, sizeof(out->torque_peak));
  out->has_thermal_capacity = view->has_thermal_capacity;
  if (view->has_thermal_capacity) memcpy(out->thermal_capacity, view->thermal_capacity, sizeof(out->thermal_capacity));
  out->has_torque_ramp_rate = view->has_torque_ramp_rate;
  if (view->has_torque_ramp_rate) memcpy(out->torque_ramp_rate, view->torque_ramp_rate, sizeof(out->torque_ramp_rate));
  out->has_joint_limit_min = view->has_joint_limit_min;
  if (view->has_joint_limit_min) memcpy(out->joint_limit_min, view->joint_limit_min, sizeof(out->joint_limit_min));
  out->has_joint_limit_max = view->has_joint_limit_max;
  if (view->has_joint_limit_max) memcpy(out->joint_limit_max, view->joint_limit_max, sizeof(out->joint_limit_max));
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void device_settings_wlc_detail_value_copy(const device_settings_t *view, device_settings_value_t *out) {
  memset(out, 0, sizeof(*out));
  device_settings_value_copy_fields(view, out);
}

void device_settings_value_clear(device_settings_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  device_settings_value_defaults(value);
}

wl_codec_status_t device_settings_value_from_view(const device_settings_t *view, device_settings_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&device_settings_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  device_settings_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t device_settings_value_borrow(const device_settings_value_t *value, device_settings_t *out) {
  device_settings_clear(out);
  out->has_firmware_dt_us = value->has_firmware_dt_us;
  if (value->has_firmware_dt_us) {
    out->firmware_dt_us = value->firmware_dt_us;
  }
  out->has_gravity_scale = value->has_gravity_scale;
  if (value->has_gravity_scale) {
    memcpy(out->gravity_scale, value->gravity_scale, sizeof(out->gravity_scale));
  }
  out->has_torque_continuous = value->has_torque_continuous;
  if (value->has_torque_continuous) {
    memcpy(out->torque_continuous, value->torque_continuous, sizeof(out->torque_continuous));
  }
  out->has_torque_peak = value->has_torque_peak;
  if (value->has_torque_peak) {
    memcpy(out->torque_peak, value->torque_peak, sizeof(out->torque_peak));
  }
  out->has_thermal_capacity = value->has_thermal_capacity;
  if (value->has_thermal_capacity) {
    memcpy(out->thermal_capacity, value->thermal_capacity, sizeof(out->thermal_capacity));
  }
  out->has_torque_ramp_rate = value->has_torque_ramp_rate;
  if (value->has_torque_ramp_rate) {
    memcpy(out->torque_ramp_rate, value->torque_ramp_rate, sizeof(out->torque_ramp_rate));
  }
  out->has_joint_limit_min = value->has_joint_limit_min;
  if (value->has_joint_limit_min) {
    memcpy(out->joint_limit_min, value->joint_limit_min, sizeof(out->joint_limit_min));
  }
  out->has_joint_limit_max = value->has_joint_limit_max;
  if (value->has_joint_limit_max) {
    memcpy(out->joint_limit_max, value->joint_limit_max, sizeof(out->joint_limit_max));
  }
  return WL_CODEC_OK;
}

wl_codec_status_t device_settings_value_to_view(const device_settings_value_t *value, device_settings_t *out) {
  device_settings_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = device_settings_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&device_settings_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t device_settings_value_encoded_size(const device_settings_value_t *value) {
  device_settings_t view;
  if (value == NULL || device_settings_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return device_settings_encoded_size(&view);
}

wl_codec_status_t device_settings_value_encode(const device_settings_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  device_settings_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = device_settings_value_borrow(value, &view);
  return status == WL_CODEC_OK ? device_settings_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t device_settings_value_decode(const uint8_t *input, size_t length, device_settings_value_t *out) {
  device_settings_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = device_settings_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  device_settings_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void arm_status_value_defaults(arm_status_value_t *out) {
  (void)out;
  out->mode = 0;
  out->sequence = UINT32_C(0);
  out->timestamp_us = UINT64_C(0);
  out->gripper_position = 0;
  out->gripper_velocity = 0;
  out->gripper_torque = 0;
  out->error_flags = UINT32_C(0);
  out->last_sdk_timestamp_us = UINT64_C(0);
}

static void arm_status_value_copy_fields(const arm_status_t *view, arm_status_value_t *out) {
  out->has_mode = view->has_mode;
  out->mode = view->has_mode ? view->mode : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_timestamp_us = view->has_timestamp_us;
  out->timestamp_us = view->has_timestamp_us ? view->timestamp_us : UINT64_C(0);
  out->has_joint_position = view->has_joint_position;
  if (view->has_joint_position) memcpy(out->joint_position, view->joint_position, sizeof(out->joint_position));
  out->has_joint_velocity = view->has_joint_velocity;
  if (view->has_joint_velocity) memcpy(out->joint_velocity, view->joint_velocity, sizeof(out->joint_velocity));
  out->has_joint_torque = view->has_joint_torque;
  if (view->has_joint_torque) memcpy(out->joint_torque, view->joint_torque, sizeof(out->joint_torque));
  out->has_base_gravity = view->has_base_gravity;
  if (view->has_base_gravity) memcpy(out->base_gravity, view->base_gravity, sizeof(out->base_gravity));
  out->has_gripper_position = view->has_gripper_position;
  out->gripper_position = view->has_gripper_position ? view->gripper_position : 0;
  out->has_gripper_velocity = view->has_gripper_velocity;
  out->gripper_velocity = view->has_gripper_velocity ? view->gripper_velocity : 0;
  out->has_gripper_torque = view->has_gripper_torque;
  out->gripper_torque = view->has_gripper_torque ? view->gripper_torque : 0;
  out->has_end_effector_transform = view->has_end_effector_transform;
  if (view->has_end_effector_transform) memcpy(out->end_effector_transform, view->end_effector_transform, sizeof(out->end_effector_transform));
  out->has_external_wrench = view->has_external_wrench;
  if (view->has_external_wrench) memcpy(out->external_wrench, view->external_wrench, sizeof(out->external_wrench));
  out->has_error_flags = view->has_error_flags;
  out->error_flags = view->has_error_flags ? view->error_flags : UINT32_C(0);
  out->has_last_sdk_timestamp_us = view->has_last_sdk_timestamp_us;
  out->last_sdk_timestamp_us = view->has_last_sdk_timestamp_us ? view->last_sdk_timestamp_us : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void arm_status_wlc_detail_value_copy(const arm_status_t *view, arm_status_value_t *out) {
  memset(out, 0, sizeof(*out));
  arm_status_value_copy_fields(view, out);
}

void arm_status_value_clear(arm_status_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  arm_status_value_defaults(value);
}

wl_codec_status_t arm_status_value_from_view(const arm_status_t *view, arm_status_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&arm_status_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  arm_status_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t arm_status_value_borrow(const arm_status_value_t *value, arm_status_t *out) {
  arm_status_clear(out);
  out->has_mode = value->has_mode;
  if (value->has_mode) {
    out->mode = value->mode;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_timestamp_us = value->has_timestamp_us;
  if (value->has_timestamp_us) {
    out->timestamp_us = value->timestamp_us;
  }
  out->has_joint_position = value->has_joint_position;
  if (value->has_joint_position) {
    memcpy(out->joint_position, value->joint_position, sizeof(out->joint_position));
  }
  out->has_joint_velocity = value->has_joint_velocity;
  if (value->has_joint_velocity) {
    memcpy(out->joint_velocity, value->joint_velocity, sizeof(out->joint_velocity));
  }
  out->has_joint_torque = value->has_joint_torque;
  if (value->has_joint_torque) {
    memcpy(out->joint_torque, value->joint_torque, sizeof(out->joint_torque));
  }
  out->has_base_gravity = value->has_base_gravity;
  if (value->has_base_gravity) {
    memcpy(out->base_gravity, value->base_gravity, sizeof(out->base_gravity));
  }
  out->has_gripper_position = value->has_gripper_position;
  if (value->has_gripper_position) {
    out->gripper_position = value->gripper_position;
  }
  out->has_gripper_velocity = value->has_gripper_velocity;
  if (value->has_gripper_velocity) {
    out->gripper_velocity = value->gripper_velocity;
  }
  out->has_gripper_torque = value->has_gripper_torque;
  if (value->has_gripper_torque) {
    out->gripper_torque = value->gripper_torque;
  }
  out->has_end_effector_transform = value->has_end_effector_transform;
  if (value->has_end_effector_transform) {
    memcpy(out->end_effector_transform, value->end_effector_transform, sizeof(out->end_effector_transform));
  }
  out->has_external_wrench = value->has_external_wrench;
  if (value->has_external_wrench) {
    memcpy(out->external_wrench, value->external_wrench, sizeof(out->external_wrench));
  }
  out->has_error_flags = value->has_error_flags;
  if (value->has_error_flags) {
    out->error_flags = value->error_flags;
  }
  out->has_last_sdk_timestamp_us = value->has_last_sdk_timestamp_us;
  if (value->has_last_sdk_timestamp_us) {
    out->last_sdk_timestamp_us = value->last_sdk_timestamp_us;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t arm_status_value_to_view(const arm_status_value_t *value, arm_status_t *out) {
  arm_status_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = arm_status_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&arm_status_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t arm_status_value_encoded_size(const arm_status_value_t *value) {
  arm_status_t view;
  if (value == NULL || arm_status_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return arm_status_encoded_size(&view);
}

wl_codec_status_t arm_status_value_encode(const arm_status_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  arm_status_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = arm_status_value_borrow(value, &view);
  return status == WL_CODEC_OK ? arm_status_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t arm_status_value_decode(const uint8_t *input, size_t length, arm_status_value_t *out) {
  arm_status_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = arm_status_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  arm_status_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_feedback_value_defaults(motor_feedback_value_t *out) {
  (void)out;
  out->device_status_bits = UINT32_C(0);
  out->enabled_mask = 0;
}

static void motor_feedback_value_copy_fields(const motor_feedback_t *view, motor_feedback_value_t *out) {
  out->has_position_rad = view->has_position_rad;
  if (view->has_position_rad) memcpy(out->position_rad, view->position_rad, sizeof(out->position_rad));
  out->has_velocity_rad_s = view->has_velocity_rad_s;
  if (view->has_velocity_rad_s) memcpy(out->velocity_rad_s, view->velocity_rad_s, sizeof(out->velocity_rad_s));
  out->has_torque_nm = view->has_torque_nm;
  if (view->has_torque_nm) memcpy(out->torque_nm, view->torque_nm, sizeof(out->torque_nm));
  out->has_temperature_c = view->has_temperature_c;
  if (view->has_temperature_c) memcpy(out->temperature_c, view->temperature_c, sizeof(out->temperature_c));
  out->has_device_status_bits = view->has_device_status_bits;
  out->device_status_bits = view->has_device_status_bits ? view->device_status_bits : UINT32_C(0);
  out->has_enabled_mask = view->has_enabled_mask;
  out->enabled_mask = view->has_enabled_mask ? view->enabled_mask : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_feedback_wlc_detail_value_copy(const motor_feedback_t *view, motor_feedback_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_feedback_value_copy_fields(view, out);
}

void motor_feedback_value_clear(motor_feedback_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_feedback_value_defaults(value);
}

wl_codec_status_t motor_feedback_value_from_view(const motor_feedback_t *view, motor_feedback_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_feedback_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_feedback_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_feedback_value_borrow(const motor_feedback_value_t *value, motor_feedback_t *out) {
  motor_feedback_clear(out);
  out->has_position_rad = value->has_position_rad;
  if (value->has_position_rad) {
    memcpy(out->position_rad, value->position_rad, sizeof(out->position_rad));
  }
  out->has_velocity_rad_s = value->has_velocity_rad_s;
  if (value->has_velocity_rad_s) {
    memcpy(out->velocity_rad_s, value->velocity_rad_s, sizeof(out->velocity_rad_s));
  }
  out->has_torque_nm = value->has_torque_nm;
  if (value->has_torque_nm) {
    memcpy(out->torque_nm, value->torque_nm, sizeof(out->torque_nm));
  }
  out->has_temperature_c = value->has_temperature_c;
  if (value->has_temperature_c) {
    memcpy(out->temperature_c, value->temperature_c, sizeof(out->temperature_c));
  }
  out->has_device_status_bits = value->has_device_status_bits;
  if (value->has_device_status_bits) {
    out->device_status_bits = value->device_status_bits;
  }
  out->has_enabled_mask = value->has_enabled_mask;
  if (value->has_enabled_mask) {
    out->enabled_mask = value->enabled_mask;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_feedback_value_to_view(const motor_feedback_value_t *value, motor_feedback_t *out) {
  motor_feedback_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_feedback_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_feedback_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_feedback_value_encoded_size(const motor_feedback_value_t *value) {
  motor_feedback_t view;
  if (value == NULL || motor_feedback_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_feedback_encoded_size(&view);
}

wl_codec_status_t motor_feedback_value_encode(const motor_feedback_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_feedback_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_feedback_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_feedback_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_feedback_value_decode(const uint8_t *input, size_t length, motor_feedback_value_t *out) {
  motor_feedback_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_feedback_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_feedback_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void arm_diagnostics_value_defaults(arm_diagnostics_value_t *out) {
  (void)out;
  out->uptime_s = UINT32_C(0);
  out->tick_count = UINT32_C(0);
  out->mode_entry_ms = UINT32_C(0);
  out->bus_healthy = 0;
  out->bus_state = 0;
  out->tx_error_count = 0;
  out->rx_error_count = 0;
  out->joint_healthy_mask = 0;
  out->gripper_healthy = 0;
  out->gripper_temperature_c = 0;
  out->overheat_mask = 0;
}

static void arm_diagnostics_value_copy_fields(const arm_diagnostics_t *view, arm_diagnostics_value_t *out) {
  out->has_uptime_s = view->has_uptime_s;
  out->uptime_s = view->has_uptime_s ? view->uptime_s : UINT32_C(0);
  out->has_tick_count = view->has_tick_count;
  out->tick_count = view->has_tick_count ? view->tick_count : UINT32_C(0);
  out->has_mode_entry_ms = view->has_mode_entry_ms;
  out->mode_entry_ms = view->has_mode_entry_ms ? view->mode_entry_ms : UINT32_C(0);
  out->has_bus_healthy = view->has_bus_healthy;
  out->bus_healthy = view->has_bus_healthy ? view->bus_healthy : 0;
  out->has_bus_state = view->has_bus_state;
  out->bus_state = view->has_bus_state ? view->bus_state : 0;
  out->has_tx_error_count = view->has_tx_error_count;
  out->tx_error_count = view->has_tx_error_count ? view->tx_error_count : 0;
  out->has_rx_error_count = view->has_rx_error_count;
  out->rx_error_count = view->has_rx_error_count ? view->rx_error_count : 0;
  out->has_joint_healthy_mask = view->has_joint_healthy_mask;
  out->joint_healthy_mask = view->has_joint_healthy_mask ? view->joint_healthy_mask : 0;
  out->has_joint_temperature_c = view->has_joint_temperature_c;
  if (view->has_joint_temperature_c) memcpy(out->joint_temperature_c, view->joint_temperature_c, sizeof(out->joint_temperature_c));
  out->has_gripper_healthy = view->has_gripper_healthy;
  out->gripper_healthy = view->has_gripper_healthy ? view->gripper_healthy : 0;
  out->has_gripper_temperature_c = view->has_gripper_temperature_c;
  out->gripper_temperature_c = view->has_gripper_temperature_c ? view->gripper_temperature_c : 0;
  out->has_overheat_mask = view->has_overheat_mask;
  out->overheat_mask = view->has_overheat_mask ? view->overheat_mask : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void arm_diagnostics_wlc_detail_value_copy(const arm_diagnostics_t *view, arm_diagnostics_value_t *out) {
  memset(out, 0, sizeof(*out));
  arm_diagnostics_value_copy_fields(view, out);
}

void arm_diagnostics_value_clear(arm_diagnostics_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  arm_diagnostics_value_defaults(value);
}

wl_codec_status_t arm_diagnostics_value_from_view(const arm_diagnostics_t *view, arm_diagnostics_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&arm_diagnostics_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  arm_diagnostics_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t arm_diagnostics_value_borrow(const arm_diagnostics_value_t *value, arm_diagnostics_t *out) {
  arm_diagnostics_clear(out);
  out->has_uptime_s = value->has_uptime_s;
  if (value->has_uptime_s) {
    out->uptime_s = value->uptime_s;
  }
  out->has_tick_count = value->has_tick_count;
  if (value->has_tick_count) {
    out->tick_count = value->tick_count;
  }
  out->has_mode_entry_ms = value->has_mode_entry_ms;
  if (value->has_mode_entry_ms) {
    out->mode_entry_ms = value->mode_entry_ms;
  }
  out->has_bus_healthy = value->has_bus_healthy;
  if (value->has_bus_healthy) {
    out->bus_healthy = value->bus_healthy;
  }
  out->has_bus_state = value->has_bus_state;
  if (value->has_bus_state) {
    out->bus_state = value->bus_state;
  }
  out->has_tx_error_count = value->has_tx_error_count;
  if (value->has_tx_error_count) {
    out->tx_error_count = value->tx_error_count;
  }
  out->has_rx_error_count = value->has_rx_error_count;
  if (value->has_rx_error_count) {
    out->rx_error_count = value->rx_error_count;
  }
  out->has_joint_healthy_mask = value->has_joint_healthy_mask;
  if (value->has_joint_healthy_mask) {
    out->joint_healthy_mask = value->joint_healthy_mask;
  }
  out->has_joint_temperature_c = value->has_joint_temperature_c;
  if (value->has_joint_temperature_c) {
    memcpy(out->joint_temperature_c, value->joint_temperature_c, sizeof(out->joint_temperature_c));
  }
  out->has_gripper_healthy = value->has_gripper_healthy;
  if (value->has_gripper_healthy) {
    out->gripper_healthy = value->gripper_healthy;
  }
  out->has_gripper_temperature_c = value->has_gripper_temperature_c;
  if (value->has_gripper_temperature_c) {
    out->gripper_temperature_c = value->gripper_temperature_c;
  }
  out->has_overheat_mask = value->has_overheat_mask;
  if (value->has_overheat_mask) {
    out->overheat_mask = value->overheat_mask;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t arm_diagnostics_value_to_view(const arm_diagnostics_value_t *value, arm_diagnostics_t *out) {
  arm_diagnostics_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = arm_diagnostics_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&arm_diagnostics_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t arm_diagnostics_value_encoded_size(const arm_diagnostics_value_t *value) {
  arm_diagnostics_t view;
  if (value == NULL || arm_diagnostics_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return arm_diagnostics_encoded_size(&view);
}

wl_codec_status_t arm_diagnostics_value_encode(const arm_diagnostics_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  arm_diagnostics_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = arm_diagnostics_value_borrow(value, &view);
  return status == WL_CODEC_OK ? arm_diagnostics_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t arm_diagnostics_value_decode(const uint8_t *input, size_t length, arm_diagnostics_value_t *out) {
  arm_diagnostics_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = arm_diagnostics_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  arm_diagnostics_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_zero_request_value_defaults(set_zero_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->joint_id = 0;
}

static void set_zero_request_value_copy_fields(const set_zero_request_t *view, set_zero_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_zero_request_wlc_detail_value_copy(const set_zero_request_t *view, set_zero_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_zero_request_value_copy_fields(view, out);
}

void set_zero_request_value_clear(set_zero_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_zero_request_value_defaults(value);
}

wl_codec_status_t set_zero_request_value_from_view(const set_zero_request_t *view, set_zero_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_zero_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_zero_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_zero_request_value_borrow(const set_zero_request_value_t *value, set_zero_request_t *out) {
  set_zero_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_zero_request_value_to_view(const set_zero_request_value_t *value, set_zero_request_t *out) {
  set_zero_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_zero_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_zero_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_zero_request_value_encoded_size(const set_zero_request_value_t *value) {
  set_zero_request_t view;
  if (value == NULL || set_zero_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_zero_request_encoded_size(&view);
}

wl_codec_status_t set_zero_request_value_encode(const set_zero_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_zero_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_zero_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_zero_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_zero_request_value_decode(const uint8_t *input, size_t length, set_zero_request_value_t *out) {
  set_zero_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_zero_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_zero_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_zero_response_value_defaults(set_zero_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void set_zero_response_value_copy_fields(const set_zero_response_t *view, set_zero_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_zero_response_wlc_detail_value_copy(const set_zero_response_t *view, set_zero_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_zero_response_value_copy_fields(view, out);
}

void set_zero_response_value_clear(set_zero_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_zero_response_value_defaults(value);
}

wl_codec_status_t set_zero_response_value_from_view(const set_zero_response_t *view, set_zero_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_zero_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_zero_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_zero_response_value_borrow(const set_zero_response_value_t *value, set_zero_response_t *out) {
  set_zero_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_zero_response_value_to_view(const set_zero_response_value_t *value, set_zero_response_t *out) {
  set_zero_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_zero_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_zero_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_zero_response_value_encoded_size(const set_zero_response_value_t *value) {
  set_zero_response_t view;
  if (value == NULL || set_zero_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_zero_response_encoded_size(&view);
}

wl_codec_status_t set_zero_response_value_encode(const set_zero_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_zero_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_zero_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_zero_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_zero_response_value_decode(const uint8_t *input, size_t length, set_zero_response_value_t *out) {
  set_zero_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_zero_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_zero_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void clear_error_request_value_defaults(clear_error_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->joint_id = 0;
}

static void clear_error_request_value_copy_fields(const clear_error_request_t *view, clear_error_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void clear_error_request_wlc_detail_value_copy(const clear_error_request_t *view, clear_error_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  clear_error_request_value_copy_fields(view, out);
}

void clear_error_request_value_clear(clear_error_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  clear_error_request_value_defaults(value);
}

wl_codec_status_t clear_error_request_value_from_view(const clear_error_request_t *view, clear_error_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&clear_error_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  clear_error_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t clear_error_request_value_borrow(const clear_error_request_value_t *value, clear_error_request_t *out) {
  clear_error_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t clear_error_request_value_to_view(const clear_error_request_value_t *value, clear_error_request_t *out) {
  clear_error_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_error_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&clear_error_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t clear_error_request_value_encoded_size(const clear_error_request_value_t *value) {
  clear_error_request_t view;
  if (value == NULL || clear_error_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return clear_error_request_encoded_size(&view);
}

wl_codec_status_t clear_error_request_value_encode(const clear_error_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  clear_error_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_error_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? clear_error_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t clear_error_request_value_decode(const uint8_t *input, size_t length, clear_error_request_value_t *out) {
  clear_error_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_error_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  clear_error_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void clear_error_response_value_defaults(clear_error_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void clear_error_response_value_copy_fields(const clear_error_response_t *view, clear_error_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void clear_error_response_wlc_detail_value_copy(const clear_error_response_t *view, clear_error_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  clear_error_response_value_copy_fields(view, out);
}

void clear_error_response_value_clear(clear_error_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  clear_error_response_value_defaults(value);
}

wl_codec_status_t clear_error_response_value_from_view(const clear_error_response_t *view, clear_error_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&clear_error_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  clear_error_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t clear_error_response_value_borrow(const clear_error_response_value_t *value, clear_error_response_t *out) {
  clear_error_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t clear_error_response_value_to_view(const clear_error_response_value_t *value, clear_error_response_t *out) {
  clear_error_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_error_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&clear_error_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t clear_error_response_value_encoded_size(const clear_error_response_value_t *value) {
  clear_error_response_t view;
  if (value == NULL || clear_error_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return clear_error_response_encoded_size(&view);
}

wl_codec_status_t clear_error_response_value_encode(const clear_error_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  clear_error_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_error_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? clear_error_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t clear_error_response_value_decode(const uint8_t *input, size_t length, clear_error_response_value_t *out) {
  clear_error_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_error_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  clear_error_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void home_request_value_defaults(home_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void home_request_value_copy_fields(const home_request_t *view, home_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void home_request_wlc_detail_value_copy(const home_request_t *view, home_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  home_request_value_copy_fields(view, out);
}

void home_request_value_clear(home_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  home_request_value_defaults(value);
}

wl_codec_status_t home_request_value_from_view(const home_request_t *view, home_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&home_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  home_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t home_request_value_borrow(const home_request_value_t *value, home_request_t *out) {
  home_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t home_request_value_to_view(const home_request_value_t *value, home_request_t *out) {
  home_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = home_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&home_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t home_request_value_encoded_size(const home_request_value_t *value) {
  home_request_t view;
  if (value == NULL || home_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return home_request_encoded_size(&view);
}

wl_codec_status_t home_request_value_encode(const home_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  home_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = home_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? home_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t home_request_value_decode(const uint8_t *input, size_t length, home_request_value_t *out) {
  home_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = home_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  home_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void home_response_value_defaults(home_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void home_response_value_copy_fields(const home_response_t *view, home_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void home_response_wlc_detail_value_copy(const home_response_t *view, home_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  home_response_value_copy_fields(view, out);
}

void home_response_value_clear(home_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  home_response_value_defaults(value);
}

wl_codec_status_t home_response_value_from_view(const home_response_t *view, home_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&home_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  home_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t home_response_value_borrow(const home_response_value_t *value, home_response_t *out) {
  home_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t home_response_value_to_view(const home_response_value_t *value, home_response_t *out) {
  home_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = home_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&home_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t home_response_value_encoded_size(const home_response_value_t *value) {
  home_response_t view;
  if (value == NULL || home_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return home_response_encoded_size(&view);
}

wl_codec_status_t home_response_value_encode(const home_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  home_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = home_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? home_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t home_response_value_decode(const uint8_t *input, size_t length, home_response_value_t *out) {
  home_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = home_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  home_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void clear_faults_request_value_defaults(clear_faults_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void clear_faults_request_value_copy_fields(const clear_faults_request_t *view, clear_faults_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void clear_faults_request_wlc_detail_value_copy(const clear_faults_request_t *view, clear_faults_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  clear_faults_request_value_copy_fields(view, out);
}

void clear_faults_request_value_clear(clear_faults_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  clear_faults_request_value_defaults(value);
}

wl_codec_status_t clear_faults_request_value_from_view(const clear_faults_request_t *view, clear_faults_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&clear_faults_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  clear_faults_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t clear_faults_request_value_borrow(const clear_faults_request_value_t *value, clear_faults_request_t *out) {
  clear_faults_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t clear_faults_request_value_to_view(const clear_faults_request_value_t *value, clear_faults_request_t *out) {
  clear_faults_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_faults_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&clear_faults_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t clear_faults_request_value_encoded_size(const clear_faults_request_value_t *value) {
  clear_faults_request_t view;
  if (value == NULL || clear_faults_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return clear_faults_request_encoded_size(&view);
}

wl_codec_status_t clear_faults_request_value_encode(const clear_faults_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  clear_faults_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_faults_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? clear_faults_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t clear_faults_request_value_decode(const uint8_t *input, size_t length, clear_faults_request_value_t *out) {
  clear_faults_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_faults_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  clear_faults_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void clear_faults_response_value_defaults(clear_faults_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void clear_faults_response_value_copy_fields(const clear_faults_response_t *view, clear_faults_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void clear_faults_response_wlc_detail_value_copy(const clear_faults_response_t *view, clear_faults_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  clear_faults_response_value_copy_fields(view, out);
}

void clear_faults_response_value_clear(clear_faults_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  clear_faults_response_value_defaults(value);
}

wl_codec_status_t clear_faults_response_value_from_view(const clear_faults_response_t *view, clear_faults_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&clear_faults_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  clear_faults_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t clear_faults_response_value_borrow(const clear_faults_response_value_t *value, clear_faults_response_t *out) {
  clear_faults_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t clear_faults_response_value_to_view(const clear_faults_response_value_t *value, clear_faults_response_t *out) {
  clear_faults_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_faults_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&clear_faults_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t clear_faults_response_value_encoded_size(const clear_faults_response_value_t *value) {
  clear_faults_response_t view;
  if (value == NULL || clear_faults_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return clear_faults_response_encoded_size(&view);
}

wl_codec_status_t clear_faults_response_value_encode(const clear_faults_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  clear_faults_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_faults_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? clear_faults_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t clear_faults_response_value_decode(const uint8_t *input, size_t length, clear_faults_response_value_t *out) {
  clear_faults_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = clear_faults_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  clear_faults_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void acquire_control_lease_request_value_defaults(acquire_control_lease_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->requested_timeout_ms = UINT32_C(0);
  out->current_token = UINT64_C(0);
}

static void acquire_control_lease_request_value_copy_fields(const acquire_control_lease_request_t *view, acquire_control_lease_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_requested_timeout_ms = view->has_requested_timeout_ms;
  out->requested_timeout_ms = view->has_requested_timeout_ms ? view->requested_timeout_ms : UINT32_C(0);
  out->has_current_token = view->has_current_token;
  out->current_token = view->has_current_token ? view->current_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void acquire_control_lease_request_wlc_detail_value_copy(const acquire_control_lease_request_t *view, acquire_control_lease_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  acquire_control_lease_request_value_copy_fields(view, out);
}

void acquire_control_lease_request_value_clear(acquire_control_lease_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  acquire_control_lease_request_value_defaults(value);
}

wl_codec_status_t acquire_control_lease_request_value_from_view(const acquire_control_lease_request_t *view, acquire_control_lease_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&acquire_control_lease_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  acquire_control_lease_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t acquire_control_lease_request_value_borrow(const acquire_control_lease_request_value_t *value, acquire_control_lease_request_t *out) {
  acquire_control_lease_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_requested_timeout_ms = value->has_requested_timeout_ms;
  if (value->has_requested_timeout_ms) {
    out->requested_timeout_ms = value->requested_timeout_ms;
  }
  out->has_current_token = value->has_current_token;
  if (value->has_current_token) {
    out->current_token = value->current_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t acquire_control_lease_request_value_to_view(const acquire_control_lease_request_value_t *value, acquire_control_lease_request_t *out) {
  acquire_control_lease_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = acquire_control_lease_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&acquire_control_lease_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t acquire_control_lease_request_value_encoded_size(const acquire_control_lease_request_value_t *value) {
  acquire_control_lease_request_t view;
  if (value == NULL || acquire_control_lease_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return acquire_control_lease_request_encoded_size(&view);
}

wl_codec_status_t acquire_control_lease_request_value_encode(const acquire_control_lease_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  acquire_control_lease_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = acquire_control_lease_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? acquire_control_lease_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t acquire_control_lease_request_value_decode(const uint8_t *input, size_t length, acquire_control_lease_request_value_t *out) {
  acquire_control_lease_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = acquire_control_lease_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  acquire_control_lease_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void acquire_control_lease_response_value_defaults(acquire_control_lease_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
  out->lease_token = UINT64_C(0);
  out->granted_timeout_ms = UINT32_C(0);
}

static void acquire_control_lease_response_value_copy_fields(const acquire_control_lease_response_t *view, acquire_control_lease_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
  out->has_granted_timeout_ms = view->has_granted_timeout_ms;
  out->granted_timeout_ms = view->has_granted_timeout_ms ? view->granted_timeout_ms : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void acquire_control_lease_response_wlc_detail_value_copy(const acquire_control_lease_response_t *view, acquire_control_lease_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  acquire_control_lease_response_value_copy_fields(view, out);
}

void acquire_control_lease_response_value_clear(acquire_control_lease_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  acquire_control_lease_response_value_defaults(value);
}

wl_codec_status_t acquire_control_lease_response_value_from_view(const acquire_control_lease_response_t *view, acquire_control_lease_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&acquire_control_lease_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  acquire_control_lease_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t acquire_control_lease_response_value_borrow(const acquire_control_lease_response_value_t *value, acquire_control_lease_response_t *out) {
  acquire_control_lease_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  out->has_granted_timeout_ms = value->has_granted_timeout_ms;
  if (value->has_granted_timeout_ms) {
    out->granted_timeout_ms = value->granted_timeout_ms;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t acquire_control_lease_response_value_to_view(const acquire_control_lease_response_value_t *value, acquire_control_lease_response_t *out) {
  acquire_control_lease_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = acquire_control_lease_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&acquire_control_lease_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t acquire_control_lease_response_value_encoded_size(const acquire_control_lease_response_value_t *value) {
  acquire_control_lease_response_t view;
  if (value == NULL || acquire_control_lease_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return acquire_control_lease_response_encoded_size(&view);
}

wl_codec_status_t acquire_control_lease_response_value_encode(const acquire_control_lease_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  acquire_control_lease_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = acquire_control_lease_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? acquire_control_lease_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t acquire_control_lease_response_value_decode(const uint8_t *input, size_t length, acquire_control_lease_response_value_t *out) {
  acquire_control_lease_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = acquire_control_lease_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  acquire_control_lease_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void release_control_lease_request_value_defaults(release_control_lease_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->lease_token = UINT64_C(0);
}

static void release_control_lease_request_value_copy_fields(const release_control_lease_request_t *view, release_control_lease_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void release_control_lease_request_wlc_detail_value_copy(const release_control_lease_request_t *view, release_control_lease_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  release_control_lease_request_value_copy_fields(view, out);
}

void release_control_lease_request_value_clear(release_control_lease_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  release_control_lease_request_value_defaults(value);
}

wl_codec_status_t release_control_lease_request_value_from_view(const release_control_lease_request_t *view, release_control_lease_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&release_control_lease_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  release_control_lease_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t release_control_lease_request_value_borrow(const release_control_lease_request_value_t *value, release_control_lease_request_t *out) {
  release_control_lease_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t release_control_lease_request_value_to_view(const release_control_lease_request_value_t *value, release_control_lease_request_t *out) {
  release_control_lease_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = release_control_lease_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&release_control_lease_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t release_control_lease_request_value_encoded_size(const release_control_lease_request_value_t *value) {
  release_control_lease_request_t view;
  if (value == NULL || release_control_lease_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return release_control_lease_request_encoded_size(&view);
}

wl_codec_status_t release_control_lease_request_value_encode(const release_control_lease_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  release_control_lease_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = release_control_lease_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? release_control_lease_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t release_control_lease_request_value_decode(const uint8_t *input, size_t length, release_control_lease_request_value_t *out) {
  release_control_lease_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = release_control_lease_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  release_control_lease_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void release_control_lease_response_value_defaults(release_control_lease_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void release_control_lease_response_value_copy_fields(const release_control_lease_response_t *view, release_control_lease_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void release_control_lease_response_wlc_detail_value_copy(const release_control_lease_response_t *view, release_control_lease_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  release_control_lease_response_value_copy_fields(view, out);
}

void release_control_lease_response_value_clear(release_control_lease_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  release_control_lease_response_value_defaults(value);
}

wl_codec_status_t release_control_lease_response_value_from_view(const release_control_lease_response_t *view, release_control_lease_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&release_control_lease_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  release_control_lease_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t release_control_lease_response_value_borrow(const release_control_lease_response_value_t *value, release_control_lease_response_t *out) {
  release_control_lease_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t release_control_lease_response_value_to_view(const release_control_lease_response_value_t *value, release_control_lease_response_t *out) {
  release_control_lease_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = release_control_lease_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&release_control_lease_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t release_control_lease_response_value_encoded_size(const release_control_lease_response_value_t *value) {
  release_control_lease_response_t view;
  if (value == NULL || release_control_lease_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return release_control_lease_response_encoded_size(&view);
}

wl_codec_status_t release_control_lease_response_value_encode(const release_control_lease_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  release_control_lease_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = release_control_lease_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? release_control_lease_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t release_control_lease_response_value_decode(const uint8_t *input, size_t length, release_control_lease_response_value_t *out) {
  release_control_lease_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = release_control_lease_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  release_control_lease_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void get_motor_feedback_request_value_defaults(get_motor_feedback_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void get_motor_feedback_request_value_copy_fields(const get_motor_feedback_request_t *view, get_motor_feedback_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void get_motor_feedback_request_wlc_detail_value_copy(const get_motor_feedback_request_t *view, get_motor_feedback_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  get_motor_feedback_request_value_copy_fields(view, out);
}

void get_motor_feedback_request_value_clear(get_motor_feedback_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  get_motor_feedback_request_value_defaults(value);
}

wl_codec_status_t get_motor_feedback_request_value_from_view(const get_motor_feedback_request_t *view, get_motor_feedback_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&get_motor_feedback_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  get_motor_feedback_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t get_motor_feedback_request_value_borrow(const get_motor_feedback_request_value_t *value, get_motor_feedback_request_t *out) {
  get_motor_feedback_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t get_motor_feedback_request_value_to_view(const get_motor_feedback_request_value_t *value, get_motor_feedback_request_t *out) {
  get_motor_feedback_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_motor_feedback_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&get_motor_feedback_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t get_motor_feedback_request_value_encoded_size(const get_motor_feedback_request_value_t *value) {
  get_motor_feedback_request_t view;
  if (value == NULL || get_motor_feedback_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return get_motor_feedback_request_encoded_size(&view);
}

wl_codec_status_t get_motor_feedback_request_value_encode(const get_motor_feedback_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  get_motor_feedback_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_motor_feedback_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? get_motor_feedback_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t get_motor_feedback_request_value_decode(const uint8_t *input, size_t length, get_motor_feedback_request_value_t *out) {
  get_motor_feedback_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_motor_feedback_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  get_motor_feedback_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void get_motor_feedback_response_value_defaults(get_motor_feedback_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
  motor_feedback_value_defaults(&out->feedback);
}

static void get_motor_feedback_response_value_copy_fields(const get_motor_feedback_response_t *view, get_motor_feedback_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
  out->has_feedback = view->has_feedback;
  if (view->has_feedback) motor_feedback_value_copy_fields(&view->feedback, &out->feedback);
  else motor_feedback_value_defaults(&out->feedback);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void get_motor_feedback_response_wlc_detail_value_copy(const get_motor_feedback_response_t *view, get_motor_feedback_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  get_motor_feedback_response_value_copy_fields(view, out);
}

void get_motor_feedback_response_value_clear(get_motor_feedback_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  get_motor_feedback_response_value_defaults(value);
}

wl_codec_status_t get_motor_feedback_response_value_from_view(const get_motor_feedback_response_t *view, get_motor_feedback_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&get_motor_feedback_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  get_motor_feedback_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t get_motor_feedback_response_value_borrow(const get_motor_feedback_response_value_t *value, get_motor_feedback_response_t *out) {
  get_motor_feedback_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  out->has_feedback = value->has_feedback;
  if (value->has_feedback) {
    wl_codec_status_t status = motor_feedback_value_borrow(&value->feedback, &out->feedback);
    if (status != WL_CODEC_OK) return status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t get_motor_feedback_response_value_to_view(const get_motor_feedback_response_value_t *value, get_motor_feedback_response_t *out) {
  get_motor_feedback_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_motor_feedback_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&get_motor_feedback_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t get_motor_feedback_response_value_encoded_size(const get_motor_feedback_response_value_t *value) {
  get_motor_feedback_response_t view;
  if (value == NULL || get_motor_feedback_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return get_motor_feedback_response_encoded_size(&view);
}

wl_codec_status_t get_motor_feedback_response_value_encode(const get_motor_feedback_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  get_motor_feedback_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_motor_feedback_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? get_motor_feedback_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t get_motor_feedback_response_value_decode(const uint8_t *input, size_t length, get_motor_feedback_response_value_t *out) {
  get_motor_feedback_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_motor_feedback_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  get_motor_feedback_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void get_device_info_request_value_defaults(get_device_info_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void get_device_info_request_value_copy_fields(const get_device_info_request_t *view, get_device_info_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void get_device_info_request_wlc_detail_value_copy(const get_device_info_request_t *view, get_device_info_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  get_device_info_request_value_copy_fields(view, out);
}

void get_device_info_request_value_clear(get_device_info_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  get_device_info_request_value_defaults(value);
}

wl_codec_status_t get_device_info_request_value_from_view(const get_device_info_request_t *view, get_device_info_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&get_device_info_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  get_device_info_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t get_device_info_request_value_borrow(const get_device_info_request_value_t *value, get_device_info_request_t *out) {
  get_device_info_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t get_device_info_request_value_to_view(const get_device_info_request_value_t *value, get_device_info_request_t *out) {
  get_device_info_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_info_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&get_device_info_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t get_device_info_request_value_encoded_size(const get_device_info_request_value_t *value) {
  get_device_info_request_t view;
  if (value == NULL || get_device_info_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return get_device_info_request_encoded_size(&view);
}

wl_codec_status_t get_device_info_request_value_encode(const get_device_info_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  get_device_info_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_info_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? get_device_info_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t get_device_info_request_value_decode(const uint8_t *input, size_t length, get_device_info_request_value_t *out) {
  get_device_info_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_info_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  get_device_info_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void get_device_info_response_value_defaults(get_device_info_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
  device_info_value_defaults(&out->info);
}

static void get_device_info_response_value_copy_fields(const get_device_info_response_t *view, get_device_info_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
  out->has_info = view->has_info;
  if (view->has_info) device_info_value_copy_fields(&view->info, &out->info);
  else device_info_value_defaults(&out->info);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void get_device_info_response_wlc_detail_value_copy(const get_device_info_response_t *view, get_device_info_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  get_device_info_response_value_copy_fields(view, out);
}

void get_device_info_response_value_clear(get_device_info_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  get_device_info_response_value_defaults(value);
}

wl_codec_status_t get_device_info_response_value_from_view(const get_device_info_response_t *view, get_device_info_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&get_device_info_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  get_device_info_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t get_device_info_response_value_borrow(const get_device_info_response_value_t *value, get_device_info_response_t *out) {
  get_device_info_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  out->has_info = value->has_info;
  if (value->has_info) {
    wl_codec_status_t status = device_info_value_borrow(&value->info, &out->info);
    if (status != WL_CODEC_OK) return status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t get_device_info_response_value_to_view(const get_device_info_response_value_t *value, get_device_info_response_t *out) {
  get_device_info_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_info_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&get_device_info_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t get_device_info_response_value_encoded_size(const get_device_info_response_value_t *value) {
  get_device_info_response_t view;
  if (value == NULL || get_device_info_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return get_device_info_response_encoded_size(&view);
}

wl_codec_status_t get_device_info_response_value_encode(const get_device_info_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  get_device_info_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_info_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? get_device_info_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t get_device_info_response_value_decode(const uint8_t *input, size_t length, get_device_info_response_value_t *out) {
  get_device_info_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_info_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  get_device_info_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_device_info_request_value_defaults(set_device_info_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void set_device_info_request_value_copy_fields(const set_device_info_request_t *view, set_device_info_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_custom_name = view->has_custom_name;
  if (view->has_custom_name) {
    out->custom_name.length = view->custom_name.length;
    if (view->custom_name.length != 0U) memcpy(out->custom_name.data, view->custom_name.data, view->custom_name.length);
  } else {
  }
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_device_info_request_wlc_detail_value_copy(const set_device_info_request_t *view, set_device_info_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_device_info_request_value_copy_fields(view, out);
}

void set_device_info_request_value_clear(set_device_info_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_device_info_request_value_defaults(value);
}

wl_codec_status_t set_device_info_request_value_from_view(const set_device_info_request_t *view, set_device_info_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_device_info_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_device_info_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_device_info_request_value_borrow(const set_device_info_request_value_t *value, set_device_info_request_t *out) {
  set_device_info_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_custom_name = value->has_custom_name;
  if (value->has_custom_name) {
    if (value->custom_name.length > 31U) return WL_CODEC_ERR_INVALID_VALUE;
    out->custom_name.length = value->custom_name.length;
    out->custom_name.data = value->custom_name.data;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_device_info_request_value_to_view(const set_device_info_request_value_t *value, set_device_info_request_t *out) {
  set_device_info_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_info_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_device_info_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_device_info_request_value_encoded_size(const set_device_info_request_value_t *value) {
  set_device_info_request_t view;
  if (value == NULL || set_device_info_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_device_info_request_encoded_size(&view);
}

wl_codec_status_t set_device_info_request_value_encode(const set_device_info_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_device_info_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_info_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_device_info_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_device_info_request_value_decode(const uint8_t *input, size_t length, set_device_info_request_value_t *out) {
  set_device_info_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_info_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_device_info_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_device_info_response_value_defaults(set_device_info_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void set_device_info_response_value_copy_fields(const set_device_info_response_t *view, set_device_info_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_device_info_response_wlc_detail_value_copy(const set_device_info_response_t *view, set_device_info_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_device_info_response_value_copy_fields(view, out);
}

void set_device_info_response_value_clear(set_device_info_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_device_info_response_value_defaults(value);
}

wl_codec_status_t set_device_info_response_value_from_view(const set_device_info_response_t *view, set_device_info_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_device_info_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_device_info_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_device_info_response_value_borrow(const set_device_info_response_value_t *value, set_device_info_response_t *out) {
  set_device_info_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_device_info_response_value_to_view(const set_device_info_response_value_t *value, set_device_info_response_t *out) {
  set_device_info_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_info_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_device_info_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_device_info_response_value_encoded_size(const set_device_info_response_value_t *value) {
  set_device_info_response_t view;
  if (value == NULL || set_device_info_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_device_info_response_encoded_size(&view);
}

wl_codec_status_t set_device_info_response_value_encode(const set_device_info_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_device_info_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_info_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_device_info_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_device_info_response_value_decode(const uint8_t *input, size_t length, set_device_info_response_value_t *out) {
  set_device_info_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_info_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_device_info_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_arm_control_mode_request_value_defaults(set_arm_control_mode_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->mode = 0;
}

static void set_arm_control_mode_request_value_copy_fields(const set_arm_control_mode_request_t *view, set_arm_control_mode_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_mode = view->has_mode;
  out->mode = view->has_mode ? view->mode : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_arm_control_mode_request_wlc_detail_value_copy(const set_arm_control_mode_request_t *view, set_arm_control_mode_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_arm_control_mode_request_value_copy_fields(view, out);
}

void set_arm_control_mode_request_value_clear(set_arm_control_mode_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_arm_control_mode_request_value_defaults(value);
}

wl_codec_status_t set_arm_control_mode_request_value_from_view(const set_arm_control_mode_request_t *view, set_arm_control_mode_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_arm_control_mode_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_arm_control_mode_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_arm_control_mode_request_value_borrow(const set_arm_control_mode_request_value_t *value, set_arm_control_mode_request_t *out) {
  set_arm_control_mode_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_mode = value->has_mode;
  if (value->has_mode) {
    out->mode = value->mode;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_arm_control_mode_request_value_to_view(const set_arm_control_mode_request_value_t *value, set_arm_control_mode_request_t *out) {
  set_arm_control_mode_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_control_mode_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_arm_control_mode_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_arm_control_mode_request_value_encoded_size(const set_arm_control_mode_request_value_t *value) {
  set_arm_control_mode_request_t view;
  if (value == NULL || set_arm_control_mode_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_arm_control_mode_request_encoded_size(&view);
}

wl_codec_status_t set_arm_control_mode_request_value_encode(const set_arm_control_mode_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_arm_control_mode_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_control_mode_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_arm_control_mode_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_arm_control_mode_request_value_decode(const uint8_t *input, size_t length, set_arm_control_mode_request_value_t *out) {
  set_arm_control_mode_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_control_mode_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_arm_control_mode_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_arm_control_mode_response_value_defaults(set_arm_control_mode_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void set_arm_control_mode_response_value_copy_fields(const set_arm_control_mode_response_t *view, set_arm_control_mode_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_arm_control_mode_response_wlc_detail_value_copy(const set_arm_control_mode_response_t *view, set_arm_control_mode_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_arm_control_mode_response_value_copy_fields(view, out);
}

void set_arm_control_mode_response_value_clear(set_arm_control_mode_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_arm_control_mode_response_value_defaults(value);
}

wl_codec_status_t set_arm_control_mode_response_value_from_view(const set_arm_control_mode_response_t *view, set_arm_control_mode_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_arm_control_mode_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_arm_control_mode_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_arm_control_mode_response_value_borrow(const set_arm_control_mode_response_value_t *value, set_arm_control_mode_response_t *out) {
  set_arm_control_mode_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_arm_control_mode_response_value_to_view(const set_arm_control_mode_response_value_t *value, set_arm_control_mode_response_t *out) {
  set_arm_control_mode_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_control_mode_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_arm_control_mode_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_arm_control_mode_response_value_encoded_size(const set_arm_control_mode_response_value_t *value) {
  set_arm_control_mode_response_t view;
  if (value == NULL || set_arm_control_mode_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_arm_control_mode_response_encoded_size(&view);
}

wl_codec_status_t set_arm_control_mode_response_value_encode(const set_arm_control_mode_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_arm_control_mode_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_control_mode_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_arm_control_mode_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_arm_control_mode_response_value_decode(const uint8_t *input, size_t length, set_arm_control_mode_response_value_t *out) {
  set_arm_control_mode_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_control_mode_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_arm_control_mode_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_gripper_control_mode_request_value_defaults(set_gripper_control_mode_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->mode = 0;
}

static void set_gripper_control_mode_request_value_copy_fields(const set_gripper_control_mode_request_t *view, set_gripper_control_mode_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_mode = view->has_mode;
  out->mode = view->has_mode ? view->mode : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_gripper_control_mode_request_wlc_detail_value_copy(const set_gripper_control_mode_request_t *view, set_gripper_control_mode_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_gripper_control_mode_request_value_copy_fields(view, out);
}

void set_gripper_control_mode_request_value_clear(set_gripper_control_mode_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_gripper_control_mode_request_value_defaults(value);
}

wl_codec_status_t set_gripper_control_mode_request_value_from_view(const set_gripper_control_mode_request_t *view, set_gripper_control_mode_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_gripper_control_mode_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_gripper_control_mode_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_gripper_control_mode_request_value_borrow(const set_gripper_control_mode_request_value_t *value, set_gripper_control_mode_request_t *out) {
  set_gripper_control_mode_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_mode = value->has_mode;
  if (value->has_mode) {
    out->mode = value->mode;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_gripper_control_mode_request_value_to_view(const set_gripper_control_mode_request_value_t *value, set_gripper_control_mode_request_t *out) {
  set_gripper_control_mode_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_gripper_control_mode_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_gripper_control_mode_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_gripper_control_mode_request_value_encoded_size(const set_gripper_control_mode_request_value_t *value) {
  set_gripper_control_mode_request_t view;
  if (value == NULL || set_gripper_control_mode_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_gripper_control_mode_request_encoded_size(&view);
}

wl_codec_status_t set_gripper_control_mode_request_value_encode(const set_gripper_control_mode_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_gripper_control_mode_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_gripper_control_mode_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_gripper_control_mode_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_gripper_control_mode_request_value_decode(const uint8_t *input, size_t length, set_gripper_control_mode_request_value_t *out) {
  set_gripper_control_mode_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_gripper_control_mode_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_gripper_control_mode_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_gripper_control_mode_response_value_defaults(set_gripper_control_mode_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void set_gripper_control_mode_response_value_copy_fields(const set_gripper_control_mode_response_t *view, set_gripper_control_mode_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_gripper_control_mode_response_wlc_detail_value_copy(const set_gripper_control_mode_response_t *view, set_gripper_control_mode_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_gripper_control_mode_response_value_copy_fields(view, out);
}

void set_gripper_control_mode_response_value_clear(set_gripper_control_mode_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_gripper_control_mode_response_value_defaults(value);
}

wl_codec_status_t set_gripper_control_mode_response_value_from_view(const set_gripper_control_mode_response_t *view, set_gripper_control_mode_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_gripper_control_mode_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_gripper_control_mode_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_gripper_control_mode_response_value_borrow(const set_gripper_control_mode_response_value_t *value, set_gripper_control_mode_response_t *out) {
  set_gripper_control_mode_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_gripper_control_mode_response_value_to_view(const set_gripper_control_mode_response_value_t *value, set_gripper_control_mode_response_t *out) {
  set_gripper_control_mode_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_gripper_control_mode_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_gripper_control_mode_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_gripper_control_mode_response_value_encoded_size(const set_gripper_control_mode_response_value_t *value) {
  set_gripper_control_mode_response_t view;
  if (value == NULL || set_gripper_control_mode_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_gripper_control_mode_response_encoded_size(&view);
}

wl_codec_status_t set_gripper_control_mode_response_value_encode(const set_gripper_control_mode_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_gripper_control_mode_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_gripper_control_mode_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_gripper_control_mode_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_gripper_control_mode_response_value_decode(const uint8_t *input, size_t length, set_gripper_control_mode_response_value_t *out) {
  set_gripper_control_mode_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_gripper_control_mode_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_gripper_control_mode_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_register_read_request_value_defaults(motor_register_read_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->joint_id = 0;
  out->register_id = 0;
}

static void motor_register_read_request_value_copy_fields(const motor_register_read_request_t *view, motor_register_read_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
  out->has_register_id = view->has_register_id;
  out->register_id = view->has_register_id ? view->register_id : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_register_read_request_wlc_detail_value_copy(const motor_register_read_request_t *view, motor_register_read_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_register_read_request_value_copy_fields(view, out);
}

void motor_register_read_request_value_clear(motor_register_read_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_register_read_request_value_defaults(value);
}

wl_codec_status_t motor_register_read_request_value_from_view(const motor_register_read_request_t *view, motor_register_read_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_register_read_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_register_read_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_register_read_request_value_borrow(const motor_register_read_request_value_t *value, motor_register_read_request_t *out) {
  motor_register_read_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  out->has_register_id = value->has_register_id;
  if (value->has_register_id) {
    out->register_id = value->register_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_register_read_request_value_to_view(const motor_register_read_request_value_t *value, motor_register_read_request_t *out) {
  motor_register_read_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_read_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_register_read_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_register_read_request_value_encoded_size(const motor_register_read_request_value_t *value) {
  motor_register_read_request_t view;
  if (value == NULL || motor_register_read_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_register_read_request_encoded_size(&view);
}

wl_codec_status_t motor_register_read_request_value_encode(const motor_register_read_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_register_read_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_read_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_register_read_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_register_read_request_value_decode(const uint8_t *input, size_t length, motor_register_read_request_value_t *out) {
  motor_register_read_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_read_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_register_read_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_register_read_response_value_defaults(motor_register_read_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
  out->joint_id = 0;
  out->register_id = 0;
  out->value = 0;
}

static void motor_register_read_response_value_copy_fields(const motor_register_read_response_t *view, motor_register_read_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
  out->has_register_id = view->has_register_id;
  out->register_id = view->has_register_id ? view->register_id : 0;
  out->has_value = view->has_value;
  out->value = view->has_value ? view->value : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_register_read_response_wlc_detail_value_copy(const motor_register_read_response_t *view, motor_register_read_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_register_read_response_value_copy_fields(view, out);
}

void motor_register_read_response_value_clear(motor_register_read_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_register_read_response_value_defaults(value);
}

wl_codec_status_t motor_register_read_response_value_from_view(const motor_register_read_response_t *view, motor_register_read_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_register_read_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_register_read_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_register_read_response_value_borrow(const motor_register_read_response_value_t *value, motor_register_read_response_t *out) {
  motor_register_read_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  out->has_register_id = value->has_register_id;
  if (value->has_register_id) {
    out->register_id = value->register_id;
  }
  out->has_value = value->has_value;
  if (value->has_value) {
    out->value = value->value;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_register_read_response_value_to_view(const motor_register_read_response_value_t *value, motor_register_read_response_t *out) {
  motor_register_read_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_read_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_register_read_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_register_read_response_value_encoded_size(const motor_register_read_response_value_t *value) {
  motor_register_read_response_t view;
  if (value == NULL || motor_register_read_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_register_read_response_encoded_size(&view);
}

wl_codec_status_t motor_register_read_response_value_encode(const motor_register_read_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_register_read_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_read_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_register_read_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_register_read_response_value_decode(const uint8_t *input, size_t length, motor_register_read_response_value_t *out) {
  motor_register_read_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_read_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_register_read_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_register_write_request_value_defaults(motor_register_write_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->joint_id = 0;
  out->register_id = 0;
  out->value = 0;
}

static void motor_register_write_request_value_copy_fields(const motor_register_write_request_t *view, motor_register_write_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
  out->has_register_id = view->has_register_id;
  out->register_id = view->has_register_id ? view->register_id : 0;
  out->has_value = view->has_value;
  out->value = view->has_value ? view->value : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_register_write_request_wlc_detail_value_copy(const motor_register_write_request_t *view, motor_register_write_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_register_write_request_value_copy_fields(view, out);
}

void motor_register_write_request_value_clear(motor_register_write_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_register_write_request_value_defaults(value);
}

wl_codec_status_t motor_register_write_request_value_from_view(const motor_register_write_request_t *view, motor_register_write_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_register_write_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_register_write_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_register_write_request_value_borrow(const motor_register_write_request_value_t *value, motor_register_write_request_t *out) {
  motor_register_write_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  out->has_register_id = value->has_register_id;
  if (value->has_register_id) {
    out->register_id = value->register_id;
  }
  out->has_value = value->has_value;
  if (value->has_value) {
    out->value = value->value;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_register_write_request_value_to_view(const motor_register_write_request_value_t *value, motor_register_write_request_t *out) {
  motor_register_write_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_write_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_register_write_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_register_write_request_value_encoded_size(const motor_register_write_request_value_t *value) {
  motor_register_write_request_t view;
  if (value == NULL || motor_register_write_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_register_write_request_encoded_size(&view);
}

wl_codec_status_t motor_register_write_request_value_encode(const motor_register_write_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_register_write_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_write_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_register_write_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_register_write_request_value_decode(const uint8_t *input, size_t length, motor_register_write_request_value_t *out) {
  motor_register_write_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_write_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_register_write_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_register_write_response_value_defaults(motor_register_write_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void motor_register_write_response_value_copy_fields(const motor_register_write_response_t *view, motor_register_write_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_register_write_response_wlc_detail_value_copy(const motor_register_write_response_t *view, motor_register_write_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_register_write_response_value_copy_fields(view, out);
}

void motor_register_write_response_value_clear(motor_register_write_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_register_write_response_value_defaults(value);
}

wl_codec_status_t motor_register_write_response_value_from_view(const motor_register_write_response_t *view, motor_register_write_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_register_write_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_register_write_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_register_write_response_value_borrow(const motor_register_write_response_value_t *value, motor_register_write_response_t *out) {
  motor_register_write_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_register_write_response_value_to_view(const motor_register_write_response_value_t *value, motor_register_write_response_t *out) {
  motor_register_write_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_write_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_register_write_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_register_write_response_value_encoded_size(const motor_register_write_response_value_t *value) {
  motor_register_write_response_t view;
  if (value == NULL || motor_register_write_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_register_write_response_encoded_size(&view);
}

wl_codec_status_t motor_register_write_response_value_encode(const motor_register_write_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_register_write_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_write_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_register_write_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_register_write_response_value_decode(const uint8_t *input, size_t length, motor_register_write_response_value_t *out) {
  motor_register_write_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_register_write_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_register_write_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_store_parameters_request_value_defaults(motor_store_parameters_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->joint_id = 0;
}

static void motor_store_parameters_request_value_copy_fields(const motor_store_parameters_request_t *view, motor_store_parameters_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_store_parameters_request_wlc_detail_value_copy(const motor_store_parameters_request_t *view, motor_store_parameters_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_store_parameters_request_value_copy_fields(view, out);
}

void motor_store_parameters_request_value_clear(motor_store_parameters_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_store_parameters_request_value_defaults(value);
}

wl_codec_status_t motor_store_parameters_request_value_from_view(const motor_store_parameters_request_t *view, motor_store_parameters_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_store_parameters_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_store_parameters_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_store_parameters_request_value_borrow(const motor_store_parameters_request_value_t *value, motor_store_parameters_request_t *out) {
  motor_store_parameters_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_store_parameters_request_value_to_view(const motor_store_parameters_request_value_t *value, motor_store_parameters_request_t *out) {
  motor_store_parameters_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_store_parameters_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_store_parameters_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_store_parameters_request_value_encoded_size(const motor_store_parameters_request_value_t *value) {
  motor_store_parameters_request_t view;
  if (value == NULL || motor_store_parameters_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_store_parameters_request_encoded_size(&view);
}

wl_codec_status_t motor_store_parameters_request_value_encode(const motor_store_parameters_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_store_parameters_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_store_parameters_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_store_parameters_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_store_parameters_request_value_decode(const uint8_t *input, size_t length, motor_store_parameters_request_value_t *out) {
  motor_store_parameters_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_store_parameters_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_store_parameters_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_store_parameters_response_value_defaults(motor_store_parameters_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void motor_store_parameters_response_value_copy_fields(const motor_store_parameters_response_t *view, motor_store_parameters_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_store_parameters_response_wlc_detail_value_copy(const motor_store_parameters_response_t *view, motor_store_parameters_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_store_parameters_response_value_copy_fields(view, out);
}

void motor_store_parameters_response_value_clear(motor_store_parameters_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_store_parameters_response_value_defaults(value);
}

wl_codec_status_t motor_store_parameters_response_value_from_view(const motor_store_parameters_response_t *view, motor_store_parameters_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_store_parameters_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_store_parameters_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_store_parameters_response_value_borrow(const motor_store_parameters_response_value_t *value, motor_store_parameters_response_t *out) {
  motor_store_parameters_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_store_parameters_response_value_to_view(const motor_store_parameters_response_value_t *value, motor_store_parameters_response_t *out) {
  motor_store_parameters_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_store_parameters_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_store_parameters_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_store_parameters_response_value_encoded_size(const motor_store_parameters_response_value_t *value) {
  motor_store_parameters_response_t view;
  if (value == NULL || motor_store_parameters_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_store_parameters_response_encoded_size(&view);
}

wl_codec_status_t motor_store_parameters_response_value_encode(const motor_store_parameters_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_store_parameters_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_store_parameters_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_store_parameters_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_store_parameters_response_value_decode(const uint8_t *input, size_t length, motor_store_parameters_response_value_t *out) {
  motor_store_parameters_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_store_parameters_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_store_parameters_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_set_zero_request_value_defaults(motor_set_zero_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->joint_id = 0;
}

static void motor_set_zero_request_value_copy_fields(const motor_set_zero_request_t *view, motor_set_zero_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_joint_id = view->has_joint_id;
  out->joint_id = view->has_joint_id ? view->joint_id : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_set_zero_request_wlc_detail_value_copy(const motor_set_zero_request_t *view, motor_set_zero_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_set_zero_request_value_copy_fields(view, out);
}

void motor_set_zero_request_value_clear(motor_set_zero_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_set_zero_request_value_defaults(value);
}

wl_codec_status_t motor_set_zero_request_value_from_view(const motor_set_zero_request_t *view, motor_set_zero_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_set_zero_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_set_zero_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_set_zero_request_value_borrow(const motor_set_zero_request_value_t *value, motor_set_zero_request_t *out) {
  motor_set_zero_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_joint_id = value->has_joint_id;
  if (value->has_joint_id) {
    out->joint_id = value->joint_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_set_zero_request_value_to_view(const motor_set_zero_request_value_t *value, motor_set_zero_request_t *out) {
  motor_set_zero_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_set_zero_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_set_zero_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_set_zero_request_value_encoded_size(const motor_set_zero_request_value_t *value) {
  motor_set_zero_request_t view;
  if (value == NULL || motor_set_zero_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_set_zero_request_encoded_size(&view);
}

wl_codec_status_t motor_set_zero_request_value_encode(const motor_set_zero_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_set_zero_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_set_zero_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_set_zero_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_set_zero_request_value_decode(const uint8_t *input, size_t length, motor_set_zero_request_value_t *out) {
  motor_set_zero_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_set_zero_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_set_zero_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void motor_set_zero_response_value_defaults(motor_set_zero_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void motor_set_zero_response_value_copy_fields(const motor_set_zero_response_t *view, motor_set_zero_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void motor_set_zero_response_wlc_detail_value_copy(const motor_set_zero_response_t *view, motor_set_zero_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  motor_set_zero_response_value_copy_fields(view, out);
}

void motor_set_zero_response_value_clear(motor_set_zero_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  motor_set_zero_response_value_defaults(value);
}

wl_codec_status_t motor_set_zero_response_value_from_view(const motor_set_zero_response_t *view, motor_set_zero_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&motor_set_zero_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  motor_set_zero_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t motor_set_zero_response_value_borrow(const motor_set_zero_response_value_t *value, motor_set_zero_response_t *out) {
  motor_set_zero_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t motor_set_zero_response_value_to_view(const motor_set_zero_response_value_t *value, motor_set_zero_response_t *out) {
  motor_set_zero_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_set_zero_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&motor_set_zero_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t motor_set_zero_response_value_encoded_size(const motor_set_zero_response_value_t *value) {
  motor_set_zero_response_t view;
  if (value == NULL || motor_set_zero_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return motor_set_zero_response_encoded_size(&view);
}

wl_codec_status_t motor_set_zero_response_value_encode(const motor_set_zero_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  motor_set_zero_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_set_zero_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? motor_set_zero_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t motor_set_zero_response_value_decode(const uint8_t *input, size_t length, motor_set_zero_response_value_t *out) {
  motor_set_zero_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = motor_set_zero_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  motor_set_zero_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_arm_mode_request_value_defaults(set_arm_mode_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->mode = 0;
}

static void set_arm_mode_request_value_copy_fields(const set_arm_mode_request_t *view, set_arm_mode_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_mode = view->has_mode;
  out->mode = view->has_mode ? view->mode : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_arm_mode_request_wlc_detail_value_copy(const set_arm_mode_request_t *view, set_arm_mode_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_arm_mode_request_value_copy_fields(view, out);
}

void set_arm_mode_request_value_clear(set_arm_mode_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_arm_mode_request_value_defaults(value);
}

wl_codec_status_t set_arm_mode_request_value_from_view(const set_arm_mode_request_t *view, set_arm_mode_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_arm_mode_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_arm_mode_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_arm_mode_request_value_borrow(const set_arm_mode_request_value_t *value, set_arm_mode_request_t *out) {
  set_arm_mode_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_mode = value->has_mode;
  if (value->has_mode) {
    out->mode = value->mode;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_arm_mode_request_value_to_view(const set_arm_mode_request_value_t *value, set_arm_mode_request_t *out) {
  set_arm_mode_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_mode_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_arm_mode_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_arm_mode_request_value_encoded_size(const set_arm_mode_request_value_t *value) {
  set_arm_mode_request_t view;
  if (value == NULL || set_arm_mode_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_arm_mode_request_encoded_size(&view);
}

wl_codec_status_t set_arm_mode_request_value_encode(const set_arm_mode_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_arm_mode_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_mode_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_arm_mode_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_arm_mode_request_value_decode(const uint8_t *input, size_t length, set_arm_mode_request_value_t *out) {
  set_arm_mode_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_mode_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_arm_mode_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_arm_mode_response_value_defaults(set_arm_mode_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void set_arm_mode_response_value_copy_fields(const set_arm_mode_response_t *view, set_arm_mode_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_arm_mode_response_wlc_detail_value_copy(const set_arm_mode_response_t *view, set_arm_mode_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_arm_mode_response_value_copy_fields(view, out);
}

void set_arm_mode_response_value_clear(set_arm_mode_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_arm_mode_response_value_defaults(value);
}

wl_codec_status_t set_arm_mode_response_value_from_view(const set_arm_mode_response_t *view, set_arm_mode_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_arm_mode_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_arm_mode_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_arm_mode_response_value_borrow(const set_arm_mode_response_value_t *value, set_arm_mode_response_t *out) {
  set_arm_mode_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_arm_mode_response_value_to_view(const set_arm_mode_response_value_t *value, set_arm_mode_response_t *out) {
  set_arm_mode_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_mode_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_arm_mode_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_arm_mode_response_value_encoded_size(const set_arm_mode_response_value_t *value) {
  set_arm_mode_response_t view;
  if (value == NULL || set_arm_mode_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_arm_mode_response_encoded_size(&view);
}

wl_codec_status_t set_arm_mode_response_value_encode(const set_arm_mode_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_arm_mode_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_mode_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_arm_mode_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_arm_mode_response_value_decode(const uint8_t *input, size_t length, set_arm_mode_response_value_t *out) {
  set_arm_mode_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_arm_mode_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_arm_mode_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void get_device_settings_request_value_defaults(get_device_settings_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void get_device_settings_request_value_copy_fields(const get_device_settings_request_t *view, get_device_settings_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void get_device_settings_request_wlc_detail_value_copy(const get_device_settings_request_t *view, get_device_settings_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  get_device_settings_request_value_copy_fields(view, out);
}

void get_device_settings_request_value_clear(get_device_settings_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  get_device_settings_request_value_defaults(value);
}

wl_codec_status_t get_device_settings_request_value_from_view(const get_device_settings_request_t *view, get_device_settings_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&get_device_settings_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  get_device_settings_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t get_device_settings_request_value_borrow(const get_device_settings_request_value_t *value, get_device_settings_request_t *out) {
  get_device_settings_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t get_device_settings_request_value_to_view(const get_device_settings_request_value_t *value, get_device_settings_request_t *out) {
  get_device_settings_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_settings_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&get_device_settings_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t get_device_settings_request_value_encoded_size(const get_device_settings_request_value_t *value) {
  get_device_settings_request_t view;
  if (value == NULL || get_device_settings_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return get_device_settings_request_encoded_size(&view);
}

wl_codec_status_t get_device_settings_request_value_encode(const get_device_settings_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  get_device_settings_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_settings_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? get_device_settings_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t get_device_settings_request_value_decode(const uint8_t *input, size_t length, get_device_settings_request_value_t *out) {
  get_device_settings_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_settings_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  get_device_settings_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void get_device_settings_response_value_defaults(get_device_settings_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
  device_settings_value_defaults(&out->settings);
}

static void get_device_settings_response_value_copy_fields(const get_device_settings_response_t *view, get_device_settings_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
  out->has_settings = view->has_settings;
  if (view->has_settings) device_settings_value_copy_fields(&view->settings, &out->settings);
  else device_settings_value_defaults(&out->settings);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void get_device_settings_response_wlc_detail_value_copy(const get_device_settings_response_t *view, get_device_settings_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  get_device_settings_response_value_copy_fields(view, out);
}

void get_device_settings_response_value_clear(get_device_settings_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  get_device_settings_response_value_defaults(value);
}

wl_codec_status_t get_device_settings_response_value_from_view(const get_device_settings_response_t *view, get_device_settings_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&get_device_settings_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  get_device_settings_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t get_device_settings_response_value_borrow(const get_device_settings_response_value_t *value, get_device_settings_response_t *out) {
  get_device_settings_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  out->has_settings = value->has_settings;
  if (value->has_settings) {
    wl_codec_status_t status = device_settings_value_borrow(&value->settings, &out->settings);
    if (status != WL_CODEC_OK) return status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t get_device_settings_response_value_to_view(const get_device_settings_response_value_t *value, get_device_settings_response_t *out) {
  get_device_settings_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_settings_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&get_device_settings_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t get_device_settings_response_value_encoded_size(const get_device_settings_response_value_t *value) {
  get_device_settings_response_t view;
  if (value == NULL || get_device_settings_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return get_device_settings_response_encoded_size(&view);
}

wl_codec_status_t get_device_settings_response_value_encode(const get_device_settings_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  get_device_settings_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_settings_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? get_device_settings_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t get_device_settings_response_value_decode(const uint8_t *input, size_t length, get_device_settings_response_value_t *out) {
  get_device_settings_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = get_device_settings_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  get_device_settings_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_device_settings_request_value_defaults(set_device_settings_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  device_settings_value_defaults(&out->settings);
}

static void set_device_settings_request_value_copy_fields(const set_device_settings_request_t *view, set_device_settings_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_settings = view->has_settings;
  if (view->has_settings) device_settings_value_copy_fields(&view->settings, &out->settings);
  else device_settings_value_defaults(&out->settings);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_device_settings_request_wlc_detail_value_copy(const set_device_settings_request_t *view, set_device_settings_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_device_settings_request_value_copy_fields(view, out);
}

void set_device_settings_request_value_clear(set_device_settings_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_device_settings_request_value_defaults(value);
}

wl_codec_status_t set_device_settings_request_value_from_view(const set_device_settings_request_t *view, set_device_settings_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_device_settings_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_device_settings_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_device_settings_request_value_borrow(const set_device_settings_request_value_t *value, set_device_settings_request_t *out) {
  set_device_settings_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_settings = value->has_settings;
  if (value->has_settings) {
    wl_codec_status_t status = device_settings_value_borrow(&value->settings, &out->settings);
    if (status != WL_CODEC_OK) return status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_device_settings_request_value_to_view(const set_device_settings_request_value_t *value, set_device_settings_request_t *out) {
  set_device_settings_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_settings_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_device_settings_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_device_settings_request_value_encoded_size(const set_device_settings_request_value_t *value) {
  set_device_settings_request_t view;
  if (value == NULL || set_device_settings_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_device_settings_request_encoded_size(&view);
}

wl_codec_status_t set_device_settings_request_value_encode(const set_device_settings_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_device_settings_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_settings_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_device_settings_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_device_settings_request_value_decode(const uint8_t *input, size_t length, set_device_settings_request_value_t *out) {
  set_device_settings_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_settings_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_device_settings_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void set_device_settings_response_value_defaults(set_device_settings_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
  device_settings_value_defaults(&out->settings);
}

static void set_device_settings_response_value_copy_fields(const set_device_settings_response_t *view, set_device_settings_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
  out->has_settings = view->has_settings;
  if (view->has_settings) device_settings_value_copy_fields(&view->settings, &out->settings);
  else device_settings_value_defaults(&out->settings);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void set_device_settings_response_wlc_detail_value_copy(const set_device_settings_response_t *view, set_device_settings_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  set_device_settings_response_value_copy_fields(view, out);
}

void set_device_settings_response_value_clear(set_device_settings_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  set_device_settings_response_value_defaults(value);
}

wl_codec_status_t set_device_settings_response_value_from_view(const set_device_settings_response_t *view, set_device_settings_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&set_device_settings_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  set_device_settings_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t set_device_settings_response_value_borrow(const set_device_settings_response_value_t *value, set_device_settings_response_t *out) {
  set_device_settings_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  out->has_settings = value->has_settings;
  if (value->has_settings) {
    wl_codec_status_t status = device_settings_value_borrow(&value->settings, &out->settings);
    if (status != WL_CODEC_OK) return status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t set_device_settings_response_value_to_view(const set_device_settings_response_value_t *value, set_device_settings_response_t *out) {
  set_device_settings_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_settings_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&set_device_settings_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t set_device_settings_response_value_encoded_size(const set_device_settings_response_value_t *value) {
  set_device_settings_response_t view;
  if (value == NULL || set_device_settings_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return set_device_settings_response_encoded_size(&view);
}

wl_codec_status_t set_device_settings_response_value_encode(const set_device_settings_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  set_device_settings_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_settings_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? set_device_settings_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t set_device_settings_response_value_decode(const uint8_t *input, size_t length, set_device_settings_response_value_t *out) {
  set_device_settings_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = set_device_settings_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  set_device_settings_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void joint_mit_command_value_defaults(joint_mit_command_value_t *out) {
  (void)out;
  out->dt_us = UINT32_C(0);
  out->sequence = UINT32_C(0);
  out->gravity_compensation = 0;
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void joint_mit_command_value_copy_fields(const joint_mit_command_t *view, joint_mit_command_value_t *out) {
  out->has_position = view->has_position;
  if (view->has_position) memcpy(out->position, view->position, sizeof(out->position));
  out->has_velocity = view->has_velocity;
  if (view->has_velocity) memcpy(out->velocity, view->velocity, sizeof(out->velocity));
  out->has_torque = view->has_torque;
  if (view->has_torque) memcpy(out->torque, view->torque, sizeof(out->torque));
  out->has_kp = view->has_kp;
  if (view->has_kp) memcpy(out->kp, view->kp, sizeof(out->kp));
  out->has_kd = view->has_kd;
  if (view->has_kd) memcpy(out->kd, view->kd, sizeof(out->kd));
  out->has_dt_us = view->has_dt_us;
  out->dt_us = view->has_dt_us ? view->dt_us : UINT32_C(0);
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_gravity_compensation = view->has_gravity_compensation;
  out->gravity_compensation = view->has_gravity_compensation ? view->gravity_compensation : 0;
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void joint_mit_command_wlc_detail_value_copy(const joint_mit_command_t *view, joint_mit_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  joint_mit_command_value_copy_fields(view, out);
}

void joint_mit_command_value_clear(joint_mit_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  joint_mit_command_value_defaults(value);
}

wl_codec_status_t joint_mit_command_value_from_view(const joint_mit_command_t *view, joint_mit_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&joint_mit_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  joint_mit_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t joint_mit_command_value_borrow(const joint_mit_command_value_t *value, joint_mit_command_t *out) {
  joint_mit_command_clear(out);
  out->has_position = value->has_position;
  if (value->has_position) {
    memcpy(out->position, value->position, sizeof(out->position));
  }
  out->has_velocity = value->has_velocity;
  if (value->has_velocity) {
    memcpy(out->velocity, value->velocity, sizeof(out->velocity));
  }
  out->has_torque = value->has_torque;
  if (value->has_torque) {
    memcpy(out->torque, value->torque, sizeof(out->torque));
  }
  out->has_kp = value->has_kp;
  if (value->has_kp) {
    memcpy(out->kp, value->kp, sizeof(out->kp));
  }
  out->has_kd = value->has_kd;
  if (value->has_kd) {
    memcpy(out->kd, value->kd, sizeof(out->kd));
  }
  out->has_dt_us = value->has_dt_us;
  if (value->has_dt_us) {
    out->dt_us = value->dt_us;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_gravity_compensation = value->has_gravity_compensation;
  if (value->has_gravity_compensation) {
    out->gravity_compensation = value->gravity_compensation;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t joint_mit_command_value_to_view(const joint_mit_command_value_t *value, joint_mit_command_t *out) {
  joint_mit_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_mit_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&joint_mit_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t joint_mit_command_value_encoded_size(const joint_mit_command_value_t *value) {
  joint_mit_command_t view;
  if (value == NULL || joint_mit_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return joint_mit_command_encoded_size(&view);
}

wl_codec_status_t joint_mit_command_value_encode(const joint_mit_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  joint_mit_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_mit_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? joint_mit_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t joint_mit_command_value_decode(const uint8_t *input, size_t length, joint_mit_command_value_t *out) {
  joint_mit_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_mit_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  joint_mit_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void emergency_stop_request_value_defaults(emergency_stop_request_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
}

static void emergency_stop_request_value_copy_fields(const emergency_stop_request_t *view, emergency_stop_request_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void emergency_stop_request_wlc_detail_value_copy(const emergency_stop_request_t *view, emergency_stop_request_value_t *out) {
  memset(out, 0, sizeof(*out));
  emergency_stop_request_value_copy_fields(view, out);
}

void emergency_stop_request_value_clear(emergency_stop_request_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  emergency_stop_request_value_defaults(value);
}

wl_codec_status_t emergency_stop_request_value_from_view(const emergency_stop_request_t *view, emergency_stop_request_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&emergency_stop_request_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  emergency_stop_request_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t emergency_stop_request_value_borrow(const emergency_stop_request_value_t *value, emergency_stop_request_t *out) {
  emergency_stop_request_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t emergency_stop_request_value_to_view(const emergency_stop_request_value_t *value, emergency_stop_request_t *out) {
  emergency_stop_request_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = emergency_stop_request_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&emergency_stop_request_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t emergency_stop_request_value_encoded_size(const emergency_stop_request_value_t *value) {
  emergency_stop_request_t view;
  if (value == NULL || emergency_stop_request_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return emergency_stop_request_encoded_size(&view);
}

wl_codec_status_t emergency_stop_request_value_encode(const emergency_stop_request_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  emergency_stop_request_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = emergency_stop_request_value_borrow(value, &view);
  return status == WL_CODEC_OK ? emergency_stop_request_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t emergency_stop_request_value_decode(const uint8_t *input, size_t length, emergency_stop_request_value_t *out) {
  emergency_stop_request_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = emergency_stop_request_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  emergency_stop_request_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void emergency_stop_response_value_defaults(emergency_stop_response_value_t *out) {
  (void)out;
  out->operation_id = UINT32_C(0);
  out->status = 0;
}

static void emergency_stop_response_value_copy_fields(const emergency_stop_response_t *view, emergency_stop_response_value_t *out) {
  out->has_operation_id = view->has_operation_id;
  out->operation_id = view->has_operation_id ? view->operation_id : UINT32_C(0);
  out->has_status = view->has_status;
  out->status = view->has_status ? view->status : 0;
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void emergency_stop_response_wlc_detail_value_copy(const emergency_stop_response_t *view, emergency_stop_response_value_t *out) {
  memset(out, 0, sizeof(*out));
  emergency_stop_response_value_copy_fields(view, out);
}

void emergency_stop_response_value_clear(emergency_stop_response_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  emergency_stop_response_value_defaults(value);
}

wl_codec_status_t emergency_stop_response_value_from_view(const emergency_stop_response_t *view, emergency_stop_response_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&emergency_stop_response_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  emergency_stop_response_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t emergency_stop_response_value_borrow(const emergency_stop_response_value_t *value, emergency_stop_response_t *out) {
  emergency_stop_response_clear(out);
  out->has_operation_id = value->has_operation_id;
  if (value->has_operation_id) {
    out->operation_id = value->operation_id;
  }
  out->has_status = value->has_status;
  if (value->has_status) {
    out->status = value->status;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t emergency_stop_response_value_to_view(const emergency_stop_response_value_t *value, emergency_stop_response_t *out) {
  emergency_stop_response_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = emergency_stop_response_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&emergency_stop_response_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t emergency_stop_response_value_encoded_size(const emergency_stop_response_value_t *value) {
  emergency_stop_response_t view;
  if (value == NULL || emergency_stop_response_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return emergency_stop_response_encoded_size(&view);
}

wl_codec_status_t emergency_stop_response_value_encode(const emergency_stop_response_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  emergency_stop_response_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = emergency_stop_response_value_borrow(value, &view);
  return status == WL_CODEC_OK ? emergency_stop_response_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t emergency_stop_response_value_decode(const uint8_t *input, size_t length, emergency_stop_response_value_t *out) {
  emergency_stop_response_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = emergency_stop_response_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  emergency_stop_response_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void gripper_mit_command_value_defaults(gripper_mit_command_value_t *out) {
  (void)out;
  out->position = 0;
  out->velocity = 0;
  out->torque = 0;
  out->kp = 0;
  out->kd = 0;
  out->dt_us = UINT32_C(0);
  out->sequence = UINT32_C(0);
  out->gravity_compensation = 0;
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void gripper_mit_command_value_copy_fields(const gripper_mit_command_t *view, gripper_mit_command_value_t *out) {
  out->has_position = view->has_position;
  out->position = view->has_position ? view->position : 0;
  out->has_velocity = view->has_velocity;
  out->velocity = view->has_velocity ? view->velocity : 0;
  out->has_torque = view->has_torque;
  out->torque = view->has_torque ? view->torque : 0;
  out->has_kp = view->has_kp;
  out->kp = view->has_kp ? view->kp : 0;
  out->has_kd = view->has_kd;
  out->kd = view->has_kd ? view->kd : 0;
  out->has_dt_us = view->has_dt_us;
  out->dt_us = view->has_dt_us ? view->dt_us : UINT32_C(0);
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_gravity_compensation = view->has_gravity_compensation;
  out->gravity_compensation = view->has_gravity_compensation ? view->gravity_compensation : 0;
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void gripper_mit_command_wlc_detail_value_copy(const gripper_mit_command_t *view, gripper_mit_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  gripper_mit_command_value_copy_fields(view, out);
}

void gripper_mit_command_value_clear(gripper_mit_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  gripper_mit_command_value_defaults(value);
}

wl_codec_status_t gripper_mit_command_value_from_view(const gripper_mit_command_t *view, gripper_mit_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&gripper_mit_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  gripper_mit_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t gripper_mit_command_value_borrow(const gripper_mit_command_value_t *value, gripper_mit_command_t *out) {
  gripper_mit_command_clear(out);
  out->has_position = value->has_position;
  if (value->has_position) {
    out->position = value->position;
  }
  out->has_velocity = value->has_velocity;
  if (value->has_velocity) {
    out->velocity = value->velocity;
  }
  out->has_torque = value->has_torque;
  if (value->has_torque) {
    out->torque = value->torque;
  }
  out->has_kp = value->has_kp;
  if (value->has_kp) {
    out->kp = value->kp;
  }
  out->has_kd = value->has_kd;
  if (value->has_kd) {
    out->kd = value->kd;
  }
  out->has_dt_us = value->has_dt_us;
  if (value->has_dt_us) {
    out->dt_us = value->dt_us;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_gravity_compensation = value->has_gravity_compensation;
  if (value->has_gravity_compensation) {
    out->gravity_compensation = value->gravity_compensation;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t gripper_mit_command_value_to_view(const gripper_mit_command_value_t *value, gripper_mit_command_t *out) {
  gripper_mit_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_mit_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&gripper_mit_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t gripper_mit_command_value_encoded_size(const gripper_mit_command_value_t *value) {
  gripper_mit_command_t view;
  if (value == NULL || gripper_mit_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return gripper_mit_command_encoded_size(&view);
}

wl_codec_status_t gripper_mit_command_value_encode(const gripper_mit_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  gripper_mit_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_mit_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? gripper_mit_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t gripper_mit_command_value_decode(const uint8_t *input, size_t length, gripper_mit_command_value_t *out) {
  gripper_mit_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_mit_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  gripper_mit_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void joint_position_velocity_command_value_defaults(joint_position_velocity_command_value_t *out) {
  (void)out;
  out->enabled_mask = 0;
  out->sequence = UINT32_C(0);
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void joint_position_velocity_command_value_copy_fields(const joint_position_velocity_command_t *view, joint_position_velocity_command_value_t *out) {
  out->has_position = view->has_position;
  if (view->has_position) memcpy(out->position, view->position, sizeof(out->position));
  out->has_velocity = view->has_velocity;
  if (view->has_velocity) memcpy(out->velocity, view->velocity, sizeof(out->velocity));
  out->has_enabled_mask = view->has_enabled_mask;
  out->enabled_mask = view->has_enabled_mask ? view->enabled_mask : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void joint_position_velocity_command_wlc_detail_value_copy(const joint_position_velocity_command_t *view, joint_position_velocity_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  joint_position_velocity_command_value_copy_fields(view, out);
}

void joint_position_velocity_command_value_clear(joint_position_velocity_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  joint_position_velocity_command_value_defaults(value);
}

wl_codec_status_t joint_position_velocity_command_value_from_view(const joint_position_velocity_command_t *view, joint_position_velocity_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&joint_position_velocity_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  joint_position_velocity_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t joint_position_velocity_command_value_borrow(const joint_position_velocity_command_value_t *value, joint_position_velocity_command_t *out) {
  joint_position_velocity_command_clear(out);
  out->has_position = value->has_position;
  if (value->has_position) {
    memcpy(out->position, value->position, sizeof(out->position));
  }
  out->has_velocity = value->has_velocity;
  if (value->has_velocity) {
    memcpy(out->velocity, value->velocity, sizeof(out->velocity));
  }
  out->has_enabled_mask = value->has_enabled_mask;
  if (value->has_enabled_mask) {
    out->enabled_mask = value->enabled_mask;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t joint_position_velocity_command_value_to_view(const joint_position_velocity_command_value_t *value, joint_position_velocity_command_t *out) {
  joint_position_velocity_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_position_velocity_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&joint_position_velocity_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t joint_position_velocity_command_value_encoded_size(const joint_position_velocity_command_value_t *value) {
  joint_position_velocity_command_t view;
  if (value == NULL || joint_position_velocity_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return joint_position_velocity_command_encoded_size(&view);
}

wl_codec_status_t joint_position_velocity_command_value_encode(const joint_position_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  joint_position_velocity_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_position_velocity_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? joint_position_velocity_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t joint_position_velocity_command_value_decode(const uint8_t *input, size_t length, joint_position_velocity_command_value_t *out) {
  joint_position_velocity_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_position_velocity_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  joint_position_velocity_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void joint_velocity_command_value_defaults(joint_velocity_command_value_t *out) {
  (void)out;
  out->enabled_mask = 0;
  out->sequence = UINT32_C(0);
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void joint_velocity_command_value_copy_fields(const joint_velocity_command_t *view, joint_velocity_command_value_t *out) {
  out->has_velocity = view->has_velocity;
  if (view->has_velocity) memcpy(out->velocity, view->velocity, sizeof(out->velocity));
  out->has_enabled_mask = view->has_enabled_mask;
  out->enabled_mask = view->has_enabled_mask ? view->enabled_mask : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void joint_velocity_command_wlc_detail_value_copy(const joint_velocity_command_t *view, joint_velocity_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  joint_velocity_command_value_copy_fields(view, out);
}

void joint_velocity_command_value_clear(joint_velocity_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  joint_velocity_command_value_defaults(value);
}

wl_codec_status_t joint_velocity_command_value_from_view(const joint_velocity_command_t *view, joint_velocity_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&joint_velocity_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  joint_velocity_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t joint_velocity_command_value_borrow(const joint_velocity_command_value_t *value, joint_velocity_command_t *out) {
  joint_velocity_command_clear(out);
  out->has_velocity = value->has_velocity;
  if (value->has_velocity) {
    memcpy(out->velocity, value->velocity, sizeof(out->velocity));
  }
  out->has_enabled_mask = value->has_enabled_mask;
  if (value->has_enabled_mask) {
    out->enabled_mask = value->enabled_mask;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t joint_velocity_command_value_to_view(const joint_velocity_command_value_t *value, joint_velocity_command_t *out) {
  joint_velocity_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_velocity_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&joint_velocity_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t joint_velocity_command_value_encoded_size(const joint_velocity_command_value_t *value) {
  joint_velocity_command_t view;
  if (value == NULL || joint_velocity_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return joint_velocity_command_encoded_size(&view);
}

wl_codec_status_t joint_velocity_command_value_encode(const joint_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  joint_velocity_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_velocity_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? joint_velocity_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t joint_velocity_command_value_decode(const uint8_t *input, size_t length, joint_velocity_command_value_t *out) {
  joint_velocity_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_velocity_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  joint_velocity_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void joint_pvt_command_value_defaults(joint_pvt_command_value_t *out) {
  (void)out;
  out->enabled_mask = 0;
  out->sequence = UINT32_C(0);
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void joint_pvt_command_value_copy_fields(const joint_pvt_command_t *view, joint_pvt_command_value_t *out) {
  out->has_position = view->has_position;
  if (view->has_position) memcpy(out->position, view->position, sizeof(out->position));
  out->has_velocity_limit = view->has_velocity_limit;
  if (view->has_velocity_limit) memcpy(out->velocity_limit, view->velocity_limit, sizeof(out->velocity_limit));
  out->has_current_limit_normalized = view->has_current_limit_normalized;
  if (view->has_current_limit_normalized) memcpy(out->current_limit_normalized, view->current_limit_normalized, sizeof(out->current_limit_normalized));
  out->has_enabled_mask = view->has_enabled_mask;
  out->enabled_mask = view->has_enabled_mask ? view->enabled_mask : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void joint_pvt_command_wlc_detail_value_copy(const joint_pvt_command_t *view, joint_pvt_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  joint_pvt_command_value_copy_fields(view, out);
}

void joint_pvt_command_value_clear(joint_pvt_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  joint_pvt_command_value_defaults(value);
}

wl_codec_status_t joint_pvt_command_value_from_view(const joint_pvt_command_t *view, joint_pvt_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&joint_pvt_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  joint_pvt_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t joint_pvt_command_value_borrow(const joint_pvt_command_value_t *value, joint_pvt_command_t *out) {
  joint_pvt_command_clear(out);
  out->has_position = value->has_position;
  if (value->has_position) {
    memcpy(out->position, value->position, sizeof(out->position));
  }
  out->has_velocity_limit = value->has_velocity_limit;
  if (value->has_velocity_limit) {
    memcpy(out->velocity_limit, value->velocity_limit, sizeof(out->velocity_limit));
  }
  out->has_current_limit_normalized = value->has_current_limit_normalized;
  if (value->has_current_limit_normalized) {
    memcpy(out->current_limit_normalized, value->current_limit_normalized, sizeof(out->current_limit_normalized));
  }
  out->has_enabled_mask = value->has_enabled_mask;
  if (value->has_enabled_mask) {
    out->enabled_mask = value->enabled_mask;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t joint_pvt_command_value_to_view(const joint_pvt_command_value_t *value, joint_pvt_command_t *out) {
  joint_pvt_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_pvt_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&joint_pvt_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t joint_pvt_command_value_encoded_size(const joint_pvt_command_value_t *value) {
  joint_pvt_command_t view;
  if (value == NULL || joint_pvt_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return joint_pvt_command_encoded_size(&view);
}

wl_codec_status_t joint_pvt_command_value_encode(const joint_pvt_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  joint_pvt_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_pvt_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? joint_pvt_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t joint_pvt_command_value_decode(const uint8_t *input, size_t length, joint_pvt_command_value_t *out) {
  joint_pvt_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = joint_pvt_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  joint_pvt_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void cartesian_pose_command_value_defaults(cartesian_pose_command_value_t *out) {
  (void)out;
  out->dt_us = UINT32_C(0);
  out->sequence = UINT32_C(0);
  out->gravity_compensation = 0;
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void cartesian_pose_command_value_copy_fields(const cartesian_pose_command_t *view, cartesian_pose_command_value_t *out) {
  out->has_transform = view->has_transform;
  if (view->has_transform) memcpy(out->transform, view->transform, sizeof(out->transform));
  out->has_kp = view->has_kp;
  if (view->has_kp) memcpy(out->kp, view->kp, sizeof(out->kp));
  out->has_kd = view->has_kd;
  if (view->has_kd) memcpy(out->kd, view->kd, sizeof(out->kd));
  out->has_dt_us = view->has_dt_us;
  out->dt_us = view->has_dt_us ? view->dt_us : UINT32_C(0);
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_gravity_compensation = view->has_gravity_compensation;
  out->gravity_compensation = view->has_gravity_compensation ? view->gravity_compensation : 0;
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void cartesian_pose_command_wlc_detail_value_copy(const cartesian_pose_command_t *view, cartesian_pose_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  cartesian_pose_command_value_copy_fields(view, out);
}

void cartesian_pose_command_value_clear(cartesian_pose_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  cartesian_pose_command_value_defaults(value);
}

wl_codec_status_t cartesian_pose_command_value_from_view(const cartesian_pose_command_t *view, cartesian_pose_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&cartesian_pose_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  cartesian_pose_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t cartesian_pose_command_value_borrow(const cartesian_pose_command_value_t *value, cartesian_pose_command_t *out) {
  cartesian_pose_command_clear(out);
  out->has_transform = value->has_transform;
  if (value->has_transform) {
    memcpy(out->transform, value->transform, sizeof(out->transform));
  }
  out->has_kp = value->has_kp;
  if (value->has_kp) {
    memcpy(out->kp, value->kp, sizeof(out->kp));
  }
  out->has_kd = value->has_kd;
  if (value->has_kd) {
    memcpy(out->kd, value->kd, sizeof(out->kd));
  }
  out->has_dt_us = value->has_dt_us;
  if (value->has_dt_us) {
    out->dt_us = value->dt_us;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_gravity_compensation = value->has_gravity_compensation;
  if (value->has_gravity_compensation) {
    out->gravity_compensation = value->gravity_compensation;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t cartesian_pose_command_value_to_view(const cartesian_pose_command_value_t *value, cartesian_pose_command_t *out) {
  cartesian_pose_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = cartesian_pose_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&cartesian_pose_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t cartesian_pose_command_value_encoded_size(const cartesian_pose_command_value_t *value) {
  cartesian_pose_command_t view;
  if (value == NULL || cartesian_pose_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return cartesian_pose_command_encoded_size(&view);
}

wl_codec_status_t cartesian_pose_command_value_encode(const cartesian_pose_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  cartesian_pose_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = cartesian_pose_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? cartesian_pose_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t cartesian_pose_command_value_decode(const uint8_t *input, size_t length, cartesian_pose_command_value_t *out) {
  cartesian_pose_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = cartesian_pose_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  cartesian_pose_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void cartesian_velocity_command_value_defaults(cartesian_velocity_command_value_t *out) {
  (void)out;
  out->dt_us = UINT32_C(0);
  out->sequence = UINT32_C(0);
  out->gravity_compensation = 0;
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void cartesian_velocity_command_value_copy_fields(const cartesian_velocity_command_t *view, cartesian_velocity_command_value_t *out) {
  out->has_twist = view->has_twist;
  if (view->has_twist) memcpy(out->twist, view->twist, sizeof(out->twist));
  out->has_kp = view->has_kp;
  if (view->has_kp) memcpy(out->kp, view->kp, sizeof(out->kp));
  out->has_kd = view->has_kd;
  if (view->has_kd) memcpy(out->kd, view->kd, sizeof(out->kd));
  out->has_dt_us = view->has_dt_us;
  out->dt_us = view->has_dt_us ? view->dt_us : UINT32_C(0);
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_gravity_compensation = view->has_gravity_compensation;
  out->gravity_compensation = view->has_gravity_compensation ? view->gravity_compensation : 0;
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void cartesian_velocity_command_wlc_detail_value_copy(const cartesian_velocity_command_t *view, cartesian_velocity_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  cartesian_velocity_command_value_copy_fields(view, out);
}

void cartesian_velocity_command_value_clear(cartesian_velocity_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  cartesian_velocity_command_value_defaults(value);
}

wl_codec_status_t cartesian_velocity_command_value_from_view(const cartesian_velocity_command_t *view, cartesian_velocity_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&cartesian_velocity_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  cartesian_velocity_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t cartesian_velocity_command_value_borrow(const cartesian_velocity_command_value_t *value, cartesian_velocity_command_t *out) {
  cartesian_velocity_command_clear(out);
  out->has_twist = value->has_twist;
  if (value->has_twist) {
    memcpy(out->twist, value->twist, sizeof(out->twist));
  }
  out->has_kp = value->has_kp;
  if (value->has_kp) {
    memcpy(out->kp, value->kp, sizeof(out->kp));
  }
  out->has_kd = value->has_kd;
  if (value->has_kd) {
    memcpy(out->kd, value->kd, sizeof(out->kd));
  }
  out->has_dt_us = value->has_dt_us;
  if (value->has_dt_us) {
    out->dt_us = value->dt_us;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_gravity_compensation = value->has_gravity_compensation;
  if (value->has_gravity_compensation) {
    out->gravity_compensation = value->gravity_compensation;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t cartesian_velocity_command_value_to_view(const cartesian_velocity_command_value_t *value, cartesian_velocity_command_t *out) {
  cartesian_velocity_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = cartesian_velocity_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&cartesian_velocity_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t cartesian_velocity_command_value_encoded_size(const cartesian_velocity_command_value_t *value) {
  cartesian_velocity_command_t view;
  if (value == NULL || cartesian_velocity_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return cartesian_velocity_command_encoded_size(&view);
}

wl_codec_status_t cartesian_velocity_command_value_encode(const cartesian_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  cartesian_velocity_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = cartesian_velocity_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? cartesian_velocity_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t cartesian_velocity_command_value_decode(const uint8_t *input, size_t length, cartesian_velocity_command_value_t *out) {
  cartesian_velocity_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = cartesian_velocity_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  cartesian_velocity_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void gripper_position_velocity_command_value_defaults(gripper_position_velocity_command_value_t *out) {
  (void)out;
  out->position = 0;
  out->velocity = 0;
  out->sequence = UINT32_C(0);
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void gripper_position_velocity_command_value_copy_fields(const gripper_position_velocity_command_t *view, gripper_position_velocity_command_value_t *out) {
  out->has_position = view->has_position;
  out->position = view->has_position ? view->position : 0;
  out->has_velocity = view->has_velocity;
  out->velocity = view->has_velocity ? view->velocity : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void gripper_position_velocity_command_wlc_detail_value_copy(const gripper_position_velocity_command_t *view, gripper_position_velocity_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  gripper_position_velocity_command_value_copy_fields(view, out);
}

void gripper_position_velocity_command_value_clear(gripper_position_velocity_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  gripper_position_velocity_command_value_defaults(value);
}

wl_codec_status_t gripper_position_velocity_command_value_from_view(const gripper_position_velocity_command_t *view, gripper_position_velocity_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&gripper_position_velocity_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  gripper_position_velocity_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t gripper_position_velocity_command_value_borrow(const gripper_position_velocity_command_value_t *value, gripper_position_velocity_command_t *out) {
  gripper_position_velocity_command_clear(out);
  out->has_position = value->has_position;
  if (value->has_position) {
    out->position = value->position;
  }
  out->has_velocity = value->has_velocity;
  if (value->has_velocity) {
    out->velocity = value->velocity;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t gripper_position_velocity_command_value_to_view(const gripper_position_velocity_command_value_t *value, gripper_position_velocity_command_t *out) {
  gripper_position_velocity_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_position_velocity_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&gripper_position_velocity_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t gripper_position_velocity_command_value_encoded_size(const gripper_position_velocity_command_value_t *value) {
  gripper_position_velocity_command_t view;
  if (value == NULL || gripper_position_velocity_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return gripper_position_velocity_command_encoded_size(&view);
}

wl_codec_status_t gripper_position_velocity_command_value_encode(const gripper_position_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  gripper_position_velocity_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_position_velocity_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? gripper_position_velocity_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t gripper_position_velocity_command_value_decode(const uint8_t *input, size_t length, gripper_position_velocity_command_value_t *out) {
  gripper_position_velocity_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_position_velocity_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  gripper_position_velocity_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void gripper_velocity_command_value_defaults(gripper_velocity_command_value_t *out) {
  (void)out;
  out->velocity = 0;
  out->sequence = UINT32_C(0);
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void gripper_velocity_command_value_copy_fields(const gripper_velocity_command_t *view, gripper_velocity_command_value_t *out) {
  out->has_velocity = view->has_velocity;
  out->velocity = view->has_velocity ? view->velocity : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void gripper_velocity_command_wlc_detail_value_copy(const gripper_velocity_command_t *view, gripper_velocity_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  gripper_velocity_command_value_copy_fields(view, out);
}

void gripper_velocity_command_value_clear(gripper_velocity_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  gripper_velocity_command_value_defaults(value);
}

wl_codec_status_t gripper_velocity_command_value_from_view(const gripper_velocity_command_t *view, gripper_velocity_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&gripper_velocity_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  gripper_velocity_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t gripper_velocity_command_value_borrow(const gripper_velocity_command_value_t *value, gripper_velocity_command_t *out) {
  gripper_velocity_command_clear(out);
  out->has_velocity = value->has_velocity;
  if (value->has_velocity) {
    out->velocity = value->velocity;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t gripper_velocity_command_value_to_view(const gripper_velocity_command_value_t *value, gripper_velocity_command_t *out) {
  gripper_velocity_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_velocity_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&gripper_velocity_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t gripper_velocity_command_value_encoded_size(const gripper_velocity_command_value_t *value) {
  gripper_velocity_command_t view;
  if (value == NULL || gripper_velocity_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return gripper_velocity_command_encoded_size(&view);
}

wl_codec_status_t gripper_velocity_command_value_encode(const gripper_velocity_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  gripper_velocity_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_velocity_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? gripper_velocity_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t gripper_velocity_command_value_decode(const uint8_t *input, size_t length, gripper_velocity_command_value_t *out) {
  gripper_velocity_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_velocity_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  gripper_velocity_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

static void gripper_pvt_command_value_defaults(gripper_pvt_command_value_t *out) {
  (void)out;
  out->position = 0;
  out->velocity_limit = 0;
  out->current_limit_normalized = 0;
  out->sequence = UINT32_C(0);
  out->sdk_timestamp_us = UINT64_C(0);
  out->lease_token = UINT64_C(0);
}

static void gripper_pvt_command_value_copy_fields(const gripper_pvt_command_t *view, gripper_pvt_command_value_t *out) {
  out->has_position = view->has_position;
  out->position = view->has_position ? view->position : 0;
  out->has_velocity_limit = view->has_velocity_limit;
  out->velocity_limit = view->has_velocity_limit ? view->velocity_limit : 0;
  out->has_current_limit_normalized = view->has_current_limit_normalized;
  out->current_limit_normalized = view->has_current_limit_normalized ? view->current_limit_normalized : 0;
  out->has_sequence = view->has_sequence;
  out->sequence = view->has_sequence ? view->sequence : UINT32_C(0);
  out->has_sdk_timestamp_us = view->has_sdk_timestamp_us;
  out->sdk_timestamp_us = view->has_sdk_timestamp_us ? view->sdk_timestamp_us : UINT64_C(0);
  out->has_lease_token = view->has_lease_token;
  out->lease_token = view->has_lease_token ? view->lease_token : UINT64_C(0);
}

/* Generator-private: input is an unmodified successful decode, or has been measured. */
void gripper_pvt_command_wlc_detail_value_copy(const gripper_pvt_command_t *view, gripper_pvt_command_value_t *out) {
  memset(out, 0, sizeof(*out));
  gripper_pvt_command_value_copy_fields(view, out);
}

void gripper_pvt_command_value_clear(gripper_pvt_command_value_t *value) {
  if (value == NULL) return;
  memset(value, 0, sizeof(*value));
  gripper_pvt_command_value_defaults(value);
}

wl_codec_status_t gripper_pvt_command_value_from_view(const gripper_pvt_command_t *view, gripper_pvt_command_value_t *out) {
  size_t size;
  wl_codec_status_t status;
  if (view == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = wlc_measure(&gripper_pvt_command_desc, view, &size);
  if (status != WL_CODEC_OK) return status;
  gripper_pvt_command_wlc_detail_value_copy(view, out);
  return WL_CODEC_OK;
}

/* Private conversion does not re-validate each nested subtree. */
static wl_codec_status_t gripper_pvt_command_value_borrow(const gripper_pvt_command_value_t *value, gripper_pvt_command_t *out) {
  gripper_pvt_command_clear(out);
  out->has_position = value->has_position;
  if (value->has_position) {
    out->position = value->position;
  }
  out->has_velocity_limit = value->has_velocity_limit;
  if (value->has_velocity_limit) {
    out->velocity_limit = value->velocity_limit;
  }
  out->has_current_limit_normalized = value->has_current_limit_normalized;
  if (value->has_current_limit_normalized) {
    out->current_limit_normalized = value->current_limit_normalized;
  }
  out->has_sequence = value->has_sequence;
  if (value->has_sequence) {
    out->sequence = value->sequence;
  }
  out->has_sdk_timestamp_us = value->has_sdk_timestamp_us;
  if (value->has_sdk_timestamp_us) {
    out->sdk_timestamp_us = value->sdk_timestamp_us;
  }
  out->has_lease_token = value->has_lease_token;
  if (value->has_lease_token) {
    out->lease_token = value->lease_token;
  }
  return WL_CODEC_OK;
}

wl_codec_status_t gripper_pvt_command_value_to_view(const gripper_pvt_command_value_t *value, gripper_pvt_command_t *out) {
  gripper_pvt_command_t view;
  size_t size;
  wl_codec_status_t status;
  if (value == NULL || out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_pvt_command_value_borrow(value, &view);
  if (status == WL_CODEC_OK) status = wlc_measure(&gripper_pvt_command_desc, &view, &size);
  if (status != WL_CODEC_OK) return status;
  *out = view;
  return WL_CODEC_OK;
}

size_t gripper_pvt_command_value_encoded_size(const gripper_pvt_command_value_t *value) {
  gripper_pvt_command_t view;
  if (value == NULL || gripper_pvt_command_value_borrow(value, &view) != WL_CODEC_OK) return SIZE_MAX;
  return gripper_pvt_command_encoded_size(&view);
}

wl_codec_status_t gripper_pvt_command_value_encode(const gripper_pvt_command_value_t *value, uint8_t *out, size_t capacity, size_t *length) {
  gripper_pvt_command_t view;
  wl_codec_status_t status;
  if (value == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_pvt_command_value_borrow(value, &view);
  return status == WL_CODEC_OK ? gripper_pvt_command_encode(&view, out, capacity, length) : status;
}

wl_codec_status_t gripper_pvt_command_value_decode(const uint8_t *input, size_t length, gripper_pvt_command_value_t *out) {
  gripper_pvt_command_t view;
  wl_codec_status_t status;
  if (out == NULL) return WL_CODEC_ERR_INVALID_VALUE;
  status = gripper_pvt_command_decode(input, length, &view);
  if (status != WL_CODEC_OK) return status;
  gripper_pvt_command_wlc_detail_value_copy(&view, out);
  return WL_CODEC_OK;
}

