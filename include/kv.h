#ifndef KV_H
#define KV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <stdbool.h>

typedef enum {
  KV_OK = 0,
  KV_NOT_FOUND = 1,
  KV_ERROR = -1,
} kv_status_t;

typedef struct kv_entry_s kv_entry_t;
typedef struct kv_s kv_t;
typedef struct kv_write_op_s kv_write_op_t;
typedef struct kv_write_batch_s kv_write_batch_t;
typedef struct kv_read_req_s kv_read_req_t;
typedef struct kv_read_batch_s kv_read_batch_t;

typedef int (*kv_read_cb)(kv_status_t status, const uint8_t *key, size_t key_len, uint8_t *val, size_t val_len, void *data);

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

struct kv_write_op_s {
  bool del;
  uint8_t *key;
  size_t key_len;
  uint8_t *val;
  size_t val_len;
};

struct kv_write_batch_s {
  kv_t *kv;
  kv_write_op_t *ops;
  size_t len;
  size_t capacity;
};

struct kv_read_req_s {
  const uint8_t *key;
  size_t key_len;
  uint8_t **val;
  size_t *val_len;
  kv_read_cb cb;
  void *data;
};

struct kv_read_batch_s {
  kv_t *kv;
  kv_read_req_t *reqs;
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
kv_get (kv_t *kv, const uint8_t *key, size_t key_len, uint8_t **val, size_t *val_len);

int
kv_get_cb (kv_t *kv, const uint8_t *key, size_t key_len, kv_read_cb cb, void *data);

int
kv_del (kv_t *kv, const uint8_t *key, size_t key_len);

void
kv_write_batch_init (kv_write_batch_t *batch, kv_t *kv, size_t hint);

void
kv_write_batch_destroy (kv_write_batch_t *batch);

int
kv_write_batch_put (kv_write_batch_t *batch, const uint8_t *key, size_t key_len, const uint8_t *val, size_t val_len);

int
kv_write_batch_del (kv_write_batch_t *batch, const uint8_t *key, size_t key_len);

int
kv_write_batch_flush (kv_write_batch_t *batch);

void
kv_read_batch_init (kv_read_batch_t *batch, kv_t *kv, size_t hint);

void
kv_read_batch_destroy (kv_read_batch_t *batch);

int
kv_read_batch_get (kv_read_batch_t *batch, const uint8_t *key, size_t key_len, uint8_t **val, size_t *val_len);

int
kv_read_batch_get_cb (kv_read_batch_t *batch, const uint8_t *key, size_t key_len, kv_read_cb cb, void *data);

int
kv_read_batch_flush (kv_read_batch_t *batch);

#ifdef __cplusplus
}
#endif

#endif // KV_H
