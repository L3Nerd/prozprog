/**
 * @file grillmeister.h
 * Modul fuer den Grillmeister-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */
#ifndef __GRILLMEISTER_H__
#define __GRILLMEISTER_H__

#include "msg_helper.h"

/**
 * Legt eine Nachricht vom Typ MsgTypes in die MsgBox ab.
 *
 * @param[in] msg_type Die Nachricht, die abgelegt werden soll.
 */
void send_grillmeister_msg (MsgTypes msg_type);

/**
 *  Initialisiert den Grillmeister Task und MsgBox.
 */
void init_grillmeister (void);

#endif
