/**
 * @file fleischer.h
 * Modul fuer den Fleischer-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */
#ifndef __FLEISCHER_H__
#define __FLEISCHER_H__

#include "msg_helper.h"

/**
 * Legt eine Nachricht vom Typ MsgTypes in die MsgBox ab.
 *
 * @param[in] msg_type Die Nachricht, die abgelegt werden soll.
 */
void send_fleischer_msg (MsgTypes msg_type);

/**
 *  Initialisiert den Fleischer Task und MsgBox.
 */
void init_fleischer (void);

#endif
