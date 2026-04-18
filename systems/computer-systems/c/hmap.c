#include "hmap.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define set_next_probe(i, cap) i=(i+1)%cap

static const uint8_t INITIAL_CAP = 4;
static const float LOAD_FACTOR = 0.75;
static const intptr_t KEY_EMPTY = INTPTR_MIN;
static const intptr_t KEY_DELETED = INTPTR_MAX;


hmap_t*
hmap_new() {

    hmap_t* hmap = malloc(sizeof(hmap_t));
    hmap->k_count=0;
    hmap->cap=INITIAL_CAP;

    int64_t* v_arr = malloc(INITIAL_CAP*sizeof(int64_t));
    hmap->v_arr=v_arr;

    char** k_arr = malloc(INITIAL_CAP*sizeof(char*));
    for (hindex_t i=0; i<INITIAL_CAP; i++) {
        k_arr[i] = (char*)KEY_EMPTY;
    }
    hmap->k_arr=k_arr;

    return hmap;
}

static hindex_t
__hash(char* k) {
    hindex_t hash = 5381;
    for (uint64_t i=0; i<strlen(k);i++) {
        hash = (hash<<5) + hash + k[i];
    }
    return hash;
}

hmap_err_t
hmap_get(hmap_t* h, char* k, int64_t* v) {
    hindex_t idx = __hash(k);
    idx %= h->cap;

    hindex_t i=idx;
    do {
        if (h->k_arr[i] == (char*)KEY_DELETED) {
            continue;
        } else if (h->k_arr[i] == (char*)KEY_EMPTY) {
            return KEY_NOT_FOUND;
        } else if (!strncmp(h->k_arr[i], k, strlen(h->k_arr[i]))) {
            *v = h->v_arr[idx];
            return NO_ERROR;
        }
        set_next_probe(i, h->cap);
    } while (i!=idx);

    return KEY_NOT_FOUND;
}

hmap_err_t
hmap_set(hmap_t* h, char* k, int64_t v) {
    // TODO: implement resizing
    if (h->k_count == h->cap) {return HMAP_FULL;}

    hindex_t idx= __hash(k);
    idx %= h->cap;

    hindex_t i=idx;
    do {
        if (
            h->k_arr[i] == (char*)KEY_EMPTY ||
            h->k_arr[i] == (char*)KEY_DELETED
        ) {
            // TODO: malloc can fail here; we need arenaas mate
            h->k_arr[i] = malloc(strlen(k) * sizeof(char));
            strncpy(h->k_arr[i], k, strlen(k));
            h->v_arr[i] = v;
            return NO_ERROR;
        } else if (!strncmp(h->k_arr[i], k, strlen(h->k_arr[i]))) {
            h->v_arr[i] = v;
            return NO_ERROR;
        }
        set_next_probe(i, h->cap);
    } while (i != idx);

    return HMAP_FULL;
}

void
hmap_del(hmap_t *h, char *k) {
    hindex_t idx = __hash(k);
    idx %= h->cap;

    hindex_t i=idx;
    do {
        if (h->k_arr[i] == (char*)KEY_EMPTY) {
            return;
        } else if (!strcmp(h->k_arr[i], k)) {
            h->k_arr[i] = (char*)KEY_DELETED;
            return;
        }
        set_next_probe(i, h->cap);
    } while (i!=idx);
}
