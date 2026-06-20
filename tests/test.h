/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * test.h — Minimal test framework.  No dependencies beyond stdio/stdlib.
 *
 * Each test file defines static test_*() functions and a main() that calls
 * them.  Assertions count passes/fails; main returns 0 iff all pass.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef TESTS_TEST_H
#define TESTS_TEST_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _tests_run = 0;
static int _tests_failed = 0;

#define ASSERT(cond, msg)                                                              \
    do {                                                                               \
        _tests_run++;                                                                  \
        if (!(cond)) {                                                                 \
            printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__);                  \
            _tests_failed++;                                                           \
        } else {                                                                       \
            printf("  ok: %s\n", msg);                                                 \
        }                                                                              \
    } while (0)

#define ASSERT_EQ_INT(a, b, msg)                                                       \
    do {                                                                               \
        _tests_run++;                                                                  \
        int _ae_a = (int)(a);                                                          \
        int _ae_b = (int)(b);                                                          \
        if (_ae_a != _ae_b) {                                                          \
            printf("  FAIL: %s: %d != %d  (%s:%d)\n", msg, _ae_a, _ae_b, __FILE__,    \
                   __LINE__);                                                          \
            _tests_failed++;                                                           \
        } else {                                                                       \
            printf("  ok: %s\n", msg);                                                 \
        }                                                                              \
    } while (0)

#define ASSERT_STREQ(a, b, msg)                                                        \
    do {                                                                               \
        _tests_run++;                                                                  \
        const char *_as_a = (a);                                                       \
        const char *_as_b = (b);                                                       \
        if (strcmp(_as_a, _as_b) != 0) {                                               \
            printf("  FAIL: %s: '%s' != '%s'  (%s:%d)\n", msg, _as_a, _as_b,          \
                   __FILE__, __LINE__);                                                \
            _tests_failed++;                                                           \
        } else {                                                                       \
            printf("  ok: %s\n", msg);                                                 \
        }                                                                              \
    } while (0)

#define TEST_SUMMARY()                                                                 \
    printf("\n%d/%d passed\n", (_tests_run) - (_tests_failed), _tests_run)

#define TEST_RETURN() ((_tests_failed) > 0 ? EXIT_FAILURE : EXIT_SUCCESS)

#endif /* TESTS_TEST_H */
