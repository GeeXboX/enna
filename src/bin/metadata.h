#ifndef __ENNA_METADATA_H__
#define __ENNA_METADATA_H__

typedef struct _Enna_Metadata Enna_Metadata;
typedef struct _Enna_Metadata_Video Enna_Metadata_Video;
typedef struct _Enna_Metadata_Music Enna_Metadata_Music;
typedef struct _Enna_Metadata_Grabber Enna_Metadata_Grabber;

struct _Enna_Metadata_Music
{

    char *artist;
    char *album;
    char *year;
    char *genre;
    char *comment;
    char *discid;
    int track;
    int rating;
    int play_count;
    char *codec;
    int bitrate; /* in Bps */
    int channels;
    int samplerate;
};

struct _Enna_Metadata_Video
{
    char *codec;
    int width;
    int height;
    float aspect;
    int channels;
    int streams;
    float framerate;
    int bitrate; /* in Bps */
};

struct _Enna_Metadata
{
    char *keywords;
    char *md5;
    char *uri;
    char *title;
    int size; /* in Bytes */
    int type;
    int length; /* in seconds */
    char *overview;
    int runtime;
    int year;
    char *categories;
    char *cover;
    char *backdrop;
    Enna_Metadata_Video *video;
    Enna_Metadata_Music *music;

};

#define ENNA_GRABBER_CAP_AUDIO       0x0001
#define ENNA_GRABBER_CAP_VIDEO       0x0002
#define ENNA_GRABBER_CAP_PICTURE     0x0004
#define ENNA_GRABBER_CAP_NETWORK     0x0008

#define ENNA_GRABBER_PRIORITY_MAX 1
#define ENNA_GRABBER_PRIORITY_MIN 10

struct _Enna_Metadata_Grabber
{
    char *name;
    int priority;
    int caps;
    void (* grab) (Enna_Metadata *meta, int caps);
};

Enna_Metadata *enna_metadata_new();
void enna_metadata_free(Enna_Metadata *m);

void enna_metadata_add_grabber (Enna_Metadata_Grabber *grabber);
void enna_metadata_remove_grabber (char *name);
Enna_Metadata *enna_metadata_grab (int caps, char *keywords);

#endif
