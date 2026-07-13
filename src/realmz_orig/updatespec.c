#include "prototypes.h"
#include "variables.h"

/******************************* updatespec **************************/
/* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
 * NOTE(iSynic): Update a specific character and keep special ability table accesses in bounds.
 */
void updatespec(short mode, struct character* character) {
  char demodif[12][19] = {
      // clang-format off
      /**  3    4    5    6    7   17   18   19   20   21   22   23   24   25   26   27   28   29   30 **/
          -5,  -4,  -3,  -2,  -1,   1,   1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5,   5,   5, /*** Sneak Attack ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Hide In Shadows ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Ressurrect ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Major Wound ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Detect Secret ****/
         -20, -15, -10,  -5,  -2,   5,   8,  11,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65, /*** Acrobatic Act ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Detect Trap ****/
         -25, -20, -15, -10,  -5,   5,  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70, /*** Disarm Trap ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Hear Noise ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Force Door ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Move Silently ****/
         -25, -20, -15, -10,  -5,   5,  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70, /*** Pick Lock ****/
      // clang-format on
  };

  char range1[19] = {
      // clang-format off
      /** 3  4  5  6  7  17  18  19  20  21  22  23  24  25  26  27  28  29  30 **/
          3, 4, 5, 6, 7, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, /**** Brawn/DX Ranges ***/
      // clang-format on
  };

  char stmodif[12][19] = {
      // clang-format off
      /**  3    4    5    6    7   17   18   19   20   21   22   23   24   25   26   27   28   29   30 **/
          -5,  -4,  -3,  -2,  -1,   1,   1,   1,   2,   2,   2,   3,   3,   3,   4,   4,   4,   4,   4, /*** Sneak Attack ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Hide In Shadows ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Ressurrect ****/
          -5,  -4,  -3,  -2,  -1,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14, /*** Major Wound ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Detect Secret ****/
         -75, -60, -45, -30, -15,   5,  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70, /*** Acrobatic Act ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Detect Trap ****/
         -10,  -8,  -6,  -4,  -2,   2,   4,   6,   8,  10,  12,  14,  16,  18,  20,  22,  24,  26,  28, /*** Disarm Trap ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Hear Noise ****/
         -75, -60, -45, -30, -15,   5,  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70, /*** Force Lock ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*** Move Silently ****/
           0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 /*** Pick Lock ****/
      // clang-format on
  };

  short agility, brawn, direction, t, tt;

  direction = mode == 2 ? -1 : 1;

  loadprofile(character->race, character->caste);

  switch (mode) {
    case 1: /****** initialize ******/

      for (t = 0; t < 14; t++) // v7.1
      {
        if (caste.specialability[0][t])
          character->spec[t] = caste.specialability[0][t] + races.specialability[t];
        else
          character->spec[t] = 0;
      }

      /* fall through */

    case 2: /****** remove attribute modifiers ******/
    case 3: /****** add attribute modifiers ******/

      brawn = pin(character->st + character->magst, 3, 30);
      agility = pin(character->de, 3, 30);

      for (tt = 0; tt < 19; tt++) {
        if (range1[tt] == brawn) {
          for (t = 0; t < 12; t++)
            if (caste.specialability[0][t])
              character->spec[t] += direction * stmodif[t][tt];
        }
      }

      for (tt = 0; tt < 19; tt++) {
        if (range1[tt] == agility) {
          for (t = 0; t < 12; t++)
            if (caste.specialability[0][t])
              character->spec[t] += direction * demodif[t][tt];
        }
      }

      break;

    default:

      for (t = 0; t < 14; t++) // v7.1
      {
        if (caste.specialability[1][t])
          character->spec[t] += Rand(caste.specialability[1][t]);
      }

      break;
  }

  if ((mode != 2) && (mode != 3)) {
    for (t = 0; t < 12; t++)
      character->spec[t] = pin(character->spec[t], 0, 100);

    character->dodge = pin(character->dodge, 0, 100);
    character->missile = pin(character->missile, 0, 100);
    character->magres = pin(character->magres, 0, 100);
    character->twohand = pin(character->twohand, 0, 100);
    character->damage = pin(character->damage, 0, 200);
    character->handtohand = pin(character->handtohand, 0, 200);
  }
}
/* *** END CHANGES *** */

/* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
 * NOTE(iSynic): Initialize and keep derived character stats in sync when attributes change.
 */
static void updateattributestats(struct character* character, short direction) {
  short magicres, saveindex;

  loadprofile(character->race, character->caste);

  strength(character->st + character->magst);
  character->tohit += direction * temp;
  character->damage += direction * damage;

  magicres = (character->in + character->wi) / 10;
  if (character->in > 15)
    magicres += character->in - 15;
  if (character->wi > 15)
    magicres += character->wi - 15;
  character->magres += direction * magicres * caste.magres;

  character->dodge += direction * 2 * character->de;
  if (character->de > 14)
    character->ac += direction * 2 * (character->de - 14);

  if (character->co > 18)
    for (saveindex = 0; saveindex < 8; saveindex++)
      character->save[saveindex] += direction * 5 * (character->co - 18);
}

void updatestatmods(struct character* character, short direction) {
  updateattributestats(character, direction);
  updatespec(direction < 0 ? 2 : 3, character);
}

void initializestatmods(struct character* character) {
  short saveindex;

  updateattributestats(character, 1);
  updatespec(1, character);

  for (saveindex = 0; saveindex < 8; saveindex++)
    character->save[saveindex] = pin(character->save[saveindex], -99, 120);
}
/* *** END CHANGES *** */
