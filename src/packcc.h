/*
 * PackCC: a packrat parser generator for C.
 *
 * Copyright (c) 2014 Arihiro Yoshida. All rights reserved.
 * Copyright (c) 2017 Arvid Gerstmann. All rights reserved.
 * Copyright (c) 2020 Jean-Noël Namory. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef _MSC_VER
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#ifndef _MSC_VER
#if defined __GNUC__ && defined _WIN32 /* MinGW */
static size_t strnlen(const char *str, size_t maxlen)
{
  size_t i;
  for (i = 0; str[i] && i < maxlen; i++)
    ;
  return i;
}
#else
#include <unistd.h> /* for strnlen() */
#endif
#endif

#ifdef _MSC_VER
#undef snprintf
#undef unlink
#define snprintf _snprintf
#define unlink _unlink
#endif

#define VERSION "1.2.1"

#ifndef BUFFER_INIT_SIZE
#define BUFFER_INIT_SIZE 256
#endif
#ifndef ARRAY_INIT_SIZE
#define ARRAY_INIT_SIZE 2
#endif

typedef enum bool_tag {
    FALSE = 0,
    TRUE
} bool_t;

typedef struct char_array_tag {
    char *buf;
    int max;
    int len;
} char_array_t;

typedef enum node_type_tag {
    NODE_RULE = 0,
    NODE_REFERENCE,
    NODE_STRING,
    NODE_CHARCLASS,
    NODE_QUANTITY,
    NODE_PREDICATE,
    NODE_SEQUENCE,
    NODE_ALTERNATE,
    NODE_CAPTURE,
    NODE_EXPAND,
    NODE_ACTION,
    NODE_TEST,
    NODE_ERROR,
} node_type_t;

typedef struct node_tag node_t;

typedef struct node_array_tag {
    node_t **buf;
    int max;
    int len;
} node_array_t;

typedef struct node_const_array_tag {
    const node_t **buf;
    int max;
    int len;
} node_const_array_t;

typedef struct node_hash_table_tag {
    const node_t **buf;
    int max;
    int mod;
} node_hash_table_t;

typedef struct node_rule_tag {
    char *name;
    node_t *expr;
    int ref; /* mutable */
    node_const_array_t vars;
    node_const_array_t capts;
    node_const_array_t codes;
    int line;
    int col;
} node_rule_t;

typedef struct node_reference_tag {
    char *var; /* NULL if no variable name */
    int index;
    char *name;
    const node_t *rule;
    int line;
    int col;
} node_reference_t;

typedef struct node_string_tag {
    char *value;
} node_string_t;

typedef struct node_charclass_tag {
    char *value; /* NULL means any character */
} node_charclass_t;

typedef struct node_quantity_tag {
    int min;
    int max;
    node_t *expr;
} node_quantity_t;

typedef struct node_predicate_tag {
    bool_t neg;
    node_t *expr;
} node_predicate_t;

typedef struct node_sequence_tag {
    node_array_t nodes;
} node_sequence_t;

typedef struct node_alternate_tag {
    node_array_t nodes;
} node_alternate_t;

typedef struct node_capture_tag {
    node_t *expr;
    int index;
} node_capture_t;

typedef struct node_expand_tag {
    int index;
    int line;
    int col;
} node_expand_t;

typedef struct node_action_tag {
    char *value;
    int index;
    node_const_array_t vars;
    node_const_array_t capts;
} node_action_t;

typedef struct node_error_tag {
    node_t *expr;
    char *value;
    int index;
    node_const_array_t vars;
    node_const_array_t capts;
} node_error_t;

typedef struct node_test_tag {
    node_t *expr;
    char *value;
    int index;
    node_const_array_t vars;
    node_const_array_t capts;
} node_test_t;

typedef union node_data_tag {
    node_rule_t rule;
    node_reference_t reference;
    node_string_t string;
    node_charclass_t charclass;
    node_quantity_t quantity;
    node_predicate_t predicate;
    node_sequence_t sequence;
    node_alternate_t alternate;
    node_capture_t capture;
    node_expand_t expand;
    node_action_t action;
    node_test_t test;
    node_error_t error;
} node_data_t;

struct node_tag {
    node_type_t type;
    node_data_t data;
};

typedef struct section_tag section_t;
typedef struct parse_callback_tag parse_callback_t ;
typedef void (*parse_callback_fn)(parse_callback_t *ctx);

struct section_tag {
    const char *name;
    const char *data;
    struct section_tag *next;
};

typedef struct context_tag {
    char *iname;
    char *sname;
    char *hname;
    FILE *ifile;
    FILE *sfile;
    FILE *hfile;
    char *hid;
    char *vtype;
    char *atype;
    char *prefix;
    bool_t debug;
    int errnum;
    int linenum;
    int linepos;
    int bufpos;
    char_array_t buffer;
    node_array_t rules;
    node_hash_table_t rulehash;
    section_t *sections;
} context_t;

typedef struct generate_tag {
    FILE *stream;
    const node_t *rule;
    int label;
} generate_t;

typedef enum string_flag_tag {
    STRING_FLAG__NONE = 0,
    STRING_FLAG__NOTEMPTY = 1,
    STRING_FLAG__NOTVOID = 2,
    STRING_FLAG__IDENTIFIER = 4,
} string_flag_t;

typedef enum code_reach_tag {
    CODE_REACH__BOTH = 0,
    CODE_REACH__ALWAYS_SUCCEED = 1,
    CODE_REACH__ALWAYS_FAIL = -1
} code_reach_t;

struct parse_callback_tag {
    parse_callback_fn call;
    const char *name;
    const char *src_dir;
    const char *out_dir;
    const char *tmp_dir;
    node_array_t *rules;
    section_t *sections;
    const char *option_value;
    const char *option_auxil;
    const char *option_prefix;
};

int packcc(const char *iname, const char *oname, int debug);
int packcc_transform(const char *iname, const char *oname, parse_callback_t *parse_ctx);
