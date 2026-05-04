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
  kv_destroy(&kv);
}
