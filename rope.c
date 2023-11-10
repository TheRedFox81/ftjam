#include "rope.h"
#include <stdlib.h>
#include <memory.h>

void rope_init(Rope*  rope)
{
   rope->len  = 0;
   rope->max  = 0;
   rope->data = NULL;
}


int  rope_grow( Rope*  rope,
                int    extra )
{
  int    newlen = rope->len + extra;
  int    oldmax = rope->max;
  int    newmax;
  char*  newdata;

  if (newlen <= oldmax)
    return 0;

  newmax = oldmax;
  while ( newmax < newlen )
    newmax += (newmax >> 1) + 16;

  newdata = realloc( rope->data, newmax );
  if (newdata == NULL)
    return -1;

  rope->data = newdata;
  rope->max  = newmax;

  return 0;
}


int  rope_add( Rope*  rope, const char*  buff, int  size )
{
  if ( size < 0 || rope_grow(rope, size) )
    return -1;

  memcpy( rope->data + rope->len, buff, size );
  rope->len += size;
  return 0;
}

int   rope_addc( Rope*  rope, int  c )
{
  char  c0 = (char)c;

  return rope_add( rope, &c0, 1 );
}

int rope_append(Rope *rope, const char *buf)
{
   return rope_add(rope, buf, strlen(buf));
}


void rope_done(Rope*  rope)
{
   if (rope->data != NULL) {
       free(rope->data);
       rope->data = NULL;
   }
   rope->len = 0;
   rope->max = 0;
}
