/**
 * @file feuerwehr.c
 * Modul fuer den Feuerwehr-Task.
 *
 * @author inf1073   Lennart Kindermann
 * @author inf100849 Oliver Westphal
 */

/* System-Header einbinden */

#include <stdlib.h>
#include "includes.h"

/* Lokale Header einbinden */

#include "grill.h"

/* lokale Konstanten und Makros */
/** @privatesection */

#define TASK_STACKSIZE     2048
#define FEUERWEHR_PRIORITY 19

/* Modulvariablen */

OS_STK stk_feuerwehr[TASK_STACKSIZE];

/* Forwarddeklarationen */

void task_feuerwehr (void* pdata);

/* Öffentliche Funktionen */
/** @publicsection */

/**
 *  Initialisiert den Feuerwehr-Task.
 */
void init_feuerwehr (void)
{
  OSTaskCreateExt(task_feuerwehr,
                  NULL,
                  (void *)&stk_feuerwehr[TASK_STACKSIZE-1],
                  FEUERWEHR_PRIORITY,
                  FEUERWEHR_PRIORITY,
                  stk_feuerwehr,
                  TASK_STACKSIZE,
                  NULL,
                  0);
}

/* Private Funktionen */
/** @privatesection */

/**
 * Task: Feuerwehr
 *
 * Die Feuerwehr prueft den Grill alle 60 Sekunden auf eine moegliche Entzuendung. Wird
 * eine solche erkannt, loescht sie den Grill unverzueglich und verzehrt alle auf dem Grill
 * befindlichen Wuerste.
 *
 * Ressourcen: Grill -> un/lock
 *
 */
void task_feuerwehr(void* pdata)
{
  while (1)
  {
    OSTimeDlyHMSM(0, 1, 0, 0);

    grill_lock();
    if (grill_get_brennt()) {
      grill_reset();
    }
    grill_unlock();
  }
}
