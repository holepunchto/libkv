#include <assert.h>
#include <stdio.h>

#include "../include/kv.h"

#define N 1000000

int
main () {
  kv_t kv;
  kv_init(&kv);

  for (int i = 0; i < N; i++) {
    uint8_t key[4];
    key[0] = (i >> 24) & 0xff;
    key[1] = (i >> 16) & 0xff;
    key[2] = (i >> 8) & 0xff;
    key[3] = i & 0xff;
    kv_put(&kv, key, 4, key, 4);
  }

  assert(kv.len == N);

  for (int i = 0; i < N; i++) {
    uint8_t key[4];
    key[0] = (i >> 24) & 0xff;
    key[1] = (i >> 16) & 0xff;
    key[2] = (i >> 8) & 0xff;
    key[3] = i & 0xff;

    const uint8_t *val;
    size_t val_len;

    assert(kv_get(&kv, key, 4, &val, &val_len) == 0);
    assert(val_len == 4);
    assert(val[0] == key[0] && val[1] == key[1] && val[2] == key[2] && val[3] == key[3]);
  }

  kv_destroy(&kv);
}
