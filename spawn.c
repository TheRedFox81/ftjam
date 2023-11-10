#include "jam.h"
#include "lists.h"
#include "rope.h"
#include "spawn.h"

#ifdef HAVE_POPEN

#include <stdio.h>

#if defined(_MSC_VER) || defined(__BORLANDC__)
    #define popen _popen
    #define pclose _pclose
#endif

int  spawn( LIST*      command,
            Rope*      output )
{
     FILE*        p = NULL;
     Rope         cmd;
     const char*  cmd_string;

     rope_init( output );

     fflush(NULL);

     rope_init( &cmd );

     cmd_string = command->string;
     if ( command->next )
     {
         for ( ; command; command = command->next )
             rope_append( &cmd, command->string );

         cmd_string = rope->data;
     }

     p = popen( cmd_string, "r");

     rope_done( &cmd );

     if ( p == NULL )
         return L0;

     for (;;) {
         ret = fread( buffer, sizeof(char), sizeof(buffer), p );
         if (ret <= 0)
             break;

         rope_add( output, buffer, ret );
     }

     pclose(p);

     return 0;
}

#endif /* HAVE_POPEN */

