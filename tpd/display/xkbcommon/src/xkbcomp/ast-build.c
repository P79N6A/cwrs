/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
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
 ********************************************************/

/*
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
 *         Ran Benita <ran234@gmail.com>
 */

#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "parser-priv.h"
#include "include.h"

ATTR_MALLOC static void *
malloc_or_die(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

ParseCommon *
AppendStmt(ParseCommon *to, ParseCommon *append)
{
    ParseCommon *iter;

    if (!to)
        return append;

    for (iter = to; iter->next; iter = iter->next);

    iter->next = append;
    return to;
}

ExprDef *
ExprCreate(enum expr_op_type op, enum expr_value_type type)
{
    ExprDef *expr;

    expr = malloc_or_die(sizeof(*expr));

    expr->common.type = STMT_EXPR;
    expr->common.next = NULL;
    expr->op = op;
    expr->value_type = type;
    return expr;
}

ExprDef *
ExprCreateUnary(enum expr_op_type op, enum expr_value_type type,
                ExprDef *child)
{
    ExprDef *expr;
    expr = malloc_or_die(sizeof(*expr));

    expr->common.type = STMT_EXPR;
    expr->common.next = NULL;
    expr->op = op;
    expr->value_type = type;
    expr->value.child = child;
    return expr;
}

ExprDef *
ExprCreateBinary(enum expr_op_type op, ExprDef *left, ExprDef *right)
{
    ExprDef *expr;

    expr = malloc_or_die(sizeof(*expr));

    expr->common.type = STMT_EXPR;
    expr->common.next = NULL;
    expr->op = op;
    if (op == EXPR_ASSIGN || left->value_type == EXPR_TYPE_UNKNOWN)
        expr->value_type = right->value_type;
    else if (left->value_type == right->value_type ||
             right->value_type == EXPR_TYPE_UNKNOWN)
        expr->value_type = left->value_type;
    else
        expr->value_type = EXPR_TYPE_UNKNOWN;
    expr->value.binary.left = left;
    expr->value.binary.right = right;
    return expr;
}

KeycodeDef *
KeycodeCreate(xkb_atom_t name, int64_t value)
{
    KeycodeDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_KEYCODE;
    def->common.next = NULL;
    def->name = name;
    def->value = value;
    return def;
}

KeyAliasDef *
KeyAliasCreate(xkb_atom_t alias, xkb_atom_t real)
{
    KeyAliasDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_ALIAS;
    def->common.next = NULL;
    def->alias = alias;
    def->real = real;
    return def;
}

VModDef *
VModCreate(xkb_atom_t name, ExprDef * value)
{
    VModDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_VMOD;
    def->common.next = NULL;
    def->name = name;
    def->value = value;
    return def;
}

VarDef *
VarCreate(ExprDef * name, ExprDef * value)
{
    VarDef *def;
    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_VAR;
    def->common.next = NULL;
    def->name = name;
    def->value = value;
    return def;
}

VarDef *
BoolVarCreate(xkb_atom_t nameToken, unsigned set)
{
    ExprDef *name, *value;

    name = ExprCreate(EXPR_IDENT, EXPR_TYPE_UNKNOWN);
    name->value.str = nameToken;
    value = ExprCreate(EXPR_VALUE, EXPR_TYPE_BOOLEAN);
    value->value.uval = set;
    return VarCreate(name, value);
}

InterpDef *
InterpCreate(char *sym, ExprDef * match)
{
    InterpDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_INTERP;
    def->common.next = NULL;
    def->sym = sym;
    def->match = match;
    return def;
}

KeyTypeDef *
KeyTypeCreate(xkb_atom_t name, VarDef * body)
{
    KeyTypeDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_TYPE;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;
    return def;
}

SymbolsDef *
SymbolsCreate(xkb_atom_t keyName, ExprDef *symbols)
{
    SymbolsDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_SYMBOLS;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->keyName = keyName;
    def->symbols = symbols;
    return def;
}

GroupCompatDef *
GroupCompatCreate(int group, ExprDef * val)
{
    GroupCompatDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_GROUP_COMPAT;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->group = group;
    def->def = val;
    return def;
}

ModMapDef *
ModMapCreate(uint32_t modifier, ExprDef * keys)
{
    ModMapDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_MODMAP;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->modifier = modifier;
    def->keys = keys;
    return def;
}

IndicatorMapDef *
IndicatorMapCreate(xkb_atom_t name, VarDef * body)
{
    IndicatorMapDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_INDICATOR_MAP;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;
    return def;
}

IndicatorNameDef *
IndicatorNameCreate(int ndx, ExprDef * name, bool virtual)
{
    IndicatorNameDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.type = STMT_INDICATOR_NAME;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->ndx = ndx;
    def->name = name;
    def->virtual = virtual;
    return def;
}

ExprDef *
ActionCreate(xkb_atom_t name, ExprDef * args)
{
    ExprDef *act;

    act = malloc_or_die(sizeof(*act));

    act->common.type = STMT_EXPR;
    act->common.next = NULL;
    act->op = EXPR_ACTION_DECL;
    act->value.action.name = name;
    act->value.action.args = args;
    return act;
}

ExprDef *
CreateKeysymList(char *sym)
{
    ExprDef *def;

    def = ExprCreate(EXPR_KEYSYM_LIST, EXPR_TYPE_SYMBOLS);

    darray_init(def->value.list.syms);
    darray_init(def->value.list.symsMapIndex);
    darray_init(def->value.list.symsNumEntries);

    darray_append(def->value.list.syms, sym);
    darray_append(def->value.list.symsMapIndex, 0);
    darray_append(def->value.list.symsNumEntries, 1);

    return def;
}

ExprDef *
CreateMultiKeysymList(ExprDef *list)
{
    size_t nLevels = darray_size(list->value.list.symsMapIndex);

    darray_resize(list->value.list.symsMapIndex, 1);
    darray_resize(list->value.list.symsNumEntries, 1);
    darray_item(list->value.list.symsMapIndex, 0) = 0;
    darray_item(list->value.list.symsNumEntries, 0) = nLevels;

    return list;
}

ExprDef *
AppendKeysymList(ExprDef * list, char *sym)
{
    size_t nSyms = darray_size(list->value.list.syms);

    darray_append(list->value.list.symsMapIndex, nSyms);
    darray_append(list->value.list.symsNumEntries, 1);
    darray_append(list->value.list.syms, sym);

    return list;
}

ExprDef *
AppendMultiKeysymList(ExprDef * list, ExprDef * append)
{
    size_t nSyms = darray_size(list->value.list.syms);
    size_t numEntries = darray_size(append->value.list.syms);

    darray_append(list->value.list.symsMapIndex, nSyms);
    darray_append(list->value.list.symsNumEntries, numEntries);
    darray_append_items(list->value.list.syms,
                        darray_mem(append->value.list.syms, 0),
                        numEntries);

    darray_resize(append->value.list.syms, 0);
    FreeStmt(&append->common);

    return list;
}

static void
FreeInclude(IncludeStmt *incl);

IncludeStmt *
IncludeCreate(struct xkb_context *ctx, char *str, enum merge_mode merge)
{
    IncludeStmt *incl, *first;
    char *file, *map, *stmt, *tmp, *extra_data;
    char nextop;

    incl = first = NULL;
    file = map = NULL;
    tmp = str;
    stmt = strdup_safe(str);
    while (tmp && *tmp)
    {
        if (!ParseIncludeMap(&tmp, &file, &map, &nextop, &extra_data))
            goto err;

        /*
         * Given an RMLVO (here layout) like 'us,,fr', the rules parser
         * will give out something like 'pc+us+:2+fr:3+inet(evdev)'.
         * We should just skip the ':2' in this case and leave it to the
         * appropriate section to deal with the empty group.
         */
        if (isempty(file)) {
            free(file);
            free(map);
            free(extra_data);
            continue;
        }

        if (first == NULL) {
            first = incl = malloc(sizeof(*first));
        } else {
            incl->next_incl = malloc(sizeof(*first));
            incl = incl->next_incl;
        }

        if (!incl) {
            log_wsgo(ctx,
                     "Allocation failure in IncludeCreate; "
                     "Using only part of the include\n");
            break;
        }

        incl->common.type = STMT_INCLUDE;
        incl->common.next = NULL;
        incl->merge = merge;
        incl->stmt = NULL;
        incl->file = file;
        incl->map = map;
        incl->modifier = extra_data;
        incl->next_incl = NULL;

        if (nextop == '|')
            merge = MERGE_AUGMENT;
        else
            merge = MERGE_OVERRIDE;
    }

    if (first)
        first->stmt = stmt;
    else
        free(stmt);

    return first;

err:
    log_err(ctx, "Illegal include statement \"%s\"; Ignored\n", stmt);
    FreeInclude(first);
    free(stmt);
    return NULL;
}

/*
 * All latin-1 alphanumerics, plus parens, slash, minus, underscore and
 * wildcards.
 */
static const unsigned char componentSpecLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

static void
EnsureSafeMapName(char *name)
{
    if (!name)
        return;

    while (*name != '\0') {
        if ((componentSpecLegal[(*name) / 8] & (1 << ((*name) % 8))) == 0)
            *name = '_';
        name++;
    }
}

XkbFile *
XkbFileCreate(struct xkb_context *ctx, enum xkb_file_type type, char *name,
              ParseCommon *defs, enum xkb_map_flags flags)
{
    XkbFile *file;

    file = calloc(1, sizeof(*file));
    if (!file)
        return NULL;

    EnsureSafeMapName(name);
    file->file_type = type;
    file->topName = strdup_safe(name);
    file->name = name;
    file->defs = defs;
    file->id = xkb_context_take_file_id(ctx);
    file->flags = flags;
    return file;
}

XkbFile *
XkbFileFromComponents(struct xkb_context *ctx,
                      struct xkb_component_names *kkctgs)
{
    IncludeStmt *inc;
    XkbFile *keycodes, *types, *compat, *symbols;

    inc = IncludeCreate(ctx, kkctgs->keycodes, MERGE_DEFAULT);
    keycodes = XkbFileCreate(ctx, FILE_TYPE_KEYCODES, NULL,
                             (ParseCommon *) inc, 0);

    inc = IncludeCreate(ctx, kkctgs->types, MERGE_DEFAULT);
    types = XkbFileCreate(ctx, FILE_TYPE_TYPES, NULL,
                          (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &types->common);

    inc = IncludeCreate(ctx, kkctgs->compat, MERGE_DEFAULT);
    compat = XkbFileCreate(ctx, FILE_TYPE_COMPAT, NULL,
                           (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &compat->common);

    inc = IncludeCreate(ctx, kkctgs->symbols, MERGE_DEFAULT);
    symbols = XkbFileCreate(ctx, FILE_TYPE_SYMBOLS, NULL,
                            (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &symbols->common);

    return XkbFileCreate(ctx, FILE_TYPE_KEYMAP, NULL,
                         &keycodes->common, 0);
}

static void
FreeExpr(ExprDef *expr)
{
    char **sym;

    if (!expr)
        return;

    switch (expr->op) {
    case EXPR_ACTION_LIST:
    case EXPR_NEGATE:
    case EXPR_UNARY_PLUS:
    case EXPR_NOT:
    case EXPR_INVERT:
        FreeStmt(&expr->value.child->common);
        break;

    case EXPR_DIVIDE:
    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_ASSIGN:
        FreeStmt(&expr->value.binary.left->common);
        FreeStmt(&expr->value.binary.right->common);
        break;

    case EXPR_ACTION_DECL:
        FreeStmt(&expr->value.action.args->common);
        break;

    case EXPR_ARRAY_REF:
        FreeStmt(&expr->value.array.entry->common);
        break;

    case EXPR_KEYSYM_LIST:
        darray_foreach(sym, expr->value.list.syms)
            free(*sym);
        darray_free(expr->value.list.syms);
        darray_free(expr->value.list.symsMapIndex);
        darray_free(expr->value.list.symsNumEntries);
        break;

    default:
        break;
    }
}

static void
FreeInclude(IncludeStmt *incl)
{
    IncludeStmt *next;

    while (incl)
    {
        next = incl->next_incl;

        free(incl->file);
        free(incl->map);
        free(incl->modifier);
        free(incl->stmt);

        free(incl);
        incl = next;
    }
}

void
FreeStmt(ParseCommon *stmt)
{
    ParseCommon *next;
    YYSTYPE u;

    while (stmt)
    {
        next = stmt->next;
        u.any = stmt;

        switch (stmt->type) {
        case STMT_INCLUDE:
            FreeInclude((IncludeStmt *) stmt);
            /* stmt is already free'd here. */
            stmt = NULL;
            break;
        case STMT_EXPR:
            FreeExpr(u.expr);
            break;
        case STMT_VAR:
            FreeStmt(&u.var->name->common);
            FreeStmt(&u.var->value->common);
            break;
        case STMT_TYPE:
            FreeStmt(&u.keyType->body->common);
            break;
        case STMT_INTERP:
            free(u.interp->sym);
            FreeStmt(&u.interp->match->common);
            FreeStmt(&u.interp->def->common);
            break;
        case STMT_VMOD:
            FreeStmt(&u.vmod->value->common);
            break;
        case STMT_SYMBOLS:
            FreeStmt(&u.syms->symbols->common);
            break;
        case STMT_MODMAP:
            FreeStmt(&u.modMask->keys->common);
            break;
        case STMT_GROUP_COMPAT:
            FreeStmt(&u.groupCompat->def->common);
            break;
        case STMT_INDICATOR_MAP:
            FreeStmt(&u.ledMap->body->common);
            break;
        case STMT_INDICATOR_NAME:
            FreeStmt(&u.ledName->name->common);
            break;
        default:
            break;
        }

        free(stmt);
        stmt = next;
    }
}

void
FreeXkbFile(XkbFile *file)
{
    XkbFile *next;

    while (file)
    {
        next = (XkbFile *) file->common.next;

        switch (file->file_type) {
        case FILE_TYPE_KEYMAP:
            FreeXkbFile((XkbFile *) file->defs);
            break;

        case FILE_TYPE_TYPES:
        case FILE_TYPE_COMPAT:
        case FILE_TYPE_SYMBOLS:
        case FILE_TYPE_KEYCODES:
        case FILE_TYPE_GEOMETRY:
            FreeStmt(file->defs);
            break;

        default:
            break;
        }

        free(file->name);
        free(file->topName);
        free(file);
        file = next;
    }
}

static const char *xkb_file_type_strings[_FILE_TYPE_NUM_ENTRIES] = {
    [FILE_TYPE_KEYCODES] = "xkb_keycodes",
    [FILE_TYPE_TYPES] = "xkb_types",
    [FILE_TYPE_COMPAT] = "xkb_compatibility",
    [FILE_TYPE_SYMBOLS] = "xkb_symbols",
    [FILE_TYPE_KEYMAP] = "xkb_keymap",
    [FILE_TYPE_RULES] = "rules",
};

const char *
xkb_file_type_to_string(enum xkb_file_type type)
{
    if (type > _FILE_TYPE_NUM_ENTRIES)
        return "unknown";
    return xkb_file_type_strings[type];
}

static const char *stmt_type_strings[_STMT_NUM_VALUES] = {
    [STMT_UNKNOWN] = "unknown statement",
    [STMT_INCLUDE] = "include statement",
    [STMT_KEYCODE] = "key name definition",
    [STMT_ALIAS] = "key alias definition",
    [STMT_EXPR] = "expression",
    [STMT_VAR] = "variable definition",
    [STMT_TYPE] = "key type definition",
    [STMT_INTERP] = "symbol interpretation definition",
    [STMT_VMOD] = "virtual modifiers definition",
    [STMT_SYMBOLS] = "key symbols definition",
    [STMT_MODMAP] = "modifier map declaration",
    [STMT_GROUP_COMPAT] = "group declaration",
    [STMT_INDICATOR_MAP] = "indicator map declaration",
    [STMT_INDICATOR_NAME] = "indicator name declaration",
};

const char *
stmt_type_to_string(enum stmt_type type)
{
    if (type >= _STMT_NUM_VALUES)
        return NULL;
    return stmt_type_strings[type];
}

static const char *expr_op_type_strings[_EXPR_NUM_VALUES] = {
    [EXPR_VALUE] = "literal",
    [EXPR_IDENT] = "identifier",
    [EXPR_ACTION_DECL] = "action declaration",
    [EXPR_FIELD_REF] = "field reference",
    [EXPR_ARRAY_REF] = "array reference",
    [EXPR_KEYSYM_LIST] = "list of keysyms",
    [EXPR_ACTION_LIST] = "list of actions",
    [EXPR_ADD] = "addition",
    [EXPR_SUBTRACT] = "subtraction",
    [EXPR_MULTIPLY] = "multiplication",
    [EXPR_DIVIDE] = "division",
    [EXPR_ASSIGN] = "assignment",
    [EXPR_NOT] = "logical negation",
    [EXPR_NEGATE] = "arithmetic negation",
    [EXPR_INVERT] = "bitwise inversion",
    [EXPR_UNARY_PLUS] = "unary plus",
};

const char *
expr_op_type_to_string(enum expr_op_type type)
{
    if (type >= _EXPR_NUM_VALUES)
        return NULL;
    return expr_op_type_strings[type];
}

static const char *expr_value_type_strings[_EXPR_TYPE_NUM_VALUES] = {
    [EXPR_TYPE_UNKNOWN] = "unknown",
    [EXPR_TYPE_BOOLEAN] = "boolean",
    [EXPR_TYPE_INT] = "int",
    [EXPR_TYPE_STRING] = "string",
    [EXPR_TYPE_ACTION] = "action",
    [EXPR_TYPE_KEYNAME] = "keyname",
    [EXPR_TYPE_SYMBOLS] = "symbols",
};

const char *
expr_value_type_to_string(enum expr_value_type type)
{
    if (type >= _EXPR_TYPE_NUM_VALUES)
        return NULL;
    return expr_value_type_strings[type];
}
