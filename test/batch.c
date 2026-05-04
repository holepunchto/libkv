#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../include/kv.h"

typedef struct {
  kv_status_t status;
  const uint8_t *key;
  size_t key_len;
  uint8_t *val;
  size_t val_len;
  int hits;
} cb_result_t;

static int
on_read (kv_status_t status, const uint8_t *key, size_t key_len, uint8_t *val, size_t val_len, void *data) {
  cb_result_t *r = data;
  r->status = status;
  r->key = key;
  r->key_len = key_len;
  r->val = val;
  r->val_len = val_len;
  r->hits++;
  return 0;
}

static int
on_read_fail (kv_status_t status, const uint8_t *key, size_t key_len, uint8_t *val, size_t val_len, void *data) {
  free(val);
  return *(int *) data;
}

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

  uint8_t *va = NULL, *vb = NULL, *vx = NULL, *vmiss = NULL;
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
  assert(vmiss == NULL && lmiss == 0);

  free(va);
  free(vb);
  free(vx);

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
  uint8_t *vals[100] = {0};
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
    free(vals[i]);
  }

  kv_read_batch_destroy(&rb2);

  // callback variant
  cb_result_t ra = {0}, rmiss = {0};

  kv_read_batch_t rb3;
  kv_read_batch_init(&rb3, &kv, 2);

  kv_read_batch_get_cb(&rb3, (uint8_t *) "a", 1, on_read, &ra);
  kv_read_batch_get_cb(&rb3, (uint8_t *) "missing", 7, on_read, &rmiss);

  assert(ra.hits == 0);

  kv_read_batch_flush(&rb3);

  assert(ra.hits == 1);
  assert(ra.status == KV_OK);
  assert(ra.key_len == 1 && ra.key[0] == 'a');
  assert(ra.val_len == 1 && ra.val[0] == '1');

  assert(rmiss.hits == 1);
  assert(rmiss.status == KV_NOT_FOUND);
  assert(rmiss.val == NULL && rmiss.val_len == 0);

  free(ra.val);

  kv_read_batch_destroy(&rb3);

  // kv_get_cb single-shot, cb return forwarded
  cb_result_t single = {0};
  int rc = kv_get_cb(&kv, (uint8_t *) "a", 1, on_read, &single);
  assert(rc == 0);
  assert(single.hits == 1 && single.status == KV_OK);
  assert(single.val_len == 1 && single.val[0] == '1');
  free(single.val);

  // cb returning non-zero is forwarded as kv_get_cb's return
  int code = 42;
  rc = kv_get_cb(&kv, (uint8_t *) "a", 1, on_read_fail, &code);
  assert(rc == 42);

  // batch flush forwards first non-zero cb return; all cbs still fire
  cb_result_t r1 = {0}, r2 = {0};
  int code7 = 7, code9 = 9;

  kv_read_batch_t rb4;
  kv_read_batch_init(&rb4, &kv, 4);
  kv_read_batch_get_cb(&rb4, (uint8_t *) "a", 1, on_read, &r1);
  kv_read_batch_get_cb(&rb4, (uint8_t *) "b", 1, on_read_fail, &code7);
  kv_read_batch_get_cb(&rb4, (uint8_t *) "c", 1, on_read_fail, &code9);
  kv_read_batch_get_cb(&rb4, (uint8_t *) "x", 1, on_read, &r2);

  rc = kv_read_batch_flush(&rb4);
  assert(rc == 7);
  assert(r1.hits == 1);
  assert(r2.hits == 1);
  free(r1.val);
  free(r2.val);

  kv_read_batch_destroy(&rb4);
  kv_destroy(&kv);
}
