#include "jam.h"
#include "lists.h"
#include "rope.h"
#include "spawn.h"

#ifdef USE_SPAWNUNIX

#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define  xxDEBUG

#ifdef DEBUG
#define  XLOG(x)  printf x, fflush(NULL)
#else
#define  XLOG(x)  do{}while(0)
#endif

static void
_posix_close( int  *pfd )
{
    int  fd = *pfd;
    if (fd >= 0 ) {
        *pfd = -1;
        close( fd );
    }
}

static void
_posix_close_on_exec( int  fd )
{
    fcntl( fd, F_SETFD, FD_CLOEXEC );
}


static int
_posix_dup2( int fd1, int fd2 )
{
    int  ret;
Again:
    ret = dup2( fd1, fd2 );
    if (ret == -1 && errno == EINTR)
        goto Again;

    return  ret;
}


static int
_posix_pipe( int*  fdread, int*  fdwrite )
{
    int  ret;
    int  fds[2];

    *fdread  = -1;
    *fdwrite = -1;
Again:
    ret = pipe( fds );
    if (ret == -1 && errno == EINTR)
        goto Again;

    if (ret == 0) {
        *fdread  = fds[0];
        *fdwrite = fds[1];
    }
    return ret;
}


static const char*
_spawn_check_shell( const char*  path )
{
    struct stat   statistics;
    int           ret;

Again:
    ret = stat( path, &statistics );
    if (ret == -1 && errno == EINTR)
        goto Again;

    if (ret < 0 || !S_ISREG(statistics.st_mode) )
        return NULL;

    return path;
}


static const char**
_spawn_make_shell_command_line( const char*  shell,
                                LIST*        command,
                                Rope*        rope)
{
    int           count;
    LIST*         cmd;
    const char**  result;
    int           first;

    /* this is stupid, the shell wants the command as a single string !! */
    count = 4;
    first = 1;
    for (cmd = command; cmd; cmd = cmd->next) {
        if (first)
            first = 0;
        else
            rope_addc( rope, ' ' );

        rope_append( rope, cmd->string );
    }
    rope_addc( rope, 0 );

    result = calloc( count, sizeof(const char*) );
    if (result != NULL) {
        result[0] = shell;
        result[1] = "-c";
        result[2] = rope->data;
        result[3] = NULL;
    }

    return result;
}


int  spawn( LIST*  command,
            Rope*  output )
{
    const char**  command_line = NULL;
    Rope          command_rope;
    const char*   shell        = NULL;
    char*         exec_path    = NULL;
    int           result = 0;
    int           pipe_read, pipe_write, child;

    rope_init( output );
    rope_init( &command_rope );

    pipe_read  = -1;
    pipe_write = -1;

    if ( command == NULL || command->string == NULL ) {
      XLOG(( "empty command line\n" ));
      return 0;
    }

    XLOG(( "command start is %s!\n", command->string ));

    shell = getenv("SHELL");

    if (!shell)
        shell = _spawn_check_shell("/bin/sh");

    if (!shell)
        shell = _spawn_check_shell("/sbin/sh");

    if (!shell)
    {
        XLOG(( "could not find command shell !!\n" ));
        result = -1;
        goto Exit;
    }

    exec_path    = (char*)shell;
    command_line = _spawn_make_shell_command_line( shell, command, &command_rope );

    if ( command_line == NULL ) {
        XLOG(( "not enough memory to allocate command line string\n" ));
        result = -1;
        goto Exit;
    }

#ifdef DEBUG
    XLOG(( "command line is: '" ));
    {
        int  nn;
        for (nn = 0; command_line[nn]; nn++)
            XLOG(( ",%s", command_line[nn] ));
    }
    XLOG(( "'\n" ));
#endif

    if ( _posix_pipe( &pipe_read, &pipe_write ) < 0 ) {
        XLOG(( "could not create pipe\n" ));
        result = -1;
        goto Exit;
    }

    XLOG(( "pipe read=%p write=%p\n", pipe_read, pipe_write ));

    child = fork();
    if ( child < 0 ) {
        XLOG(( "could not create child process\n" ));
        goto Exit;
    }

    if ( child == 0 )
    {
        int    nn, max;

        /* redirect standard output to our pipe */
        _posix_dup2( pipe_write, 1 );

       /* Be sure we crash if the parent exits
        * and we write to the pipe
        */
        signal (SIGPIPE, SIG_DFL);

       /* close all handles on exec, except the standard ones
        */
        max = sysconf (_SC_OPEN_MAX);
        for ( nn = 3; nn < max; nn++ )
          _posix_close_on_exec( nn );

        execvp ( shell, (char* const*)command_line );

        XLOG(( "couldn't launch shell '%s'\n", shell ));
        exit(1);  /* when we couldn't launch the program */
    }

    _posix_close( &pipe_write );

    for (;;)
    {
        char   buff[256];
        int    ret;

        ret = read( pipe_read, buff, sizeof(buff) );
        if (ret > 0) {
            if ( rope_add( output, buff, ret ) < 0 )
                break;
        } else if (ret == 0 || errno != EINTR) {
            /* end of file or read error */
            break;
        }
    }

    /* wait for the child */
    {
        int  ret, status;
    Again:
        ret = waitpid( child, &status, 0 );
        if (ret == -1 && errno == EINTR)
            goto Again;
    }

  Exit:
    rope_done( &command_rope );

    if (command_line != NULL)
          free(command_line);

    _posix_close( &pipe_read );
    _posix_close( &pipe_write );

    return  result;
}

#endif /* USE_SPAWNUNIX */

