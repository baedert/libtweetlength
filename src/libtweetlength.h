#ifndef __LIBTWEETLENGTH_H__
#define __LIBTWEETLENGTH_H__

#include <glib.h>


gsize tl_count_characters   (const char *input);
gsize tl_count_characters_n (const char *input,
                             gsize       length);


#endif
