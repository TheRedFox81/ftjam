#include "jam.h"
#include "lists.h"
#include "rope.h"
#include "spawn.h"


#ifdef USE_SPAWNNT

#if 1
#define  XLOG(x)  do{}while(0)
#else
#define  XLOG(x)  printf x, fflush(NULL)
#endif

#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

/* close a handle
 */
static void
_spawn_win32_close( HANDLE  *phandle )
{
  HANDLE  handle = *phandle;

  if ( handle != INVALID_HANDLE_VALUE )
  {
    *phandle = INVALID_HANDLE_VALUE;
    CloseHandle( handle );
  }
}


/* create a pipe
 */
static int
_spawn_win32_create_pipe( HANDLE*  pread, HANDLE*  pwrite )
{
  SECURITY_ATTRIBUTES inherit;
  BYTE                sd[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
  HANDLE              read, write;

  if ( !InitializeSecurityDescriptor( (PSECURITY_DESCRIPTOR)&sd,
                                      SECURITY_DESCRIPTOR_REVISION) )
    return 0;

  inherit.nLength              = sizeof(inherit);
  inherit.lpSecurityDescriptor = & sd;
  inherit.bInheritHandle       = TRUE;

  if ( !CreatePipe( &read, &write, &inherit, 0 ) )
    return 0;

  if ( !SetHandleInformation( read, HANDLE_FLAG_INHERIT, 0 ) )
  {
    CloseHandle( read );
    CloseHandle( write );
    return 0;
  }

  *pread  = read;
  *pwrite = write;
  return 1;
}


static const char*
_spawn_win32_make_shell_command_line( const char*  shell,
                                      LIST*        command,
                                      Rope*        rope )
{
  rope_init( rope );
  rope_append( rope, shell );
  rope_append( rope, " /c" );
  for ( ; command; command = command->next )
  {
    rope_addc( rope, ' ' );
    rope_append( rope, command->string );
  }
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
  HANDLE               pipe_read, pipe_write;

  rope_init( output );
  rope_init( &command_rope );

  pipe_read  = INVALID_HANDLE_VALUE;
  pipe_write = INVALID_HANDLE_VALUE;

  if ( command == NULL || command->string == NULL ) {
    XLOG(( "empty command line\n" ));
    return 0;
  }

  XLOG(( "command line is %s!\n", command->string ));

  {
    const char*  shell = getenv("COMSPEC");

    if (shell == NULL)
    {
      XLOG(( "could not find command shell !!\n" ));
      result = -1;
      goto Exit;
    }

    exec_path    = (char*)shell;
    command_line = _spawn_win32_make_shell_command_line( shell, command,
                                                         &command_rope );
  }

  if ( command_line == NULL )
  {
    XLOG(( "not enough memory to allocate command line string\n" ));
    result = -1;
    goto Exit;
  }

  XLOG(( "command line is: '%s'\n", command_line ));

  if ( !_spawn_win32_create_pipe( &pipe_read, &pipe_write ) )
  {
    XLOG(( "could not create pipe\n" ));
    result = -1;
    goto Exit;
  }

  XLOG(( "pipe read=%p write=%p\n", pipe_read, pipe_write ));

  /* we redirect the child's stdout to our pipe's write end */
  GetStartupInfo(&startInfo);

  startInfo.dwFlags     = STARTF_USESTDHANDLES;
  startInfo.lpReserved  = 0;
  startInfo.cbReserved2 = 0;
  startInfo.lpReserved2 = 0;
  startInfo.lpTitle     = exec_path ;
  startInfo.hStdOutput  = pipe_write;
  /* startInfo.hStdError   = pipe_write;   XXX: do we need this ? */

  if ( !CreateProcess( exec_path,
                       (char*)command_line,
                       NULL,        /* child processes don't need the PID */
                       NULL,          /* child threads don't need the PID */
                       TRUE,                        /* inherit handles !! */
                       NORMAL_PRIORITY_CLASS,
                       NULL,                                /* environment */
                       NULL,                               /* working path */
                       &startInfo,
                       &proc_info ) )
  {
   /* we couldn't start the process for some reason. for now, don't
    * be too explicit about the reason...
    */
    XLOG(( "could not launch child process\n" ));
    result = -2;
    goto Exit;
  }

  _spawn_win32_close( &pipe_write );

  for (;;)
  {
    DWORD    nread;
    char     c;

    if ( !ReadFile( pipe_read, &c, 1, &nread, NULL ) )
      break;

    if ( rope_addc( output, c ) < 0 )
      break;
  }

Exit:
  rope_done( &command_rope );

  _spawn_win32_close( &pipe_read );
  _spawn_win32_close( &pipe_write );

  return  result;
}

#endif /* US_SPAWNNT */

