#include "jam.h"
#include "lists.h"
#include "rope.h"
#include "spawn.h"

#ifdef USE_SPAWNMAC

int  spawn( LIST*      command,
            Rope*      output )
{
    /* this is completely unimplemented on this platform */
    /* so always return an empty string                  */

    rope_init( output );

    return 0;
}

#endif
