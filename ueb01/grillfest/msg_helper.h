/**
 * @file msg_helper.h
 * Modul fuer Nachrichten-Funktionen.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */
#ifndef __MSG_HELPER_H__
#define __MSG_HELPER_H__

/* Nachrichtentypen */
typedef enum {
  START_TRINKEN = 1,
  STOP_TRINKEN  = 2,
  PLACE_WURST   = 3,
  CREATE_WURST  = 4
} MsgTypes;

typedef MsgTypes * msg_ptr;

/**
 * Alloziiert eine Nachricht vom Typ MsgTypes und gibt diese zurueck.
 *
 * @param[in] msg_type Nachrichten-Typ der erstellt werden soll.
 * @returns Nachrichten-Pointer.
 */
msg_ptr create_msg (MsgTypes msg_type);

/**
 * Gibt eine Nachricht im Speicher frei.
 *
 * @param[in] msg Nachrichten-Pointer, der der Partition wieder zugefuehrt werden soll.
 */
void delete_msg(msg_ptr msg);

/**
 * Initialisiert die Nachrichten-Partition und Sempahore.
 */
void init_msg_helper (void);

#endif
