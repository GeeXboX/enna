/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef UTILS_H
#define UTILS_H

#include <Evas.h>

#define MMAX(a,b) ((a) > (b) ? (a) : (b))
#define MMIN(a,b) ((a) > (b) ? (b) : (a))

char         *enna_util_user_home_get();
int           enna_util_has_suffix(char *str, Eina_List * patterns);
unsigned int  enna_util_calculate_font_size(Evas_Coord w, Evas_Coord h);
void          enna_util_switch_objects(Evas_Object * container, Evas_Object * obj1, Evas_Object * obj2);
unsigned char enna_util_uri_has_extension(const char *uri, int type);
char *md5sum (char *str);
char *init_locale(void);
char *get_locale(void);
char *get_lang(void);

#ifdef BUILD_LIBSVDRP
#include <svdrp.h>
svdrp_t *enna_svdrp_init (char* host, int port, int timeout, svdrp_verbosity_level_t verbosity);
void enna_svdrp_uninit (void);
svdrp_t *enna_svdrp_get (void);
#endif

void enna_util_env_set(const char *var, const char *val);
char *enna_util_str_chomp(char *str);

#endif /* UTILS_H */
