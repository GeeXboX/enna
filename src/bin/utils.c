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

#include "enna.h"
#include "utils.h"

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
