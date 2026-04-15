#include "hmap.h"
#include <stdint.h>
#include <stdlib.h>

#define INITIAL_CAP 16

hmap_t*
hmap_new() {
    int64_t** v_arr = malloc(INITIAL_CAP*sizeof(int64_t));
    char** k_arr = malloc(INITIAL_CAP*sizeof(char));
    hmap_t* hmap =malloc(sizeof(hmap_t));

    hmap->k_count=0;
    hmap->cap=INITIAL_CAP;
    hmap->v_arr=v_arr;
    hmap->k_arr=k_arr;

    return hmap;
}
