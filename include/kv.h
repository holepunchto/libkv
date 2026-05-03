#ifndef KV_H
#define KV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct kv_entry_s kv_entry_t;
typedef struct kv_s kv_t;

struct kv_entry_s {
  uint8_t *key;
  size_t key_len;
  uint8_t *val;
  size_t val_len;
};

struct kv_s {
  kv_entry_t *entries;
  size_t len;
  size_t capacity;
};

void
kv_init (kv_t *kv);

void
kv_destroy (kv_t *kv);

int
kv_put (kv_t *kv, const uint8_t *key, size_t key_len, const uint8_t *val, size_t val_len);

int
kv_get (kv_t *kv, const uint8_t *key, size_t key_len, const uint8_t **val, size_t *val_len);

int
kv_del (kv_t *kv, const uint8_t *key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif // KV_H
