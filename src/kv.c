#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../include/kv.h"

static int
kv__compare (const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len) {
  int cmp = memcmp(a, b, a_len < b_len ? a_len : b_len);
  if (cmp != 0) return cmp;
  if (a_len < b_len) return -1;
  if (a_len > b_len) return 1;
  return 0;
}

static size_t
kv__search (kv_t *kv, const uint8_t *key, size_t key_len, bool *found) {
  size_t lo = 0;
  size_t hi = kv->len;

  *found = false;

  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    kv_entry_t *e = &kv->entries[mid];
    int cmp = kv__compare(key, key_len, e->key, e->key_len);

    if (cmp == 0) {
      *found = true;
      return mid;
    } else if (cmp < 0) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }

  return lo;
}

void
kv_init (kv_t *kv) {
  kv->entries = NULL;
  kv->len = 0;
  kv->capacity = 0;
}

void
kv_destroy (kv_t *kv) {
  for (size_t i = 0; i < kv->len; i++) {
    free(kv->entries[i].key);
    free(kv->entries[i].val);
  }
  free(kv->entries);
  kv->entries = NULL;
  kv->len = 0;
  kv->capacity = 0;
}

static int
kv__put (kv_t *kv, uint8_t *new_key, size_t key_len, uint8_t *new_val, size_t val_len) {
  bool found;
  size_t i = kv__search(kv, new_key, key_len, &found);

  if (found) {
    free(new_key);
    free(kv->entries[i].val);
    kv->entries[i].val = new_val;
    kv->entries[i].val_len = val_len;
    return 0;
  }

  if (kv->len == kv->capacity) {
    size_t new_capacity = kv->capacity == 0 ? 4 : kv->capacity * 2;
    kv_entry_t *new_entries = realloc(kv->entries, new_capacity * sizeof(kv_entry_t));
    if (new_entries == NULL) {
      free(new_key);
      free(new_val);
      return -1;
    }
    kv->entries = new_entries;
    kv->capacity = new_capacity;
  }

  memmove(&kv->entries[i + 1], &kv->entries[i], (kv->len - i) * sizeof(kv_entry_t));

  kv->entries[i] = (kv_entry_t){
    .key = new_key,
    .key_len = key_len,
    .val = new_val,
    .val_len = val_len,
  };

  kv->len++;

  return 0;
}

int
kv_put (kv_t *kv, const uint8_t *key, size_t key_len, const uint8_t *val, size_t val_len) {
  uint8_t *new_key = malloc(key_len);
  uint8_t *new_val = malloc(val_len);

  if (new_key == NULL || new_val == NULL) {
    free(new_key);
    free(new_val);
    return -1;
  }

  memcpy(new_key, key, key_len);
  memcpy(new_val, val, val_len);

  return kv__put(kv, new_key, key_len, new_val, val_len);
}

int
kv_get (kv_t *kv, const uint8_t *key, size_t key_len, const uint8_t **val, size_t *val_len) {
  bool found;
  size_t i = kv__search(kv, key, key_len, &found);

  if (!found) return -1;

  if (val) *val = kv->entries[i].val;
  if (val_len) *val_len = kv->entries[i].val_len;

  return 0;
}

int
kv_del (kv_t *kv, const uint8_t *key, size_t key_len) {
  bool found;
  size_t i = kv__search(kv, key, key_len, &found);

  if (!found) return -1;

  free(kv->entries[i].key);
  free(kv->entries[i].val);

  memmove(&kv->entries[i], &kv->entries[i + 1], (kv->len - i - 1) * sizeof(kv_entry_t));

  kv->len--;

  return 0;
}

void
kv_write_batch_init (kv_write_batch_t *batch, kv_t *kv, size_t hint) {
  batch->kv = kv;
  batch->ops = NULL;
  batch->len = 0;
  batch->capacity = 0;

  if (hint > 0) {
    batch->ops = malloc(hint * sizeof(kv_write_op_t));
    if (batch->ops != NULL) batch->capacity = hint;
  }
}

void
kv_write_batch_destroy (kv_write_batch_t *batch) {
  for (size_t i = 0; i < batch->len; i++) {
    free(batch->ops[i].key);
    free(batch->ops[i].val);
  }
  free(batch->ops);
  batch->ops = NULL;
  batch->len = 0;
  batch->capacity = 0;
}

static int
kv__write_batch_grow (kv_write_batch_t *batch) {
  if (batch->len < batch->capacity) return 0;
  size_t new_capacity = batch->capacity == 0 ? 4 : batch->capacity * 2;
  kv_write_op_t *new_ops = realloc(batch->ops, new_capacity * sizeof(kv_write_op_t));
  if (new_ops == NULL) return -1;
  batch->ops = new_ops;
  batch->capacity = new_capacity;
  return 0;
}

int
kv_write_batch_put (kv_write_batch_t *batch, const uint8_t *key, size_t key_len, const uint8_t *val, size_t val_len) {
  if (kv__write_batch_grow(batch) < 0) return -1;

  uint8_t *new_key = malloc(key_len);
  uint8_t *new_val = malloc(val_len);

  if (new_key == NULL || new_val == NULL) {
    free(new_key);
    free(new_val);
    return -1;
  }

  memcpy(new_key, key, key_len);
  memcpy(new_val, val, val_len);

  batch->ops[batch->len++] = (kv_write_op_t){
    .del = false,
    .key = new_key,
    .key_len = key_len,
    .val = new_val,
    .val_len = val_len,
  };

  return 0;
}

int
kv_write_batch_del (kv_write_batch_t *batch, const uint8_t *key, size_t key_len) {
  if (kv__write_batch_grow(batch) < 0) return -1;

  uint8_t *new_key = malloc(key_len);
  if (new_key == NULL) return -1;
  memcpy(new_key, key, key_len);

  batch->ops[batch->len++] = (kv_write_op_t){
    .del = true,
    .key = new_key,
    .key_len = key_len,
    .val = NULL,
    .val_len = 0,
  };

  return 0;
}

int
kv_write_batch_flush (kv_write_batch_t *batch) {
  int err = 0;
  size_t i = 0;

  for (; i < batch->len; i++) {
    kv_write_op_t *op = &batch->ops[i];
    if (op->del) {
      kv_del(batch->kv, op->key, op->key_len);
      free(op->key);
    } else {
      if (kv__put(batch->kv, op->key, op->key_len, op->val, op->val_len) < 0) {
        err = -1;
        i++;
        break;
      }
    }
  }

  for (; i < batch->len; i++) {
    free(batch->ops[i].key);
    free(batch->ops[i].val);
  }

  batch->len = 0;
  return err;
}

void
kv_read_batch_init (kv_read_batch_t *batch, kv_t *kv, size_t hint) {
  batch->kv = kv;
  batch->reqs = NULL;
  batch->len = 0;
  batch->capacity = 0;

  if (hint > 0) {
    batch->reqs = malloc(hint * sizeof(kv_read_req_t));
    if (batch->reqs != NULL) batch->capacity = hint;
  }
}

void
kv_read_batch_destroy (kv_read_batch_t *batch) {
  free(batch->reqs);
  batch->reqs = NULL;
  batch->len = 0;
  batch->capacity = 0;
}

int
kv_read_batch_get (kv_read_batch_t *batch, const uint8_t *key, size_t key_len, const uint8_t **val, size_t *val_len) {
  if (batch->len == batch->capacity) {
    size_t new_capacity = batch->capacity == 0 ? 4 : batch->capacity * 2;
    kv_read_req_t *new_reqs = realloc(batch->reqs, new_capacity * sizeof(kv_read_req_t));
    if (new_reqs == NULL) return -1;
    batch->reqs = new_reqs;
    batch->capacity = new_capacity;
  }

  batch->reqs[batch->len++] = (kv_read_req_t){
    .key = key,
    .key_len = key_len,
    .val = val,
    .val_len = val_len,
  };

  return 0;
}

int
kv_read_batch_flush (kv_read_batch_t *batch) {
  for (size_t i = 0; i < batch->len; i++) {
    kv_read_req_t *req = &batch->reqs[i];
    kv_get(batch->kv, req->key, req->key_len, req->val, req->val_len);
  }
  batch->len = 0;
  return 0;
}
