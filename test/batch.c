#include <assert.h>
#include <string.h>

#include "../include/kv.h"

int
main () {
  kv_t kv;
  kv_init(&kv);

  kv_put(&kv, (uint8_t *) "x", 1, (uint8_t *) "old", 3);

  kv_write_batch_t wb;
  kv_write_batch_init(&wb, &kv, 4);

  kv_write_batch_put(&wb, (uint8_t *) "a", 1, (uint8_t *) "1", 1);
  kv_write_batch_put(&wb, (uint8_t *) "b", 1, (uint8_t *) "2", 1);
  kv_write_batch_put(&wb, (uint8_t *) "x", 1, (uint8_t *) "new", 3);
  kv_write_batch_del(&wb, (uint8_t *) "missing", 7);

  assert(kv.len == 1);

  kv_write_batch_flush(&wb);

  assert(kv.len == 3);
  kv_write_batch_destroy(&wb);

  const uint8_t *va = NULL, *vb = NULL, *vx = NULL, *vmiss = NULL;
  size_t la = 0, lb = 0, lx = 0, lmiss = 0;

  kv_read_batch_t rb;
  kv_read_batch_init(&rb, &kv, 4);

  kv_read_batch_get(&rb, (uint8_t *) "a", 1, &va, &la);
  kv_read_batch_get(&rb, (uint8_t *) "b", 1, &vb, &lb);
  kv_read_batch_get(&rb, (uint8_t *) "x", 1, &vx, &lx);
  kv_read_batch_get(&rb, (uint8_t *) "missing", 7, &vmiss, &lmiss);

  assert(va == NULL);

  kv_read_batch_flush(&rb);

  assert(la == 1 && va[0] == '1');
  assert(lb == 1 && vb[0] == '2');
  assert(lx == 3 && memcmp(vx, "new", 3) == 0);
  assert(vmiss == NULL);

  kv_read_batch_destroy(&rb);

  // grow: hint of 0, queue many ops to force capacity doubling
  kv_write_batch_t wb2;
  kv_write_batch_init(&wb2, &kv, 0);
  assert(wb2.capacity == 0);

  for (int i = 0; i < 100; i++) {
    uint8_t key[2] = {'k', (uint8_t) i};
    uint8_t val[2] = {'v', (uint8_t) i};
    kv_write_batch_put(&wb2, key, 2, val, 2);
  }

  assert(wb2.len == 100);
  assert(wb2.capacity >= 100);

  kv_write_batch_flush(&wb2);
  kv_write_batch_destroy(&wb2);

  kv_read_batch_t rb2;
  kv_read_batch_init(&rb2, &kv, 0);

  uint8_t keys[100][2];
  const uint8_t *vals[100] = {0};
  size_t lens[100] = {0};

  for (int i = 0; i < 100; i++) {
    keys[i][0] = 'k';
    keys[i][1] = (uint8_t) i;
    kv_read_batch_get(&rb2, keys[i], 2, &vals[i], &lens[i]);
  }

  assert(rb2.capacity >= 100);

  kv_read_batch_flush(&rb2);

  for (int i = 0; i < 100; i++) {
    assert(lens[i] == 2);
    assert(vals[i][0] == 'v' && vals[i][1] == (uint8_t) i);
  }

  kv_read_batch_destroy(&rb2);
  kv_destroy(&kv);
}
