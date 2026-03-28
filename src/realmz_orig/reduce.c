#include "prototypes.h"
#include "variables.h"

/*************************** reduce conditions *******************/
void reduce(void) {
  short t, tt;
  Boolean verge, tag = FALSE;
  Boolean wizard = FALSE;
  Rect flamerect;

  if (partycondition[PARTY_COND_TORCH_LIT] == 1)
    tag = TRUE;
  if (partycondition[PARTY_COND_WIZARD_EYE])
    wizard = TRUE;

  // *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
  // It appears they forgot to add leftshift/downshift here when switching to 800x600. The result of this omission was
  // that party condition icons wouldn't be erased properly when they expired.
  SetRect(&flamerect, 367 + leftshift, 330 + downshift, 399 + leftshift, 362 + downshift); /**** L T R B ****/

  for (t = 1; t < 10; t++) {
    if ((partycondition[t] == 1) && (t != 5)) { // 5 = Search, which should not be erased because it's a button
      SetPort(GetWindowPort(screen));
      EraseRect(&flamerect);
    }
    if (t == 5)
      OffsetRect(&flamerect, -156, 40);
    OffsetRect(&flamerect, 39, 0);
  }

  for (t = 0; t < 10; t++)
    if (partycondition[t] > 0)
      partycondition[t]--;

  if (partycondition[PARTY_COND_TORCH_LIT] > 0)
    partycondition[PARTY_COND_TORCH_LIT]--;

  if (!incombat) {
    if ((!partycondition[PARTY_COND_WIZARD_EYE]) && (wizard)) {
      if ((!multiview) && (indung)) {
        warn(95);
        viewtype = 1;
        UpdateWindow(FALSE);
      }
    }

    if (partycondition[PARTY_COND_TORCH_LIT] / 30 != (partycondition[PARTY_COND_TORCH_LIT] + 1) / 30) {
      loaddark(partycondition[PARTY_COND_TORCH_LIT] / 30 + 1);
      centerpict();
    } else if (tag) {
      loaddark(0);
      centerpict();
      updatetorch();
    }
  }

  for (t = 0; t <= charnum; t++) {
    if ((c[t].condition[COND_REGENERATING]) && (c[t].stamina > -10) && (c[t].stamina < c[t].staminamax) && (!c[t].condition[COND_ANIMATED])) /***** Regenerate ****/
    {
      if (((incombat) && (c[t].stamina > 0)) || (!incombat)) {
        damage = abs(c[t].condition[COND_REGENERATING]);
        heal(t, damage, FALSE);
        regenerate = TRUE;
        if ((damage) && (incombat) && (c[t].inbattle))
          showresults(t, 0, 0); /********* regenerate for ***************/
        else
          updatecharshort(t, FALSE);
        regenerate = FALSE;
      }
    }

    if ((c[t].condition[COND_DISEASED]) && (c[t].stamina > 0) && (c[t].condition[COND_ANIMATED] > -1)) /***** disease ****/
    {
      damage = abs(c[t].condition[COND_DISEASED]);
      c[t].stamina -= damage;
      spellinfo.spelllook2 = 7;
      if (c[t].stamina < 1) {
        killbody(t, 0);
        flashmessage((StringPtr) "Disease takes its toll!", 30, 100, 100, 10122);
      } else if ((incombat) && (c[t].inbattle))
        showresults(t, 29, 0); /********* diseased ***************/
      else
        updatecharshort(t, FALSE);
    }

    if ((c[t].condition[COND_POISONED]) && (c[t].stamina > 0) && (c[t].condition[COND_ANIMATED] > -1)) /***** poison ****/
    {
      spellinfo.spelllook2 = 7;
      damage = abs(c[t].condition[COND_POISONED]);
      c[t].stamina -= damage;
      poisoned = TRUE;
      if (c[t].stamina < 1) {
        killbody(t, 0);
        flashmessage((StringPtr) "Poison takes its toll!", 30, 100, 100, 684);
      } else if ((incombat) && (c[t].inbattle))
        showresults(t, 0, 0); /********* poisoned for ***************/
      else
        updatecharshort(t, FALSE);
      poisoned = FALSE;
    }

    if ((c[t].condition[COND_ENERGY_DRAIN]) && (c[t].stamina > 0)) /****** power suck *******/
    {
      c[t].spellpoints -= abs(c[t].condition[COND_ENERGY_DRAIN]);
      if (c[t].spellpoints < 0)
        c[t].spellpoints = 0;
      updatecharshort(t, FALSE);
    }

    if ((c[t].condition[COND_ABSORBING_ENERGY]) && (c[t].spellpointsmax) && (c[t].stamina > 0)) /******* power absorbe ********/
    {
      c[t].spellpoints += abs(c[t].condition[COND_ABSORBING_ENERGY]);
      if (c[t].spellpoints > c[t].spellpointsmax)
        c[t].spellpoints = c[t].spellpointsmax;
      updatecharshort(t, FALSE);
    }

    if (c[t].condition[COND_CONFUSED]) {
      if (c[t].traiter) {
        c[t].traiter = 0;
        killparty--;
        numenemy--;
      }
    }

    verge = FALSE;
    for (tt = 0; tt < 40; tt++) /**** Party ****/
    {

      if (c[t].condition[tt] == 1)
        verge = TRUE;
      if (c[t].condition[tt] > 0)
        c[t].condition[tt]--;
    }
    if (verge)
      updatechar(t, 3);
  }

  if (!incombat) {
    for (t = 0; t < heldover; t++) {
      for (tt = 0; tt < 20; tt++) /**** Allies ****/
      {
        if (holdover[t].condition[tt] > 0)
          holdover[t].condition[tt]--;
      }
    }
  }
}
