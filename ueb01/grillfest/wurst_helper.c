/**
 * @file wurst_helper.c
 * Modul fuer Wurst-Funktionen.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include "includes.h"

/* Lokale Header einbinden */

#include "wurst_helper.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define MAX_WUERSTE 30

/* Modulvariablen */

/* Wurst Semaphore und Speicher */
OS_EVENT * WurstSema;
OS_MEM   * WurstBuff;
INT8U      WurstPart[MAX_WUERSTE][sizeof(Wurst)];

/* Öffentliche Funktionen */
/** @publicsection */

/**
 * Alloziiert eine Wurst und gibt diese zurueck.
 *
 * @returns Wurst-Pointer.
 */
wurst_ptr create_wurst (void)
{
  INT8U     err;
  wurst_ptr wurst;

  OSSemPend(WurstSema, 0, &err);
  wurst = OSMemGet(WurstBuff, &err);

  /* Wurst Seiten initialisieren. */
  wurst->seiten[0] = 0;
  wurst->seiten[1] = 0;
  wurst->seiten[2] = 0;
  wurst->seiten[3] = 0;

  wurst->seite = 0;

  OSSemPost(WurstSema);

  return wurst;
}

/**
 * Gibt eine Wurst im Speicher frei.
 *
 * @param[in] msg Wurst-Pointer, der der Partition wieder zugefuehrt werden soll.
 */
void delete_wurst(wurst_ptr wurst) {
  INT8U err;
  OSSemPend(WurstSema, 0, &err);
  OSMemPut(WurstBuff, (void *)wurst);
  OSSemPost(WurstSema);
}

/**
 * Initialisiert die Wurst-Partition und Sempahore.
 */
void init_wurst_helper (void)
{
  INT8U err;

  /* Partition und Semaphore fuer Wuerste alloziieren */
  WurstSema = OSSemCreate(1);
  WurstBuff = OSMemCreate(&WurstPart[0][0], MAX_WUERSTE, 4 * sizeof(Wurst), &err);
}

