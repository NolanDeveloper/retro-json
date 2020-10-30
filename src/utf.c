#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json.h"
#include "json_internal.h"

static int lookup_table[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1,
};

extern int c16len(char16_t c16) {
    return 0x1B == (c16 >> 11) ? 2 : 1;
}

extern enum c16Type c16type(char16_t c16) {
    if (c16 & 0xD800) {
        if (c16 & 0x400) {
            return UTF16_SURROGATE_LOW;
        } else {
            return UTF16_SURROGATE_HIGH;
        }
    }
    return UTF16_NOT_SURROGATE;
}

extern int c8len(char c) {
    return lookup_table[(unsigned char) c];
}

extern bool c32toc8(char32_t c32, int *n, char *c8) {
    if (c32 <= 0x007F) {
        *c8 = c32;
        *n = 1;
        return true;
    } else if (c32 <= 0x07FF) {
        *c8++ = (0xC0 | (c32 >> 6));
        *c8   = (0x80 | (c32 & 0x3F));
        *n = 2;
        return true;
    } else if (c32 <= 0xFFFF) {
        *c8++ = (0xE0 | (c32 >> 12));
        *c8++ = (0x80 | ((c32 >> 6) & 0x3F));
        *c8   = (0x80 | (c32 & 0x3F));
        *n = 3;
        return true;
    } else if (c32 <= 0x10FFFFull) {
        *c8++ = (0xF0 | (c32 >> 18));
        *c8++ = (0x80 | ((c32 >> 12) & 0x3F));
        *c8++ = (0x80 | ((c32 >> 6) & 0x3F));
        *c8   = (0x80 | (c32 & 0x3F));
        *n = 4;
        return true;
    }
    *n = 0;
    return false;
}

extern char32_t c8toc32(const char *c8) {
    assert(c8);
    unsigned char c = *c8++;
    if (c <= 0x7F) {
        return c;
    } 
    unsigned char c2 = *c8++;
    if (0xC2 <= c && c <= 0xDF) {
        return ((c & 0x1F) << 6) + (c2 & 0x3F);
    }
    unsigned char c3 = *c8++;
    if (0xE0 <= c && c <= 0xEF) {
        return ((c & 0x0F) << 12) + ((c2 & 0x3F) << 6) + (c3 & 0x3F);
    }
    unsigned char c4 = *c8;
    return ((c & 0x07) << 18) + ((c2 & 0x3F) << 12) + ((c3 & 0x3F) << 6) + (c4 & 0x3F);
}

extern char32_t c16pairtoc32(char16_t high, char16_t low) {
    return 0x10000ull + ((high & 0x3FFull) << 10) + (low & 0x3ffull);
}

extern void c32toc16be(char32_t c32, char16_t out[2]) {
    if (c32 < 0x10000ull) {
        out[0] = c32;
        out[1] = u'\0';
    } else {
        c32 -= 0x10000ull;
        out[0] = 0xD800 | (c32 >> 10);
        out[1] = 0xDC00 | (c32 & 0x3FF);
    }
}
