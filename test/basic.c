#include <assert.h>
#include <string.h>

#include "../include/kv.h"

int
main () {
  kv_t kv;
  kv_init(&kv);

  kv_put(&kv, (uint8_t *) "b", 1, (uint8_t *) "2", 1);
  kv_put(&kv, (uint8_t *) "a", 1, (uint8_t *) "1", 1);
  kv_put(&kv, (uint8_t *) "c", 1, (uint8_t *) "3", 1);

  assert(kv.len == 3);
  assert(kv.entries[0].key[0] == 'a');
  assert(kv.entries[1].key[0] == 'b');
  assert(kv.entries[2].key[0] == 'c');

  const uint8_t *val;
  size_t val_len;

  kv_get(&kv, (uint8_t *) "b", 1, &val, &val_len);
  assert(val_len == 1 && val[0] == '2');

  kv_put(&kv, (uint8_t *) "b", 1, (uint8_t *) "99", 2);
  kv_get(&kv, (uint8_t *) "b", 1, &val, &val_len);
  assert(val_len == 2 && memcmp(val, "99", 2) == 0);

  kv_del(&kv, (uint8_t *) "b", 1);
  assert(kv.len == 2);
  assert(kv_get(&kv, (uint8_t *) "b", 1, NULL, NULL) == -1);
  assert(kv_del(&kv, (uint8_t *) "b", 1) == -1);

  kv_destroy(&kv);
}
