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

#ifndef __ENNA_METADATA_H__
#define __ENNA_METADATA_H__

typedef struct _Enna_Metadata Enna_Metadata;
typedef struct _Enna_Metadata_Video Enna_Metadata_Video;
typedef struct _Enna_Metadata_Music Enna_Metadata_Music;
typedef struct _Enna_Metadata_Grabber Enna_Metadata_Grabber;

typedef enum
{
    ENNA_METADATA_UNKNOWN,
    ENNA_METADATA_AUDIO,
    ENNA_METADATA_VIDEO,
    ENNA_METADATA_PHOTO,
} Enna_Metadata_Type;

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
    Enna_Metadata_Type type;
    char *uri;
    char *md5;
    char *keywords;
    char *title;
    int size; /* in Bytes */
    int length; /* in seconds */
    char *overview;
    int runtime;
    int year;
    char *categories;
    char *cover;
    char *snapshot;
    char *backdrop;
    int parsed;
    Enna_Metadata_Video *video;
    Enna_Metadata_Music *music;

};

#define ENNA_GRABBER_CAP_AUDIO       0x0001  /* audio metadata: id3tags ... */
#define ENNA_GRABBER_CAP_VIDEO       0x0002  /* video metadata: fourccs ... */
#define ENNA_GRABBER_CAP_PICTURE     0x0004  /* photo metadata: exif ... */
#define ENNA_GRABBER_CAP_COVER       0x0008  /* covers, thumbs, snapshots */

#define ENNA_GRABBER_PRIORITY_MAX 1
#define ENNA_GRABBER_PRIORITY_MIN 10

struct _Enna_Metadata_Grabber
{
    char *name;
    int priority;
    int require_network;
    int caps;
    void (* grab) (Enna_Metadata *meta, int caps);
};

void enna_metadata_init (void);
Enna_Metadata *enna_metadata_new(char *uri);
void enna_metadata_free(Enna_Metadata *m);
void enna_metadata_add_keywords (Enna_Metadata *meta, char *keywords);
void enna_metadata_add_category (Enna_Metadata *meta, char *category);
void enna_metadata_add_grabber (Enna_Metadata_Grabber *grabber);
void enna_metadata_remove_grabber (char *name);
void enna_metadata_grab (Enna_Metadata *meta, int caps);

#endif
