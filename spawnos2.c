#include "jam.h"
#include "lists.h"
#include "rope.h"
#include "spawn.h"


/* XXX: this code has not been compiled yet !! */
/* David Turner 2007-05-13                     */

#ifdef USE_SPAWNOS2

#if 1
#define  XLOG(x)  do{}while(0)
#else
#define  XLOG(x)  printf x, fflush(NULL)
#endif

#define  INCL_DOS
#define  INCL_DOS_ERRORS
#define  INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>

#define  NULL_HFILE  ((HFILE)0xFFFFFFFF)

/* close a handle */
static void
_spawn_os2_close( HFILE  *phandle )
{
  HFILE  handle = *phandle;

  if ( handle != NULL_HFILE )
  {
    *phandle = NULL_HFILE;
    DosClose( handle );
  }
}


static int
_spawn_os2_set_inherit( HFILE  handle,
                        int    inherit )
{
  ULONG  mode;

  if ( DosQueryFHState( handle, &mode ) )
    return 0;

  if ( inherit )
    mode &= ~OPEN_FLAGS_NO_INHERIT;
  else
    mode |= OPEN_FLAGS_NO_INHERIT;

  if ( DosSetFHState( handle, mode ) )
    return 0;

  return 1;
}


static int
_spawn_os2_dup( HFILE   origin, HFILE*  ptarget, int  inherit )
{
  if ( DosDupHandle( origin, ptarget ) )
    return 0;

  if ( !_spawn_os2_setnoinherit( *ptarget, inherit ) )
  {
    DosClose( *ptarget );
    *ptarget = NULL_HFILE;
    return 0;
  }
  return 1;
}


static int
_spawn_os2_create_pipe( HFILE*  pread, HFILE*  pwrite )
{
  APIRET  urc;
  HFILE   read, write;

  *pread  = NULL_HFILE;
  *pwrite = NULL_HFILE;

  urc = DosCreatePipe( &read, &write, 4096 );
  if ( urc != 0 )
    return 0;

  *pread  = read;
  *pwrite = write;
  return 1;
}


static const char*
_spawn_os2_make_shell_command_line( const char*  shell,
                                    LIST*        command,
                                    Rope*        rope )
{
  rope_init( rope );

  /* first string is the program name by convention */
  rope_append( rope, shell )
  rope_addc( rope, 0 );

  /* second string contains the parameters themselves */
  rope_append( rope, "/c" );
  for ( ; command; command = command->next )
  {
    rope_addc( rope, ' ' );
    rope_append( rope, command->string );
  }
  rope_addc( rope, '\0' );

  /* second string followed by another zero */
  rope_addc( rope, '\0' );

  return (const char*)rope->data;
}


int  spawn( LIST*      command,
            Rope*      output )
{
  Rope                 command_rope;
  const char*          command_line = NULL;
  char*                exec_path    = NULL;
  PROCESS_INFORMATION  proc_info;
  STARTUPINFO          startInfo;
  int                  result = 0;
  HFILE                pipe_read, pipe_write;

  rope_init( output );
  rope_init( &command_rope );

  pipe_read   = NULL_HFILE;
  pipe_write  = NULL_HFILE;

  if ( command == NULL || command->string == NULL ) {
    XLOG(( "empty command line\n" ));
    return 0;
  }

  XLOG(( "command start is %s!\n", command->string ));

  {
    const char*  shell = getenv("COMSPEC");

    if (shell == NULL)
    {
      XLOG(( "could not find command shell !!\n" ));
      result = -1;
      goto Exit;
    }

    exec_path    = (char*)shell;
    command_line = _spawn_os2_make_shell_command_line( shell, command,
                                                       &command_rope );
  }

  if ( command_line == NULL )
  {
    XLOG(( "not enough memory to allocate command line string\n" ));
    result = -1;
    goto Exit;
  }

  XLOG(( "command line is: '%s'\n", command_line ));

  if ( !_spawn_os2_create_pipe( &pipe_read, &pipe_write ) )
  {
    XLOG(( "could not create pipe\n" ));
    result = -1;
    goto Exit;
  }

  XLOG(( "pipe read=%p write=%p\n", pipe_read, pipe_write ));

  {
        char         errbuff[256];
        RESULTCODES  result = { 0 };
        ULONG        ret;
        HFILE        save_out = INVALID_HFILE;
        HFILE        new_out  = write_pipe;

        /* save old stdout */
        ret = DosDupHandle(STDOUT_FILENO, &save_out);
        if (ret) {
            XLOG(( "not enough free handles to save old stdout\n" ));
            result = -2;
            goto Exit;
        }

        /* redirect stdout to our write pipe */
        ret = DosDupHandle(write_pipe, &new_out);
        if (ret) {
            DosClose( save_out );
            XLOG(( "unable to redirect stdout to write pipe\n" ));
            result = -2;
            goto Exit:
        }

        if (!_spawn_os2_set_inherit( new_out, 1 )) {
            new_out = STDOUT_FILENO;
            DosDupHandle( save_out, &new_out );
            DosClose( save_out );
            XLOG(( "could not set redirected stdout to inheritable\n" ));
            result = -2;
            goto Exit;
        }

        /* try to launch the command */
        ret = DosExecPgm( errbuff, sizeof(errbuff),
                          EXEC_ASYNC,
                          command_line,
                          NULL,
                          &result,
                          shell );

        /* restore standard output */
        new_out = STDOUT_FILENO;
        DosDupHandle( save_out, &new_out );
        DosClose( save_out );

        if (ret != 0) {
           /* we couldn't start the process for some reason. for now, don't
            * be too explicit about the reason...
            */
            XLOG(( "could not launch child process\n" ));
            result = -2;
            goto Exit;
        }
  }

  _spawn_os2_close( &pipe_write );

  for (;;)
  {
    ULONG    nread, ret;
    char     buff[256];

    ret = DosRead( pipe_read, buff, sizeof(buff), &nread );
    if (nread == 0 || ret != 0)
      break;

    if ( rope_add( output, buff, (int)nread ) < 0 )
      break;
  }

Exit:
  rope_done( &command_rope );

  _spawn_os2_close( &pipe_read );
  _spawn_os2_close( &pipe_write );

  return  result;
}

#endif /* US_SPAWNOS2 */

