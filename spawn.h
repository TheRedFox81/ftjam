/*
 * Copyright 2007 David Turner.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * spawn.h - shell command invokation
 */

typedef enum
{
  SPAWN_MODE_LINES,
  SPAWN_MODE_WORDS

} SpawnMode;

extern int  spawn( LIST*  command,
                   Rope*  output );
