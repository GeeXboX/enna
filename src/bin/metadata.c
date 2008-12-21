#include "enna.h"

#define MODULE_NAME "enna"

#define PATH_COVERS             "covers"
#define PATH_SNAPSHOTS          "snapshots"

static Eina_List *metadata_grabbers = NULL;

Enna_Metadata *
enna_metadata_new (char *uri)
{
    Enna_Metadata *m;

    if (!uri)
      return NULL;
    
    m = calloc(1, sizeof(Enna_Metadata));
    m->video = calloc(1, sizeof(Enna_Metadata_Video));
    m->music = calloc(1, sizeof(Enna_Metadata_Music));

    m->type = ENNA_METADATA_UNKNOWN;
    m->uri = strdup (uri);
    m->md5 = md5sum (uri);
    
    return m;
}

void enna_metadata_free(Enna_Metadata *m)
{

    if (!m)
        return;

    ENNA_FREE(m->uri);
    ENNA_FREE(m->md5);
    ENNA_FREE(m->keywords);
    ENNA_FREE(m->title);
    ENNA_FREE(m->overview);
    ENNA_FREE(m->categories);
    ENNA_FREE(m->cover);
    ENNA_FREE(m->snapshot);
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
enna_metadata_init (void)
{
    char dst[1024];
            
    /* try to create covers directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_COVERS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* try to create snapshots directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_SNAPSHOTS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);
}

void
enna_metadata_add_keywords (Enna_Metadata *meta, char *keywords)
{
    char key[1024];
    
    if (!meta || !keywords)
        return;

    if (!meta->keywords)
        meta->keywords = strdup (keywords);
    else
    {
        memset (key, '\0', sizeof (key));
        snprintf (key, sizeof (key), "%s %s", meta->keywords, keywords);
        free (meta->keywords);
        meta->keywords = strdup (key);
    }

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Metadata keywords set to '%s'", meta->keywords);
}

void
enna_metadata_add_grabber (Enna_Metadata_Grabber *grabber)
{
    Eina_List *tmp;
    
    if (!grabber || !grabber->name)
        return;

    tmp = eina_list_nth_list (metadata_grabbers, 0);
    do {
        Enna_Metadata_Grabber *g = NULL;

        if (tmp)
            g = (Enna_Metadata_Grabber *) tmp->data;
        
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
        Enna_Metadata_Grabber *g = NULL;

        if (tmp)
            g = (Enna_Metadata_Grabber *) tmp->data;
        
        if (g && !strcmp (g->name, name))
        {
            tmp = eina_list_remove (tmp, g);
            return;
        }
    } while ((tmp = eina_list_next (tmp)));
}

void
enna_metadata_grab (Enna_Metadata *meta, int caps)
{
    int i;

    if (!meta)
        return;
    
    for (i = ENNA_GRABBER_PRIORITY_MAX; i < ENNA_GRABBER_PRIORITY_MIN; i++)
    {
        Eina_List *tmp;

        tmp = eina_list_nth_list (metadata_grabbers, 0);
        do {
            Enna_Metadata_Grabber *g = NULL;

            if (tmp)
                g = (Enna_Metadata_Grabber *) tmp->data;
            if (!g)
                continue;

            /* check for grabber's priority */
            if (g->priority != i)
                continue;
            
            /* check if network is allowed */
            if (g->require_network && !enna->use_network)
                continue;
            
            /* check if grabber has the requested capabilities */
            if (g->caps & caps)
                g->grab (meta, caps);
        } while ((tmp = eina_list_next (tmp)));
    }
}
