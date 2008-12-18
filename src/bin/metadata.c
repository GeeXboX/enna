#include "enna.h"

static Eina_List *metadata_grabbers = NULL;

Enna_Metadata *
enna_metadata_new()
{

    Enna_Metadata *m;

    m = calloc(1, sizeof(Enna_Metadata));
    m->video = calloc(1, sizeof(Enna_Metadata_Video));
    m->music = calloc(1, sizeof(Enna_Metadata_Music));
    return m;
}

void enna_metadata_free(Enna_Metadata *m)
{

    if (!m)
        return;

    ENNA_FREE(m->keywords);
    ENNA_FREE(m->uri);
    ENNA_FREE(m->title);
    ENNA_FREE(m->overview);
    ENNA_FREE(m->categories);
    ENNA_FREE(m->cover);
    ENNA_FREE(m->backdrop);
    if (m->video)
    {
        ENNA_FREE(m->video->codec);
        free(m->video);
    }
    if (m->music)
    {
        ENNA_FREE(m->music->artist);
        ENNA_FREE(m->music->album);
        ENNA_FREE(m->music->year);
        ENNA_FREE(m->music->genre);
        ENNA_FREE(m->music->comment);
        ENNA_FREE(m->music->discid);
        ENNA_FREE(m->music->codec);
        free(m->music);
    }
    free(m);
}

void
enna_metadata_add_grabber (Enna_Metadata_Grabber *grabber)
{
    Eina_List *tmp;
    
    if (!grabber || !grabber->name)
        return;

    tmp = eina_list_nth_list (metadata_grabbers, 0);
    do {
        Enna_Metadata_Grabber *g = (Enna_Metadata_Grabber *) tmp->data;
        if (g && !strcmp (g->name, grabber->name))
            return; /* already added grabber */
    } while ((tmp = eina_list_next (tmp)));
    
    metadata_grabbers = eina_list_append (metadata_grabbers, grabber);
}

void
enna_metadata_remove_grabber (char *name)
{
    Eina_List *tmp;
    
    if (!name)
        return;

    tmp = eina_list_nth_list (metadata_grabbers, 0);
    do {
        Enna_Metadata_Grabber *g = (Enna_Metadata_Grabber *) tmp->data;
        if (g && !strcmp (g->name, name))
        {
            tmp = eina_list_remove (tmp, g);
            return;
        }
    } while ((tmp = eina_list_next (tmp)));
}

Enna_Metadata *
enna_metadata_grab (int caps, char *keywords)
{
    Enna_Metadata *meta;
    int i;

    if (!keywords)
        return NULL;
    
    meta = enna_metadata_new ();  
    meta->keywords = strdup (keywords);
    
    for (i = ENNA_GRABBER_PRIORITY_MAX; i < ENNA_GRABBER_PRIORITY_MIN; i++)
    {
        Eina_List *tmp;

        tmp = eina_list_nth_list (metadata_grabbers, 0);
        do {
            Enna_Metadata_Grabber *g = (Enna_Metadata_Grabber *) tmp->data;

            /* check for grabber's priority */
            if (g->priority != i)
                continue;

            /* check if grabber has the requested capabilities */
            if (g->caps & caps)
                g->grab (meta);
        } while ((tmp = eina_list_next (tmp)));
    }

    return meta;
}
