#include "prototypes.h"
#include "variables.h"

/******************** modify **********************/
void modify(short mode) {
  FILE* fp = NULL;
  char name[256];

  strcpy(name, c[charselectnew].name);
  strcpy(filename, ":Character Files:");
  strcat((StringPtr)filename, name);

  if ((fp = MyrFopen(filename, "rb")) == NULL)
    scratch(119);
  fread(&characterl, sizeof characterl, 1, fp);
  CvtCharacterToPc(&characterl);
  fclose(fp);

  in();

  switch (mode) {
    case 1:
      characterl = c[charselectnew];
      portrait(0);
      updatemain(FALSE, 0);
      in();
      break;

    case 2:
      characterl = c[charselectnew];
      iconpicture(0);
      updatemain(FALSE, 0);
      in();
      break;
  }
  if ((fp = MyrFopen(filename, "w+b")) == NULL)
    scratch(120);
  CvtCharacterToPc(&characterl);
  fwrite(&characterl, sizeof characterl, 1, fp);
  CvtCharacterToPc(&characterl);
  fclose(fp);
  warn(20);
  updatemain(TRUE, -1);
}

/********************************* cancast ***************************/
short cancast(short who, short silentmode) {
  short reply = TRUE;

  if ((who < 0) || (who > charnum))
    return (FALSE);

  if ((spellcasting != 0) && (silentmode == 0)) {
    warn(113);
    return (FALSE);
  }

  if (who < 6) {
    if (c[who].condition[COND_CONFUSED] || c[who].condition[COND_SILENCED] || c[who].condition[COND_HELPLESS] || c[who].condition[COND_STUPID] || c[who].condition[COND_ANIMATED] || c[who].spellpoints < 1 || c[who].stamina < 1)
      reply = 0;

    if (incombat) {
      if (c[who].beenattacked)
        reply = 0;
      if (c[who].spellsofar >= c[who].maxspellsattacks)
        reply = 0;
    }
  }
  return (reply);
}
