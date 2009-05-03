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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

#include <Eina.h>
#include <Ecore_Str.h>
#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "vfs.h"

#ifdef BUILD_LIBSVDRP
static svdrp_t *svdrp = NULL;
#endif

char * enna_util_user_home_get()
{
    static char *home = NULL;

    if (home)
        return home;

    home = strdup(getenv("HOME"));
    if (!home)
        return strdup(getenv("CWD"));
    return home;
}

int enna_util_has_suffix(char *str, Eina_List * patterns)
{
    Eina_List *l;
    int result = 0;

    int i;
    char *tmp;

    if (!patterns || !str || !str[0])
        return 0;

    for (l = patterns; l; l = eina_list_next(l))
    {
        tmp = calloc(1, strlen(str) + 1);
        for (i = 0; i < strlen(str); i++)
            tmp[i] = tolower(str[i]);
        result |= ecore_str_has_suffix(tmp, (char *)l->data);
        ENNA_FREE(tmp);
    }
    return result;
}

unsigned char enna_util_uri_has_extension(const char *uri, int type)
{

    Eina_List *l;
    Eina_List *filters = NULL;

    if (type == ENNA_CAPS_MUSIC)
        filters = enna_config->music_filters;
    else if (type == ENNA_CAPS_VIDEO)
        filters = enna_config->video_filters;
    else if (type == ENNA_CAPS_PHOTO)
        filters = enna_config->photo_filters;

    if (!filters)
        return 0;

    for (l = filters; l; l = l->next)
    {
        const char *ext = l->data;
        if (ecore_str_has_extension(uri, ext))
            return 1;
    }

    return 0;

}

unsigned int enna_util_calculate_font_size(Evas_Coord w, Evas_Coord h)
{
    float size = 12;

    size = sqrt(w * w + h * h) / (float)50.0;
    size = MMIN(size, 8);
    size = MMAX(size, 30);

    return (unsigned int)size;

}

void enna_util_switch_objects(Evas_Object * container, Evas_Object * obj1, Evas_Object * obj2)
{
    Evas_Object *s;

    if (!obj1 && !obj2)
        return;

    if ((obj1 && obj2))
    {
        s = edje_object_part_swallow_get(container, "enna.switcher.swallow2");
        edje_object_part_swallow(container, "enna.switcher.swallow1", obj2);
        edje_object_part_swallow(container, "enna.switcher.swallow2", obj1);
        if (s)
            evas_object_hide(s);

        edje_object_signal_emit(container, "enna,swallow2,default,now", "enna");
        edje_object_signal_emit(container, "enna,swallow1,state1,now", "enna");
        edje_object_signal_emit(container, "enna,swallow1,default", "enna");
        edje_object_signal_emit(container, "enna,swallow2,state2", "enna");
    }
    else if (!obj2)
    {
        edje_object_part_swallow(container, "enna.switcher.swallow2", obj1);
        edje_object_signal_emit(container, "enna,swallow2,default,now", "enna");
        edje_object_signal_emit(container, "enna,swallow2,state2", "enna");
    }
    else if (!obj1)
    {
        edje_object_part_swallow(container, "enna.switcher.swallow1", obj2);
        edje_object_signal_emit(container, "enna,swallow1,state1,now", "enna");
        edje_object_signal_emit(container, "enna,swallow1,default", "enna");
    }
}

#define MD5_SIZE 33
#define MD5_SUM_SIZE 16

typedef struct md5_s {
    uint64_t len;
    uint8_t block[64];
    uint32_t ABCD[4];
} md5_t;

const int md5_size = sizeof (md5_t);

static const uint8_t S[4][4] = {
    { 7, 12, 17, 22 }, /* Round 1 */
    { 5,  9, 14, 20 }, /* Round 2 */
    { 4, 11, 16, 23 }, /* Round 3 */
    { 6, 10, 15, 21 }  /* Round 4 */
};

static const uint32_t T[64] = { // T[i]= fabs(sin(i+1)<<32)
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, /* Round 1 */
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af,
    0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e,
    0x49b40821,

    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, /* Round 2 */
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6,
    0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9,
    0x8d2a4c8a,

    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, /* Round 3 */
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
    0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8,
    0xc4ac5665,

    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, /* Round 4 */
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0,
    0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb,
    0xeb86d391,
};

#define CORE(i, a, b, c, d)                                               \
        t = S[i >> 4][i & 3];                                             \
        a += T[i];                                                        \
                                                                          \
        if (i < 32) {                                                     \
            if (i < 16) a += (d ^ (b & (c ^ d))) + X[           i & 15 ]; \
            else        a += (c ^ (d & (c ^ b))) + X[ (1 + 5 * i) & 15 ]; \
        } else {                                                          \
            if (i < 48) a += (b ^ c ^d)          + X[ (5 + 3 * i) & 15 ]; \
            else        a += (c ^ (b |~ d))      + X[ (  7 * i)   & 15 ]; \
        }                                                                 \
        a = b + (( a << t ) | ( a >> (32 - t) ));

#define CORE2(i) \
        CORE(i,a,b,c,d) \
        CORE((i+1),d,a,b,c) \
        CORE((i+2),c,d,a,b) \
        CORE((i+3),b,c,d,a)

#define CORE4(i) \
        CORE2(i) \
        CORE2((i+4)) \
        CORE2((i+8)) \
        CORE2((i+12))

static void
body (uint32_t ABCD[4], uint32_t X[16])
{
    int t;
    unsigned int a = ABCD[3];
    unsigned int b = ABCD[2];
    unsigned int c = ABCD[1];
    unsigned int d = ABCD[0];

    CORE4 (0);
    CORE4 (16);
    CORE4 (32);
    CORE4 (48);

    ABCD[0] += d;
    ABCD[1] += c;
    ABCD[2] += b;
    ABCD[3] += a;
}

static void
md5_init (md5_t *ctx)
{
    ctx->len = 0;

    ctx->ABCD[0] = 0x10325476;
    ctx->ABCD[1] = 0x98badcfe;
    ctx->ABCD[2] = 0xefcdab89;
    ctx->ABCD[3] = 0x67452301;
}

static void
md5_update (md5_t *ctx, const uint8_t *src, const int len)
{
    int i, j;

    j = ctx->len & 63;
    ctx->len += len;

    for (i = 0; i < len; i++)
    {
        ctx->block[j++] = src[i];
        if (j == 64)
        {
            body (ctx->ABCD, (uint32_t*) ctx->block);
            j = 0;
        }
    }
}

static void
md5_final (md5_t *ctx, uint8_t *dst)
{
    int i;
    uint64_t finalcount = ctx->len << 3;

    md5_update (ctx, (const uint8_t *) "\200", 1);
    while ((ctx->len & 63) < 56)
        md5_update (ctx, (const uint8_t *) "", 1);

    md5_update (ctx, (uint8_t *) &finalcount, 8);

    for (i = 0; i < 4; i++)
        ((uint32_t *) dst)[i] = ctx->ABCD[3-i];
}

static void
md5_sum (uint8_t *dst, const uint8_t *src, const int len)
{
    md5_t ctx[1];

    md5_init (ctx);
    md5_update (ctx, src, len);
    md5_final (ctx, dst);
}

char *
md5sum (char *str)
{
    unsigned char sum[MD5_SUM_SIZE];
    char md5[MD5_SIZE];
    int i;

    if (!str)
        return NULL;

    md5_sum (sum, (const uint8_t *) str, strlen(str));
    memset (md5, '\0', MD5_SIZE);

    for (i = 0; i < MD5_SUM_SIZE; i++)
    {
        char tmp[3];
        sprintf (tmp, "%02x", sum[i]);
        strcat (md5, tmp);
    }

    return strdup (md5);
}

#ifdef BUILD_LIBSVDRP
svdrp_t *enna_svdrp_init (char* host, int port, int timeout, svdrp_verbosity_level_t verbosity)
{
    svdrp = svdrp_open (host, port, timeout, verbosity);
    
    return svdrp;
}

void enna_svdrp_uninit (void)
{
    svdrp_close (svdrp);
    svdrp = NULL;
}

svdrp_t *enna_svdrp_get (void)
{
    return svdrp;
}
#endif
