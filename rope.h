#ifndef __ROPE_H__
#define __ROPE_H__

/* A rope is a simple dynamically growing buffer */

typedef struct
{
  int    len;
  int    max;
  char*  data;

} Rope;

extern void  rope_init( Rope*  rope );
extern int   rope_grow( Rope*  rope, int    extra );
extern int   rope_append( Rope*  rope, const char*  str );
extern int   rope_add( Rope*  rope, const char* buff, int  size );
extern int   rope_addc( Rope*  rope, int  c );
extern void  rope_done( Rope*  rope );

#endif /* __ROPE_H__ */
