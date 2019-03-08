/*
 * Copyright © 2009 Dan Nicholson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <time.h>

#include "test.h"

#define BENCHMARK_ITERATIONS 1000

static int
test_rmlvo(struct xkb_context *context, const char *rules,
           const char *model, const char *layout,
           const char *variant, const char *options)
{
    struct xkb_keymap *keymap;

    keymap = test_compile_rules(context, rules, model, layout, variant,
                                options);
    if (keymap) {
        fprintf(stderr, "Compiled '%s' '%s' '%s' '%s' '%s'\n",
                strnull(rules), strnull(model), strnull(layout),
                strnull(variant), strnull(options));
        xkb_keymap_unref(keymap);
    }

    return keymap != NULL;
}

static int
test_rmlvo_silent(struct xkb_context *context, const char *rules,
                  const char *model, const char *layout,
                  const char *variant, const char *options)
{
    struct xkb_keymap *keymap;

    keymap = test_compile_rules(context, rules, model, layout, variant,
                                options);
    if (keymap)
        xkb_keymap_unref(keymap);

    return keymap != NULL;
}

static void
benchmark(struct xkb_context *context)
{
    struct timespec start, stop, elapsed;
    enum xkb_log_level old_level = xkb_context_get_log_level(context);
    int old_verb = xkb_context_get_log_verbosity(context);
    int i;

    xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(context, 0);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < BENCHMARK_ITERATIONS; i++)
        assert(test_rmlvo_silent(context, "evdev", "evdev", "us", "", ""));
    clock_gettime(CLOCK_MONOTONIC, &stop);

    xkb_context_set_log_level(context, old_level);
    xkb_context_set_log_verbosity(context, old_verb);

    elapsed.tv_sec = stop.tv_sec - start.tv_sec;
    elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;
    if (elapsed.tv_nsec < 0) {
        elapsed.tv_nsec += 1000000000;
        elapsed.tv_sec--;
    }

    fprintf(stderr, "compiled %d keymaps in %ld.%09lds\n",
            BENCHMARK_ITERATIONS, elapsed.tv_sec, elapsed.tv_nsec);
}

int main(int argc, char *argv[])
{
    struct xkb_context *ctx = test_get_context();

    assert(ctx);

    if (argc > 1 && streq(argv[1], "bench")) {
        benchmark(ctx);
        return 0;
    }

    assert(test_rmlvo(ctx, "evdev", "pc105", "us,il,ru,ca", ",,,multix", "grp:alts_toggle,ctrl:nocaps,compose:rwin"));
    assert(test_rmlvo(ctx, "base",  "pc105", "us,in", "", ""));
    assert(test_rmlvo(ctx, "evdev", "pc105", "us", "intl", ""));
    assert(test_rmlvo(ctx, "evdev", "evdev", "us", "intl", "grp:alts_toggle"));

    /* 20 is not a legal group; make sure this is handled gracefully. */
    assert(test_rmlvo(ctx, "evdev", "", "us:20", "", ""));

    /* Don't choke on missing values in RMLVO. Should just skip them. */
    assert(test_rmlvo(ctx, "evdev", "", "us,,ca", "", ""));

    assert(test_rmlvo(ctx, "", "", "", "", ""));
    assert(test_rmlvo(ctx, NULL, NULL, NULL, NULL, NULL));

    assert(!test_rmlvo(ctx, "does-not-exist", "", "", "", ""));

    xkb_context_unref(ctx);
}
