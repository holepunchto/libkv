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

int
kv_put (kv_t *kv, const uint8_t *key, size_t key_len, const uint8_t *val, size_t val_len) {
  bool found;
  size_t i = kv__search(kv, key, key_len, &found);

  if (found) {
    uint8_t *new_val = malloc(val_len);
    if (new_val == NULL) return -1;
    free(kv->entries[i].val);
    memcpy(new_val, val, val_len);
    kv->entries[i].val = new_val;
    kv->entries[i].val_len = val_len;
    return 0;
  }

  if (kv->len == kv->capacity) {
    size_t new_capacity = kv->capacity == 0 ? 4 : kv->capacity * 2;
    kv_entry_t *new_entries = realloc(kv->entries, new_capacity * sizeof(kv_entry_t));
    if (new_entries == NULL) return -1;
    kv->entries = new_entries;
    kv->capacity = new_capacity;
  }

  uint8_t *new_key = malloc(key_len);
  uint8_t *new_val = malloc(val_len);

  if (new_key == NULL || new_val == NULL) {
    free(new_key);
    free(new_val);
    return -1;
  }

  memcpy(new_key, key, key_len);
  memcpy(new_val, val, val_len);

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
