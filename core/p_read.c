#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "pal_core.h"
#include "pal_core_private.h"

int p_read (p_mem_t *mem, size_t nb, int flags, void *dst){

    memcpy(dst, mem->memptr,nb);
    return(0);
}