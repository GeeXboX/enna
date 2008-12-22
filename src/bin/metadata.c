#include "enna.h"

#define MODULE_NAME "enna"

#define PATH_BACKDROPS          "backdrops"
#define PATH_COVERS             "covers"
#define PATH_METADATA           "metadata"
#define PATH_SNAPSHOTS          "snapshots"

#define DEBUG 0
#define EET_DO_COMPRESS 1

static Eina_List *metadata_grabbers = NULL;
static Eet_File *ef;

#if DEBUG == 1
static void
enna_metadata_video_dump (Enna_Metadata_Video *m)
{
    if (!m)
        return;

    printf ("*** Metadata Video:\n");
    printf (" -- Codec: %s\n",      m->codec);
    printf (" -- Width: %d\n",      m->width);
    printf (" -- Height: %d\n",     m->height);
    printf (" -- Aspect: %f\n",     m->aspect);
    printf (" -- Channels: %d\n",   m->channels);
    printf (" -- Streams: %d\n",    m->streams);
    printf (" -- FrameRate: %f\n",  m->framerate);
    printf (" -- BitRate: %d\n",    m->bitrate);
}

static void
enna_metadata_music_dump (Enna_Metadata_Music *m)
{
    if (!m)
        return;

    printf ("*** Metadata Music:\n");
    printf (" -- Artist: %s\n",      m->artist);
    printf (" -- Album: %s\n",       m->album);
    printf (" -- Year: %s\n",        m->year);
    printf (" -- Genre: %s\n",       m->genre);
    printf (" -- Comment: %s\n",     m->comment);
    printf (" -- DiscID: %s\n",      m->discid);
    printf (" -- Track: %d\n",       m->track);
    printf (" -- Rating: %d\n",      m->rating);
    printf (" -- Play_Count: %d\n",  m->play_count);
    printf (" -- Codec: %s\n",       m->codec);
    printf (" -- Bitrate: %d\n",     m->bitrate);
    printf (" -- Channels: %d\n",    m->channels);
    printf (" -- SampleRate: %d\n",  m->samplerate);
}

static void
enna_metadata_dump (Enna_Metadata *m)
{
    if (!m)
        return;

    printf ("*** Metadata:\n");
    printf (" -- Type: %d\n",       m->type);
    printf (" -- URI: %s\n",        m->uri);
    printf (" -- MD5: %s\n",        m->md5);
    printf (" -- Keywords: %s\n",   m->keywords);
    printf (" -- Title: %s\n",      m->title);
    printf (" -- Size: %d\n",       m->size);
    printf (" -- Length: %d\n",     m->length);
    printf (" -- Overview: %s\n",   m->overview);
    printf (" -- Runtime: %d\n",    m->runtime);
    printf (" -- Year: %d\n",       m->year);
    printf (" -- Categories: %s\n", m->categories);
    printf (" -- Cover: %s\n",      m->cover);
    printf (" -- Snapshot: %s\n",   m->snapshot);
    printf (" -- Backdrop: %s\n",   m->backdrop);
    printf (" -- Parsed: %d\n",     m->parsed);

    enna_metadata_video_dump (m->video);
    enna_metadata_music_dump (m->music);
}
#endif

static Eina_Hash *
eet_eina_hash_add (Eina_Hash *hash, const char *key, const void *data)
{
    if (!hash)
        hash = eina_hash_string_superfast_new (NULL);

    if (!hash)
        return NULL;

    eina_hash_add (hash, key, data);
    return hash;
}

static Eet_Data_Descriptor *
enna_metadata_video_desc (void)
{
    Eet_Data_Descriptor *edd;
    
    edd = eet_data_descriptor_new ("Enna_Metadata_Video",
                                   sizeof (Enna_Metadata_Video),
                                   eina_list_next,
                                   eina_list_append,
                                   eina_list_data_get,
                                   eina_list_free,
                                   eina_hash_foreach,
                                   eet_eina_hash_add,
                                   eina_hash_free);
    
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "codec", codec, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "width", width, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "height", height, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "aspect", aspect, EET_T_FLOAT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "channels", channels, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "streams", streams, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "framerate", framerate, EET_T_FLOAT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Video,
                                   "bitrate", bitrate, EET_T_INT);

    return edd;
}

static Eet_Data_Descriptor *
enna_metadata_music_desc (void)
{
    Eet_Data_Descriptor *edd;

    edd = eet_data_descriptor_new ("Enna_Metadata_Music",
                                   sizeof (Enna_Metadata_Music),
                                   eina_list_next,
                                   eina_list_append,
                                   eina_list_data_get,
                                   eina_list_free,
                                   eina_hash_foreach,
                                   eet_eina_hash_add,
                                   eina_hash_free);
   
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "artist", artist, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "album", album, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "year", year, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "genre", genre, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "comment", comment, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "discid", discid, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "track", track, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "rating", rating, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "play_count", play_count, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "codec", codec, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "bitrate", bitrate, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "channels", channels, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata_Music,
                                   "samplerate", samplerate, EET_T_INT);

    return edd;
}

static Eet_Data_Descriptor *
enna_metadata_desc (void)
{
    Eet_Data_Descriptor *edd, *edd_video, *edd_music;

    edd = eet_data_descriptor_new ("Enna_Metadata", sizeof (Enna_Metadata),
                                   eina_list_next,
                                   eina_list_append,
                                   eina_list_data_get,
                                   eina_list_free,
                                   eina_hash_foreach,
                                   eet_eina_hash_add,
                                   eina_hash_free);

    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "type", type, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "uri", uri, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "md5", md5, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "keywords", keywords, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "title", title, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "size", size, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "length", length, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "overview", overview, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "runtime", runtime, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "year", year, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "categories", categories, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "cover", cover, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "snapshot", snapshot, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "backdrop", backdrop, EET_T_STRING);
    EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata,
                                   "parsed", parsed, EET_T_INT);

    edd_video = enna_metadata_video_desc ();
    EET_DATA_DESCRIPTOR_ADD_SUB (edd, Enna_Metadata,
                                 "video", video, edd_video);

    edd_music = enna_metadata_music_desc ();
    EET_DATA_DESCRIPTOR_ADD_SUB (edd, Enna_Metadata,
                                 "music", music, edd_music);
    
    return edd;
}
    
static Enna_Metadata *
enna_metadata_load_from_eet (char *md5)
{
    Enna_Metadata *m = NULL;
    Eet_Data_Descriptor *edd;
    char file[1024];
    
    if (!md5)
        return NULL;

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Trying to load %s from EET.\n", md5);

    memset (file, '\0', sizeof (file));
    snprintf (file, sizeof (file), "%s/.enna/%s/%s.eet",
              enna_util_user_home_get (), PATH_METADATA, md5);
    
    ef = eet_open (file, EET_FILE_MODE_READ);
    if (!ef)
        return NULL;

    edd = enna_metadata_desc ();
    m = eet_data_read (ef, edd, md5);
    if (!m)
    {
        eet_data_descriptor_free (edd);
        return NULL;
    }
    
#if DEBUG == 1
    enna_metadata_dump (m);
#endif

    eet_data_descriptor_free (edd);
    eet_close (ef);

    return m;
}

static void
enna_metadata_save_to_eet (Enna_Metadata *m)
{
    Eet_Data_Descriptor *edd;
    char file[1024];
    
    if (!m)
        return;

    memset (file, '\0', sizeof (file));
    snprintf (file, sizeof (file), "%s/.enna/%s/%s.eet",
              enna_util_user_home_get (), PATH_METADATA, m->md5);
    
    ef = eet_open (file, EET_FILE_MODE_WRITE);
    if (!ef)
        return;

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Trying to save %s to EET.\n", m->md5);
              
    edd = enna_metadata_desc ();
    if (!eet_data_write (ef, edd, m->md5, m, EET_DO_COMPRESS))
        enna_log (ENNA_MSG_WARNING, MODULE_NAME,
                  "Error writing EET data.\n");
    
    eet_data_descriptor_free (edd);
    eet_close (ef);
}

Enna_Metadata *
enna_metadata_new (char *uri)
{
    Enna_Metadata *m;
    char *md5;
    
    if (!uri)
      return NULL;

    md5 = md5sum (uri);
    m = enna_metadata_load_from_eet (md5);
    free (md5);
    if (m)
        return m;
    
    m = calloc(1, sizeof(Enna_Metadata));
    m->video = calloc(1, sizeof(Enna_Metadata_Video));
    m->music = calloc(1, sizeof(Enna_Metadata_Music));

    m->type = ENNA_METADATA_UNKNOWN;
    m->uri = strdup (uri);
    m->md5 = md5sum (uri);
    m->parsed = 0;
    
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
            
    /* try to create backdrops directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_BACKDROPS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* try to create covers directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_COVERS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* try to create metadata directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_METADATA);
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
enna_metadata_add_category (Enna_Metadata *meta, char *category)
{
    char cat[1024];
    
    if (!meta || !category)
        return;

    if (!meta->categories)
        meta->categories = strdup (category);
    else
    {
        memset (cat, '\0', sizeof (cat));
        snprintf (cat, sizeof (cat), "%s, %s", meta->categories, category);
        free (meta->categories);
        meta->categories = strdup (cat);
    }
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

    /* avoid parsing the same resource twice */
    if (meta->parsed)
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

    meta->parsed = 1;
    enna_metadata_save_to_eet (meta);
}
