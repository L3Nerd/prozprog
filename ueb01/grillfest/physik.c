/**
 * @file physik.c
 * Modul fuer den Physik-Task.
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

#define TASK_STACKSIZE  2048
#define PHYSIK_PRIORITY 13

/* Modulvariablen */

OS_STK stk_physik[TASK_STACKSIZE];

/* Forwarddeklarationen */

void task_physik (void* pdata);

/* �ffentliche Funktionen */
/** @publicsection */

/**
 *  Initialisiert den Physik-Task.
 */
void init_physik (void)
{
  OSTaskCreateExt(task_physik,
                  NULL,
                  (void *)&stk_physik[TASK_STACKSIZE-1],
                  PHYSIK_PRIORITY,
                  PHYSIK_PRIORITY,
                  stk_physik,
                  TASK_STACKSIZE,
                  NULL,
                  0);
}

/* Private Funktionen */
/** @privatesection */

/**
 * Task: Physik
 *
 * Die Physik sorgt dafuer, dass alle W�rste von der Unterseite gleichmaessig gebr�unt
 * werden. Die Geschwindigkeit der Braeunung haengt dabei von der durch den Benutzer
 * eingestellten Temperatur ab. Au�erdem wird der Grill entzuendet, sobald eine Wurst
 * den Braeunungsgrad von 100% ueberschreitet.
 *
 * Ressourcen: Grill -> un/lock
 *
 */
void task_physik(void* pdata)
{
  wurst_ptr wurst;
  INT8U     i;

  while (1)
  {
    OSTimeDlyHMSM(0, 0, 1, 0);

    i = 0;
    grill_lock();
    while(i < grill_get_anzahl() && !grill_get_brennt()) {
      wurst = grill_get_wurst(i);
      wurst->seiten[wurst->seite] += grill_get_temperatur()/100;
      if (wurst->seiten[wurst->seite] > 100) {
        grill_set_brennt(OS_TRUE);
      }
      i++;
    }
    grill_unlock();
  }
}
