#include "prototypes.h"
#include "variables.h"
#include "UiViewport.h"

/************************ centerfield ************************/
void centerfield(short x, short y) {
  register t, tt;
  short tempicon, ten, single;
  char bq[maxloop];
  UiViewportGeometry viewport = ui_viewport_geometry(screensize);
  int next_fieldx, next_fieldy, local, delta_x, delta_y;

  if (!incombat) {
    centerpict();
    return;
  }

  // SelectWindow(look);
  SetPort((GrafPtr)GetWindowPort(look));

  lookrect.left = 0;

  for (t = 0; t < maxloop; t++)
    bq[t] = 0;

  /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
   * Use the selected viewport's center and field limits. The same camera
   * delta must be applied to characters and monsters below.
   */
  ui_viewport_recenter(fieldx + x, viewport.combat_center_x, viewport.columns, &next_fieldx, &local);
  ui_viewport_recenter(fieldy + y, viewport.combat_center_y, viewport.rows, &next_fieldy, &local);
  delta_x = next_fieldx - fieldx;
  delta_y = next_fieldy - fieldy;
  fieldx = next_fieldx;
  fieldy = next_fieldy;

  for (t = 0; t <= charnum; t++) {
    pos[t][0] -= delta_x;
    pos[t][1] -= delta_y;
  }

  for (t = 0; t < maxmon; t++) {
    monpos[t][0] -= delta_x;
    monpos[t][1] -= delta_y;
  }

  SetPort((GrafPtr)GetWindowPort(look));

  icon.top = -32;
  icon.bottom = 0;

  ForeColor(blackColor);
  BackColor(whiteColor);

  /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
   * Retain the extra row and column for large sprites, bounded by the field.
   */
  for (tt = fieldy; (tt < fieldy + viewport.rows + 1) && (tt < 90); tt++) // Myriad (+11)
  {
    icon.top += 32;
    icon.bottom += 32;
    icon.left = -32;
    icon.right = 0;
    for (t = fieldx; (t < fieldx + viewport.columns + 1) && (t < 90); t++) // Myriad (+11)
    {
      icon.left += 32;
      icon.right += 32;
      tempicon = field[t][tt];
      point.h = t;
      point.v = tt;

      if (tempicon > 999) {
        BitMap* src = GetPortBitMapForCopyBits(gthePixels);
        BitMap* dst = GetPortBitMapForCopyBits(gbuff);
        tempicon -= 1000;
        ten = (tempicon - 1) / 20;
        single = tempicon - (ten * 20) - 1;
        itemRect.top = 32 * ten;
        itemRect.left = 32 * single;
        itemRect.right = itemRect.left + 32;
        itemRect.bottom = itemRect.top + 32;
        CopyBits(src, dst, &itemRect, &icon, 0, NIL);

      } else if (tempicon > -1) {
        /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
         * Keep the body id in range for bq (char[maxloop]) before indexing. */
        if (tempicon < maxloop) {
          bodyground(tempicon, 1);
          bq[tempicon] = TRUE;
        }
        /* *** END CHANGES *** */
      }
    }
  }
  showque();
  itemRect.top = itemRect.left = -32;
  itemRect.right = itemRect.bottom = 0;
  for (t = 0; t < maxloop; t++)
    if (bq[t])
      drawbody(t, inspell, 1);

  if (inspell)
    showtargets(1);

  if ((q[up] < 9) && (usehashmarks)) {
    BitMap* buf = GetPortBitMapForCopyBits(gbuff);
    BitMap* edgepix = GetPortBitMapForCopyBits(gedge);
    BitMap* lookpix = GetPortBitMapForCopyBits(GetWindowPort(look));

    CopyBits(buf, edgepix, &lookrect, &lookrect, 0, NIL);
    showrange(TRUE);
    InsetRect(&lookrect, 10, 10);
    CopyBits(buf, edgepix, &lookrect, &lookrect, 0, NIL);
    InsetRect(&lookrect, -10, -10);
    CopyBits(edgepix, lookpix, &lookrect, &lookrect, 0, NIL);
  } else {
    BitMap* buf = GetPortBitMapForCopyBits(gbuff);
    BitMap* lookpix = GetPortBitMapForCopyBits(GetWindowPort(look));
    CopyBits(buf, lookpix, &lookrect, &lookrect, 0, NIL);
  }

  SetPort((GrafPtr)GetWindowPort(screen));
}
