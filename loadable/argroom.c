/**
 * \file argroom.c
 * Used to reserve space for arguments.
 *
 **/

/** Size of the reserved space */
#define SIZE 0x4000

/** Name to be used for this symbol */
#define NAME __loader_room_argv

/** Reservation symbol */
volatile char NAME[SIZE];
