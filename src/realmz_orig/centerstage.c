#include "prototypes.h"
#include "variables.h"

/************************** centerstage **********************/
void centerstage(short way) {
  short who;
  // NOTE(fuzziqersoftware): It's possible for the do...while loop below to never terminate, if way is 0 (which happes
  // often) and q[aimindex] == -1. I've added this variable so the loop will always eventually terminate.
  short count = 0;

  // if ((who < 0) || (who > maxloop)) return; Myriad @@@ ????????????????????

  do {
    aimindex += way;
    if (aimindex > maxloopminus)
      aimindex = 1;
    if (aimindex < 1)
      aimindex = maxloopminus;
    if (++count > maxloopminus)
      return; // all slots empty (or way==0 with invalid aimindex)
  } while (q[aimindex] == -1);

  who = q[aimindex];

  if (who < 9) {
    if (c[who].stamina > 0)
      centerfield(pos[who][0] - (spellx - 5) * inspell, pos[who][1] - (spelly - 5) * inspell);
  } else if (monster[who - 10].stamina > 0)
    centerfield(monpos[who - 10][0] - (spellx - 5) * inspell, monpos[who - 10][1] - (spelly - 5) * inspell);

  if (way)
    sound(147);

  drawbody(who, 1, 0);
  infocombat = TRUE;
  combatupdate2(who);
  infocombat = FALSE;
}
