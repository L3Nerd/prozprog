/**
 * @file msg_helper.c
 * Modul fuer Nachrichten-Funktionen.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"

/* Lokale Header einbinden */

#include "msg_helper.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define MAX_MSG 10

/* Modulvariablen */

/* Message Semaphore und Speicher */
OS_EVENT * MsgSema;
OS_MEM   * MsgBuff;
INT8U      MsgPart[MAX_MSG][sizeof(MsgTypes)];

/* Forwarddeklarationen */

/* Öffentliche Funktionen */
/** @publicsection */

/**
 * Alloziiert eine Nachricht vom Typ MsgTypes und gibt diese zurueck.
 *
 * @param[in] msg_type Nachrichten-Typ der erstellt werden soll.
 * @returns Nachrichten-Pointer.
 */
msg_ptr create_msg (MsgTypes msg_type) {
  INT8U  err;
  msg_ptr msg;

  OSSemPend(MsgSema, 0, &err);
  msg  = OSMemGet(MsgBuff, &err);
  *msg = msg_type;
  OSSemPost(MsgSema);

  return msg;
}

/**
 * Gibt eine Nachricht im Speicher frei.
 *
 * @param[in] msg Nachrichten-Pointer, der der Partition wieder zugefuehrt werden soll.
 */
void delete_msg (msg_ptr msg) {
  INT8U err;
  if (msg != (void*)0) {
    OSSemPend(MsgSema, 0, &err);
    OSMemPut(MsgBuff, msg);
    OSSemPost(MsgSema);
  }
}

/**
 * Initialisiert die Nachrichten-Partition und Sempahore.
 */
void init_msg_helper (void)
{
  INT8U err;

  /* Partition und Semaphore fuer Messages alloziieren */
  MsgSema = OSSemCreate(1);
  MsgBuff = OSMemCreate(&MsgPart[0][0], MAX_MSG, sizeof(MsgTypes), &err);
}
