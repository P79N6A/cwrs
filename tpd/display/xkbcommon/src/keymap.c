/**
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

/************************************************************
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ********************************************************/

#include "keymap.h"
#include "text.h"

struct xkb_keymap *
xkb_keymap_new(struct xkb_context *ctx,
               enum xkb_keymap_format format,
               enum xkb_keymap_compile_flags flags)
{
    struct xkb_keymap *keymap;

    keymap = calloc(1, sizeof(*keymap));
    if (!keymap)
        return NULL;

    keymap->refcnt = 1;
    keymap->ctx = xkb_context_ref(ctx);

    keymap->format = format;
    keymap->flags = flags;

    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_keymap_ref(struct xkb_keymap *keymap)
{
    keymap->refcnt++;
    return keymap;
}

XKB_EXPORT void
xkb_keymap_unref(struct xkb_keymap *keymap)
{
    unsigned int i, j;
    struct xkb_key *key;

    if (!keymap || --keymap->refcnt > 0)
        return;

    if (keymap->keys) {
        xkb_foreach_key(key, keymap) {
            for (i = 0; i < key->num_groups; i++) {
                for (j = 0; j < XkbKeyGroupWidth(key, i); j++)
                    if (key->groups[i].levels[j].num_syms > 1)
                        free(key->groups[i].levels[j].u.syms);
                free(key->groups[i].levels);
            }
            free(key->groups);
        }
        free(keymap->keys);
    }
    for (i = 0; i < keymap->num_types; i++) {
        free(keymap->types[i].map);
        free(keymap->types[i].level_names);
    }
    free(keymap->types);
    darray_free(keymap->sym_interprets);
    darray_free(keymap->key_aliases);
    free(keymap->group_names);
    darray_free(keymap->mods);
    darray_free(keymap->indicators);
    free(keymap->keycodes_section_name);
    free(keymap->symbols_section_name);
    free(keymap->types_section_name);
    free(keymap->compat_section_name);
    xkb_context_unref(keymap->ctx);
    free(keymap);
}

/**
 * Returns the total number of modifiers active in the keymap.
 */
XKB_EXPORT xkb_mod_index_t
xkb_keymap_num_mods(struct xkb_keymap *keymap)
{
    return darray_size(keymap->mods);
}

/**
 * Return the name for a given modifier.
 */
XKB_EXPORT const char *
xkb_keymap_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx)
{
    if (idx >= darray_size(keymap->mods))
        return NULL;

    return xkb_atom_text(keymap->ctx, darray_item(keymap->mods, idx).name);
}

/**
 * Returns the index for a named modifier.
 */
XKB_EXPORT xkb_mod_index_t
xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_mod_index_t i;
    xkb_atom_t atom;
    const struct xkb_mod *mod;

    atom = xkb_atom_lookup(keymap->ctx, name);
    if (atom == XKB_ATOM_NONE)
        return XKB_MOD_INVALID;

    darray_enumerate(i, mod, keymap->mods)
        if (mod->name == atom)
            return i;

    return XKB_MOD_INVALID;
}

/**
 * Return the total number of active groups in the keymap.
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_num_layouts(struct xkb_keymap *keymap)
{
    return keymap->num_groups;
}

/**
 * Returns the name for a given group.
 */
XKB_EXPORT const char *
xkb_keymap_layout_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx)
{
    if (idx >= keymap->num_group_names)
        return NULL;

    return xkb_atom_text(keymap->ctx, keymap->group_names[idx]);
}

/**
 * Returns the index for a named layout.
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_layout_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_atom_t atom = xkb_atom_lookup(keymap->ctx, name);
    xkb_layout_index_t i;

    if (atom == XKB_ATOM_NONE)
        return XKB_LAYOUT_INVALID;

    for (i = 0; i < keymap->num_group_names; i++)
        if (keymap->group_names[i] == atom)
            return i;

    return XKB_LAYOUT_INVALID;
}

/**
 * Returns the number of layouts active for a particular key.
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_num_layouts_for_key(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return 0;

    return key->num_groups;
}

/**
 * Returns the number of levels active for a particular key and layout.
 */
XKB_EXPORT xkb_level_index_t
xkb_keymap_num_levels_for_key(struct xkb_keymap *keymap, xkb_keycode_t kc,
                              xkb_layout_index_t layout)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return 0;

    return XkbKeyGroupWidth(key, layout);
}

/**
 * Return the total number of LEDs in the keymap.
 */
XKB_EXPORT xkb_led_index_t
xkb_keymap_num_leds(struct xkb_keymap *keymap)
{
    return darray_size(keymap->indicators);
}

/**
 * Returns the name for a given LED.
 */
XKB_EXPORT const char *
xkb_keymap_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx)
{
    if (idx >= darray_size(keymap->indicators))
        return NULL;

    return xkb_atom_text(keymap->ctx,
                         darray_item(keymap->indicators, idx).name);
}

/**
 * Returns the index for a named LED.
 */
XKB_EXPORT xkb_led_index_t
xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_atom_t atom = xkb_atom_lookup(keymap->ctx, name);
    xkb_led_index_t i;
    const struct xkb_indicator_map *led;

    if (atom == XKB_ATOM_NONE)
        return XKB_LED_INVALID;

    darray_enumerate(i, led, keymap->indicators)
        if (led->name == atom)
            return i;

    return XKB_LED_INVALID;
}

/**
 * As below, but takes an explicit layout/level rather than state.
 */
XKB_EXPORT int
xkb_keymap_key_get_syms_by_level(struct xkb_keymap *keymap,
                                 xkb_keycode_t kc,
                                 xkb_layout_index_t layout,
                                 xkb_level_index_t level,
                                 const xkb_keysym_t **syms_out)
{
    const struct xkb_key *key = XkbKey(keymap, kc);
    int num_syms;

    if (!key)
        goto err;
    if (layout >= key->num_groups)
        goto err;
    if (level >= XkbKeyGroupWidth(key, layout))
        goto err;

    num_syms = key->groups[layout].levels[level].num_syms;
    if (num_syms == 0)
        goto err;

    if (num_syms == 1)
        *syms_out = &key->groups[layout].levels[level].u.sym;
    else
        *syms_out = key->groups[layout].levels[level].u.syms;

    return num_syms;

err:
    *syms_out = NULL;
    return 0;
}

/**
 * Simple boolean specifying whether or not the key should repeat.
 */
XKB_EXPORT int
xkb_keymap_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return 0;

    return key->repeats;
}
