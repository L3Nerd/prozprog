/**
 * @file box.h
 * Modul fuer die Kuehlbox.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */
#ifndef __BOX_H__
#define __BOX_H__

#include "includes.h"
#include "wurst_helper.h"

/**
 * Gibt die Anzahl an Wuersten in der Box zurueck.
 *
 * @returns Die Anzahl.
 */
INT16U box_get_anzahl (void);

/**
 * Fuegt eine Wurst der Kuehlbox (MsgQueue) hinzu und stoppt den "Leere-Box"-Timer.
 *
 * @param[in] wurst Wurst-Pointer.
 */
void box_add (wurst_ptr wurst);

/**
 * Entfernt eine Wurst aus Kuehlbox (MsgQueue) und startet den "Leere-Box"-Timer,
 * falls keine Wurst mehr verblieben ist.
 *
 * @returns Wurst-Pointer, falls keine in der Box ist wurd NULL zurück gegeben.
 */
wurst_ptr box_take (void);

/**
 * Initialisiert die Kuehlbox Queue und den Timer.
 */
void init_box (void);

#endif
