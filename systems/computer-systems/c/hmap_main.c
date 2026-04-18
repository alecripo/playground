#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "hmap.c"


int main() {
  hmap_t *h = hmap_new();

  // basic get/set functionality
  int64_t a = 5;
  hmap_set(h, "item a", a);
  int64_t retrieved;
  hmap_err_t err = hmap_get(h, "item a", &retrieved);
  assert(err == NO_ERROR);
  assert(a == retrieved);

  // using the same key should override the previous value
  int64_t c = 20;
  hmap_set(h, "item a", c);
  err = hmap_get(h, "item a", &retrieved);
  assert(err == NO_ERROR);
  assert(retrieved == c);

  // basic delete functionality
  hmap_del(h, "item a");
  assert(hmap_get(h, "item a", &retrieved) == KEY_NOT_FOUND);

  /*
   * TODO:
  // handle collisions correctly
  // note: this doesn't necessarily test expansion
  int i, n = STARTING_BUCKETS * 10, ns[n];
  char key[MAX_KEY_SIZE];
  for (i = 0; i < n; i++) {
    ns[i] = i;
    sprintf(key, "item %d", i);
    Hashmap_set(h, key, &ns[i]);
  }
  for (i = 0; i < n; i++) {
    sprintf(key, "item %d", i);
    assert(Hashmap_get(h, key) == &ns[i]);
  }

  Hashmap_free(h);

  * END_TODO
  */

  /*
     stretch goals:
     - expand the underlying array if we start to get a lot of collisions
     - support non-string keys
     - try different hash functions
     - switch from chaining to open addressing
     - use a sophisticated rehashing scheme to avoid clustered collisions
     - implement some features from Python dicts, such as reducing space use,
     maintaing key ordering etc. see https://www.youtube.com/watch?v=npw4s1QTmPg
     for ideas
     */
  printf("ok\n");
}
