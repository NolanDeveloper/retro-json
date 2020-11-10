#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>

#include "json_internal.h"

extern int c16len(char16_t c16) {
    return 0xD800 == (0xF800 & c16) ? 2 : 1;
}

extern enum c16Type c16type(char16_t c16) {
    if (1 == c16len(c16)) {
        return UTF16_NOT_SURROGATE;
    }
    return c16 & 0x0400 ? UTF16_SURROGATE_LOW : UTF16_SURROGATE_HIGH;
}

extern int c8len(char c) {
    static int lookup_table[256] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    return lookup_table[(unsigned char) c];
}

extern bool c32toc8(char32_t c32, int *n, char *c8) {
    if (c32 <= 0x007F) {
        c8[0] = c32;
        *n = 1;
        return true;
    } else if (c32 <= 0x07FF) {
        c8[0] = 0xC0 | (c32 >> 6);
        c8[1] = 0x80 | (c32 & 0x3F);
        *n = 2;
        return true;
    } else if (c32 <= 0xFFFF) {
        c8[0] = 0xE0 | (c32 >> 12);
        c8[1] = 0x80 | ((c32 >> 6) & 0x3F);
        c8[2] = 0x80 | (c32 & 0x3F);
        *n = 3;
        return true;
    } else if (c32 <= 0x10FFFFull) {
        c8[0] = 0xF0 | (c32 >> 18);
        c8[1] = 0x80 | ((c32 >> 12) & 0x3F);
        c8[2] = 0x80 | ((c32 >> 6) & 0x3F);
        c8[3] = 0x80 | (c32 & 0x3F);
        *n = 4;
        return true;
    }
    *n = 0;
    return false;
}

extern int c32c8len(char32_t c32) {
    if (c32 < 0x0080) {
        return 1;
    }
    if (c32 < 0x0800) {
        return 2;
    }
    if (c32 < 0x10000ull) {
        return 3;
    }
    return 4;
}

extern bool c8toc32(const char *c8, char32_t *c32) {
    assert(c8);
    assert(c32);
    unsigned char c = *c8;
    int len = c8len(c);
    if (!len) {
        return false;
    }
    char32_t result;
    switch (len) {
    case 1:
        *c32 = c;
        return true;
    case 2:
        result = c & 0x1F;
        break;
    case 3:
        result = c & 0x0F;
        break;
    case 4:
        result = c & 0x07;
        break;
    default:
        assert(false);
    }
    for (int i = 1; i < len; ++i) {
        c = *++c8;
        if ((0xC0 & c) != 0x80) {
            errorf("invalid trailing UTF-8 byte");
            return false;
        }
        result = (result << 6) + (c & 0x3F);
    }
    if (!c32islegal(result)) {
        errorf("illegal code point");
        return false;
    }
    if (c32c8len(result) < len) {
        errorf("overlong UTF-8 sequence");
        return false;
    }
    *c32 = result;
    return true;
}

extern char32_t c16pairtoc32(char16_t high, char16_t low) {
    assert(UTF16_SURROGATE_HIGH == c16type(high));
    assert(UTF16_SURROGATE_LOW == c16type(low));
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

extern bool c32islegal(char32_t c32) {
    return c32 < 0xD800 || (0xDFFF < c32 && c32 < 0x110000ull);
}
