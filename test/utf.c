#include "unit_tests.h"

#include "json_internal.h"

static bool test_c16len(void) {
    ASSERT(1 == c16len(0x0000));
    ASSERT(1 == c16len(0xD7FF));
    ASSERT(2 == c16len(0xD800));
    ASSERT(2 == c16len(0xDBFF));
    ASSERT(2 == c16len(0xDC00));
    ASSERT(2 == c16len(0xDFFF));
    ASSERT(1 == c16len(0xE000));
    ASSERT(1 == c16len(0xFFFF));
    return true;
}

static bool test_c16type(void) {
    ASSERT(UTF16_NOT_SURROGATE == c16type(0x0000));
    ASSERT(UTF16_NOT_SURROGATE == c16type(0xD7FF));
    ASSERT(UTF16_SURROGATE_HIGH == c16type(0xD800));
    ASSERT(UTF16_SURROGATE_HIGH == c16type(0xDBFF));
    ASSERT(UTF16_SURROGATE_LOW == c16type(0xDC00));
    ASSERT(UTF16_SURROGATE_LOW == c16type(0xDFFF));
    ASSERT(UTF16_NOT_SURROGATE == c16type(0xE000));
    ASSERT(UTF16_NOT_SURROGATE == c16type(0xFFFF));
    return true;
}

static bool test_c16pairtoc32(void) {
    ASSERT(0x0001f923 == c16pairtoc32(0xD83E, 0xDD23));
    return true;
}

static bool test_c8len(void) {
    ASSERT(1 == c8len('\x00'));
    ASSERT(1 == c8len('\x7F'));
    ASSERT(0 == c8len('\x80'));
    ASSERT(0 == c8len('\xBF'));
    ASSERT(2 == c8len('\xC0'));
    ASSERT(2 == c8len('\xDF'));
    ASSERT(3 == c8len('\xE0'));
    ASSERT(3 == c8len('\xEF'));
    ASSERT(4 == c8len('\xF7'));
    ASSERT(0 == c8len('\xFF'));
    return true;
}

static bool test_c32toc8(void) {
    char c8[4];
    int n;
    ASSERT(c32toc8(0x0001F923, &n, c8));
    ASSERT(n == 4);
    ASSERT(c8[0] == '\xF0');
    ASSERT(c8[1] == '\x9F');
    ASSERT(c8[2] == '\xA4');
    ASSERT(c8[3] == '\xA3');
    ASSERT(c32toc8(0x0000FF11, &n, c8));
    ASSERT(n == 3);
    ASSERT(c8[0] == '\xEF');
    ASSERT(c8[1] == '\xBC');
    ASSERT(c8[2] == '\x91');
    ASSERT(c32toc8(0x000007A5, &n, c8));
    ASSERT(n == 2);
    ASSERT(c8[0] == '\xDE');
    ASSERT(c8[1] == '\xA5');
    for (char32_t c32 = 0; c32 < 128; ++c32) {
        ASSERT(c32toc8(c32, &n, c8));
        ASSERT(n == 1);
        ASSERT(c8[0] == (char) c32);
    }
    return true;
}

static bool test_c8toc32(void) {
    char c8[4] = { 0 };
    char32_t c32;
    for (int i = 0; i < 128; ++i) {
        c8[0] = i;
        ASSERT(c8toc32(c8, &c32));
        ASSERT((char32_t) i == c32);
    }
    c8[0] = '\xDE';
    c8[1] = '\xA5';
    ASSERT(c8toc32(c8, &c32));
    ASSERT(0x07A5 == c32);
    c8[0] = '\xEF';
    c8[1] = '\xBC';
    c8[2] = '\x91';
    ASSERT(c8toc32(c8, &c32));
    ASSERT(0xFF11 == c32);
    c8[0] = '\xF0';
    c8[1] = '\x9F';
    c8[2] = '\xA4';
    c8[3] = '\xA3';
    ASSERT(c8toc32(c8, &c32));
    ASSERT(0x1F923 == c32);
    return true;
}

static bool test_c32toc16be(void) {
    char16_t c16[2];
    c32toc16be(0x0000, c16);
    ASSERT(0x0000 == c16[0]);
    ASSERT(0x0000 == c16[1]);
    c32toc16be(0xFFFF, c16);
    ASSERT(0xFFFF == c16[0]);
    ASSERT(0x0000 == c16[1]);
    c32toc16be(0x10000, c16);
    ASSERT(0xD800 == c16[0]);
    ASSERT(0xDC00 == c16[1]);
    c32toc16be(0x10FFFF, c16);
    ASSERT(0xDBFF == c16[0]);
    ASSERT(0xDFFF == c16[1]);
    return true;
}

extern bool test_utf(void) {
    return test_c16len() 
        && test_c16type()
        && test_c16pairtoc32()
        && test_c8len()
        && test_c32toc8()
        && test_c8toc32()
        && test_c32toc16be();
}
