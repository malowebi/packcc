/*
 * PackCC: a packrat parser generator for C.
 *
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
#define PACKCC_LIBRARY

#include "packcc.c"

// #include <stdio.h>
// #include <stdlib.h>
// #include <stdarg.h>
// #include <string.h>
// #include <assert.h>
// #include <fcntl.h>

#ifdef _MSC_VER
#include <io.h>
#define dup2 _dup2
#define open _open
#define close _close
#define fileno _fileno
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#define O_APPEND _O_APPEND
#define O_WRONLY _O_WRONLY
#else
#include <unistd.h>
#endif

int packcc(const char *iname, const char *oname, int debug) {
  context_t *ctx = create_context(iname, oname, (bool_t)debug);

  const char *err_path = replace_fileext(ctx->sname, "log");
  const char *out_path = replace_fileext(ctx->sname, "spec");

  int b = parse(ctx) && generate(ctx);

  destroy_context(ctx);

  free((void *)err_path);
  free((void *)out_path);

  if (!b) {
    return 10;
  }

  return 0;
}

int packcc_transform(const char *iname, const char *oname, parse_callback_t *parse_ctx) {
  context_t *ctx = create_context(iname, oname, (bool_t)0);

  int b = parse(ctx);

  if (b && parse_ctx && parse_ctx->call) {
    parse_ctx->rules          = &ctx->rules;
    parse_ctx->sections       = ctx->sections;
    parse_ctx->option_value   = get_value_type(ctx);
    parse_ctx->option_auxil   = get_auxil_type(ctx);
    parse_ctx->option_prefix  = get_prefix(ctx);

    parse_ctx->call(parse_ctx);
  }

  destroy_context(ctx);

  return b ? 0 : 1;
}
