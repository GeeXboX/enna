#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer_s {
    char *buf;
    size_t len;
    size_t capacity;
} buffer_t;

buffer_t *buffer_new (void);
void buffer_free (buffer_t *buffer);

void buffer_append (buffer_t *buffer, const char *str);
void buffer_appendf (buffer_t *buffer, const char *format, ...);

#endif /* BUFFER_H */
