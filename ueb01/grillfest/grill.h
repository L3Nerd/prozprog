/**
 * @file grill.h
 * Modul fuer den Grill.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */
#ifndef __GRILL_H__
#define __GRILL_H__

#include "includes.h"
#include "wurst_helper.h"

#define MAX_GRILL 30

typedef struct {
  wurst_ptr wuerste[MAX_GRILL];
  INT8U     anzahl;
  BOOLEAN   brennt;
  INT16U    temperatur;
} Grill;

/**
 * Sperrt die Grill Semaphore.
 */
void grill_lock (void);

/**
 * Entsperrt die Grill Semaphore.
 */
void grill_unlock (void);

/**
 * Legt eine Wurst auf den Grill an die letzte Stelle.
 *
 * @param[in] wurst Wurst-Pointer.
 */
void grill_add (wurst_ptr wurst);

/**
 * Entfernt eine Wurst vom Grill.
 *
 * @param[in] id Array-ID an dem die Wurst entfernt werden soll.
 */
void grill_remove (INT8U id);

/**
 * Entfernt alle Wuerste vom Grill und gibt den Speicher wieder frei.
 */
void grill_remove_all_free (void);

/**
 * Erhoet die Temperatur des Grills um TEMP_INC bis maximal TEMP_MAX.
 */
void grill_increase_temperatur (void);

/**
 * Verringert die Temperatur des Grills um TEMP_INC bis minimal TEMP_MIN.
 */
void grill_decrease_temperatur (void);

/**
 * Getter grill.wuerste[id].
 *
 * @param[in] id ID der Wurst im Array.
 * @returns grill.wuerste[id]
 */
wurst_ptr grill_get_wurst (INT8U id);

/**
 * Getter grill.anzahl.
 *
 * @returns grill.anzahl
 */
INT8U grill_get_anzahl (void);

/**
 * Getter grill.brennt.
 *
 * @returns grill.brennt
 */
BOOLEAN grill_get_brennt (void);

/**
 * Getter grill.temperatur.
 *
 * @returns grill.temperatur
 */
INT16U grill_get_temperatur (void);

/**
 * Setter grill.brennt.
 *
 * @param[in] setzt grill.brennt auf in->brennt.
 */
void grill_set_brennt (BOOLEAN brennt);

/**
 * Initialisiert die Grill-Startwerte.
 */
void grill_reset (void);

/**
 * Initialisiert die Grill Semaphore und die Start-Werte.
 */
void init_grill (void);

#endif
