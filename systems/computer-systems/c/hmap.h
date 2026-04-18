#include <stdint.h>

typedef uint64_t hindex_t;

typedef enum hmap_err {
    NO_ERROR,
    KEY_NOT_ALLOWED,
    KEY_NOT_FOUND,
    HMAP_FULL
} hmap_err_t;

typedef struct hmap_t {
    hindex_t k_count;
    hindex_t cap;
    int64_t* v_arr;
    char** k_arr;
} hmap_t;

hmap_t*
    hmap_new();
static hindex_t
    __hash(char* k);
hmap_err_t
    hmap_get(hmap_t* h, char* k, int64_t* v);
hmap_err_t
    hmap_set(hmap_t* h, char* k, int64_t v);
void
    hmap_del(hmap_t* h, char* k);
