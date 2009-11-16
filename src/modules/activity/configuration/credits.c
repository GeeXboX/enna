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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "buffer.h"

/* local globals */
static Evas_Object *o_edje = NULL;

Evas_Object *credits_panel_show(void *data)
{
    buffer_t *b;

    o_edje = edje_object_add(enna->evas);
    edje_object_file_set(o_edje, enna_config_theme_get (),
                         "activity/configuration/credits");

    b = buffer_new();
    buffer_append (b, _("Enna is GeeXboX's team MediaCenter, "));
    buffer_append (b, _("based on Enlightenment Foundation Librairies (EFL)."));
    buffer_append (b, "<br><br>");
    buffer_append (b, _("Credits go to:<br>"));
    buffer_append (b, "Nicolas Aguirre, Fabien Brisset, Davide Cavalca, ");
    buffer_append (b, "Matthias Holzer, Guillaume Lecerf, Mathieu Schroeter ");
    buffer_append (b, "and Benjamin Zores.");
    edje_object_part_text_set (o_edje, "credits.text", b->buf);
    buffer_free(b);

    edje_object_signal_emit (o_edje, "credits,show", "enna");

    return o_edje;
}

void credits_panel_hide(void *data)
{
    ENNA_OBJECT_DEL(o_edje);
}
