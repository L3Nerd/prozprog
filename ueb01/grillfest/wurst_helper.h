/**
 * @file wurst_helper.h
 * Modul fuer Wurst-Funktionen.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */
#ifndef __WURST_HELPER_H__
#define __WURST_HELPER_H__

#include "includes.h"

/* Wurst Datenstruktur */
typedef struct {
  float seiten[4];
  INT8U seite;
} Wurst;

typedef Wurst *wurst_ptr;


/**
 * Alloziiert eine Wurst und gibt diese zurueck.
 *
 * @returns Wurst-Pointer.
 */
wurst_ptr create_wurst (void);

/**
 * Gibt eine Wurst im Speicher frei.
 *
 * @param[in] msg Wurst-Pointer, der der Partition wieder zugefuehrt werden soll.
 */
void delete_wurst (wurst_ptr wurst);

/**
 * Initialisiert die Wurst-Partition und Sempahore.
 */
void init_wurst_helper (void);

#endif
