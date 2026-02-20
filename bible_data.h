#ifndef BIBLE_DATA_H
#define BIBLE_DATA_H

#include <genesis.h>

typedef struct {
    const char* name;
    u16 numChapters;
    const char * const * const * chapters;
} BibleBook;

extern const BibleBook bible_books[];
extern const u16 NUM_BIBLE_BOOKS;

#endif
