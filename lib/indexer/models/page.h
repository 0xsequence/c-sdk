#ifndef SEQUENCE_PAGE_H
#define SEQUENCE_PAGE_H

#include <stdbool.h>

typedef struct {
    int page;
    int pageSize;
    bool more;
} SequencePage;

#endif
