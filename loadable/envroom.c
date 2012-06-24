/**
 * \file envroom.c
 * Used to reserve space for environment.
 *
 **/

/** Size of the reserved space */
#define SIZE 0x4000

/** Name to be used for this symbol */
#define NAME __loader_room_env

/** Reservation symbol */
volatile char NAME[SIZE];
