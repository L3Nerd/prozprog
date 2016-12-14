/**
 * @file grill.c
 * Modul fuer den Grill.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"

/* Lokale Header einbinden */

#include "grill.h"
#include "wurst_helper.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TEMP_INIT 100
#define TEMP_MAX  400
#define TEMP_MIN  0
#define TEMP_INC  20

/* Modulvariablen */

OS_EVENT *GrillSema;
Grill     grill;

/* �ffentliche Funktionen */
/** @publicsection */

/**
 * Sperrt die Grill Semaphore.
 */
void grill_lock (void)
{
  INT8U err;
  OSSemPend(GrillSema, 0, &err);
}

/**
 * Entsperrt die Grill Semaphore.
 */
void grill_unlock (void)
{
  OSSemPost(GrillSema);
}

/**
 * Legt eine Wurst auf den Grill an die letzte Stelle.
 *
 * @param[in] wurst Wurst-Pointer.
 */
void grill_add (wurst_ptr wurst)
{
  grill.wuerste[grill.anzahl] = wurst;
  grill.anzahl++;
}

/**
 * Entfernt eine Wurst vom Grill.
 *
 * @param[in] id Array-ID an dem die Wurst entfernt werden soll.
 */
void grill_remove (INT8U id)
{
  INT8S c;
  for(c = id; c < grill.anzahl - 1; c++)
    grill.wuerste[c] = grill.wuerste[c+1];
  grill.anzahl--;
}

/**
 * Entfernt alle Wuerste vom Grill und gibt den Speicher wieder frei.
 */
void grill_remove_all_free (void)
{
  INT8U     i = 0;
  wurst_ptr wurst;
  INT8U     tmp_anzahl;

  tmp_anzahl = grill.anzahl;
  while(i < tmp_anzahl) {
    wurst = grill.wuerste[i];
    grill_remove(i);
    delete_wurst(wurst);
    i++;
  }
}

/**
 * Erhoet die Temperatur des Grills um TEMP_INC bis maximal TEMP_MAX.
 */
void grill_increase_temperatur (void)
{
  if (grill.temperatur + TEMP_INC <= TEMP_MAX)
    grill.temperatur += TEMP_INC;
}

/**
 * Verringert die Temperatur des Grills um TEMP_INC bis minimal TEMP_MIN.
 */
void grill_decrease_temperatur (void)
{
  if (grill.temperatur - TEMP_INC >= TEMP_MIN)
    grill.temperatur -= TEMP_INC;
}

/**
 * Getter grill.wuerste[id].
 *
 * @param[in] id ID der Wurst im Array.
 * @returns grill.wuerste[id]
 */
wurst_ptr grill_get_wurst (INT8U id)
{
  return grill.wuerste[id];
}

/**
 * Getter grill.anzahl.
 *
 * @returns grill.anzahl
 */
INT8U grill_get_anzahl (void)
{
  return grill.anzahl;
}

/**
 * Getter grill.brennt.
 *
 * @returns grill.brennt
 */
BOOLEAN grill_get_brennt (void)
{
  return grill.brennt;
}

/**
 * Getter grill.temperatur.
 *
 * @returns grill.temperatur
 */
INT16U grill_get_temperatur (void)
{
  return grill.temperatur;
}

/**
 * Setter grill.brennt.
 *
 * @param[in] setzt grill.brennt auf in->brennt.
 */
void grill_set_brennt (BOOLEAN brennt)
{
  grill.brennt = brennt;
}

/**
 * Initialisiert die Grill-Startwerte (und nimmt alle Wuerstchen runter).
 */
void grill_reset (void)
{
  grill_remove_all_free();
  grill.brennt     = OS_FALSE;
  grill.temperatur = TEMP_INIT;
}

/**
 * Initialisiert die Grill Semaphore und die Start-Werte.
 */
void init_grill (void)
{
  /* Grill Semaphore */
  GrillSema = OSSemCreate(1);
  grill.anzahl = 0;
  grill_reset();
}
