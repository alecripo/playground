#include <stdint.h>

typedef struct hmap_t {
    uint32_t k_count;
    uint32_t cap;
    int64_t** v_arr;
    char** k_arr;
} hmap_t;

hmap_t*
    hmap_new();
int64_t
    hmap_get(hmap_t* h, char* k);
void
    hmap_set(hmap_t* h, char* k, int64_t v);
void
    hmap_del(hmap_t* h, char* k);
