#include "prototypes.h"
#include "variables.h"

static Rect scaled_map_rect(Rect source, Rect maprect) {
  Rect dest;
  short mapsize;

  mapsize = maprect.right - maprect.left;
  dest.top = maprect.top + (source.top * mapsize) / 320;
  dest.left = maprect.left + (source.left * mapsize) / 320;
  dest.bottom = maprect.top + (source.bottom * mapsize) / 320;
  dest.right = maprect.left + (source.right * mapsize) / 320;

  return dest;
}

static Rect centered_map_rect(void) {
  Rect dest;
  short width, height, mapsize;

  width = lookrect.right - lookrect.left;
  height = lookrect.bottom - lookrect.top;
  mapsize = (width < height) ? width : height;

  dest.top = lookrect.top + (height - mapsize) / 2;
  dest.left = lookrect.left + (width - mapsize) / 2;
  dest.bottom = dest.top + mapsize;
  dest.right = dest.left + mapsize;

  return dest;
}

static void move_map_note_dialog(DialogRef dialog) {
  Rect bounds;
  short width, height, lowerwidth, left, top;

  if (!screensize) {
    MoveWindow(GetDialogWindow(dialog), GlobalLeft - 1, GlobalTop + 321, FALSE);
    return;
  }

  GetPortBounds(GetWindowPort(dialog), &bounds);
  width = bounds.right - bounds.left;
  height = bounds.bottom - bounds.top;
  lowerwidth = buttons.left;

  left = 0;
  if (width < lowerwidth)
    left = (lowerwidth - width) / 2;
  top = info.bottom - height - 1;

  MoveWindow(GetDialogWindow(dialog), GlobalLeft + left, GlobalTop + top, FALSE);
}

/********************************* fastplotmap ***************/
void fastplotmap(short id, Rect destrect) {
  FILE* fp = NULL;

  if (id < 0) {
    if (id < -999)
      id += 1000;
    if (id < -999)
      id += 1000;
    if (id < -999)
      id += 1000;

    fastplot(basetile[lastpix], destrect, 0, 0); /***** 0 = look, 1 = buff ******/
    ploticon3(id, destrect);
    return;
  }

  if (id > 999) {
    MyrBitClrShort(&id, 1);
    MyrBitClrShort(&id, 2);

    if (id > 999)
      id -= 1000;
    if (id > 999)
      id -= 1000;
    if (id > 999)
      id -= 1000;
  }

  if (id > 200) {
    fastplot(basetile[lastpix], destrect, 0, 0); /***** 0 = look, 1 = buff ******/
    return;
  }

  tempid = (id - 1) / 20;

  temp = id - (tempid * 20) - 1;

  itemRect.top = 32 * tempid;
  itemRect.left = 32 * temp;

  itemRect.right = itemRect.left + 32;
  itemRect.bottom = itemRect.top + 32;

  {
    BitMap* src = GetPortBitMapForCopyBits(gthePixels);
    BitMap* dst = GetPortBitMapForCopyBits(GetWindowPort(look));
    CopyBits(src, dst, &itemRect, &destrect, 0, NIL);
  }
}

/***************** showmap *******************/
void showmap(short mapnumber) {
  FILE* fp = NULL;
  DialogRef show;
  Boolean tag = FALSE;
  PicHandle picture = NIL;
  Rect temprect;
  Rect maprect;
  Boolean scalegeneratedmap = FALSE;
  short oldview, t, tt, temp, tempisdung;

  tempisdung = indung;
  oldview = viewtype;

  in();
  getfilename("Data MD2");
  if ((fp = MyrFopen(filename, "rb")) == NULL)
    scratch(115);
  fseek(fp, mapnumber * sizeof themap, SEEK_SET);
  fread(&themap, sizeof themap, 1, fp);
  CvtMapsToPc(&themap);
  fclose(fp);

  SelectWindow(look);

  viewtype = -1;

  if (themap.show < 0) {
    movie(themap.show, 129, 0);
    goto out;
  } else if (themap.pictid) {
    SetPort(GetWindowPort(look));
    picture = GetPicture(themap.pictid);
    if (picture) {
      /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
       * Center picture-backed maps at their native size unless the map defines a draw rect.
       */
      itemRect = (**picture).picFrame;
      OffsetRect(&itemRect, -itemRect.left, -itemRect.top);

      ForeColor(blackColor);
      PaintRect(&lookrect);

      if ((themap.rect[2]) || (themap.rect[3])) {
        itemRect.top = themap.rect[0];
        itemRect.left = themap.rect[1];
        itemRect.bottom = themap.rect[2];
        itemRect.right = themap.rect[3];
      } else {
        OffsetRect(&itemRect, lookrect.left + ((lookrect.right - lookrect.left) - itemRect.right) / 2,
            lookrect.top + ((lookrect.bottom - lookrect.top) - itemRect.bottom) / 2);
      }

      DrawPicture(picture, &itemRect);
    }
  } else {
    int enable_recomposite = WindowManager_SetEnableRecomposite(0);

    /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
     * Scale generated land maps to fit the enlarged game viewport without changing map coverage.
     */
    maprect = centered_map_rect();
    scalegeneratedmap = !themap.isdungeon;

    temp = 320 / themap.iconsize;
    if (temp * themap.iconsize < 320)
      temp++;

    if (!indung)
      saveland(landlevel);
    else
      saveland(dunglevel);

    templong = themap.level;
    indung = themap.isdungeon;
    loadland(templong, TRUE);

    xy(1);

    SetPort(GetWindowPort(look));

    ForeColor(blackColor);
    BackColor(whiteColor);

    temprect.top = temprect.left = 0;
    temprect.right = temprect.bottom = themap.iconsize;

    if (!indung) {
      PaintRect(&lookrect);
      for (t = themap.starty; t < temp + themap.starty; t++) {
        for (tt = themap.startx; tt < themap.startx + temp; tt++) {
          fastplotmap(field[tt][t], scaled_map_rect(temprect, maprect));
          OffsetRect(&temprect, themap.iconsize, 0);
        }
        OffsetRect(&temprect, -(themap.iconsize * temp), themap.iconsize);
      }
    } else {
      SetPort(GetWindowPort(look));
      ForeColor(blackColor);
      BackColor(whiteColor);
      editon = TRUE;
      centerpict();
      xy(1);
      editon = FALSE;
      SetPort(GetWindowPort(look));
      tag = TRUE;
    }

    for (t = 0; t < 10; t++) {
      if (themap.icon[t][0]) {
        if (!themap.iconsize)
          themap.iconsize = 32;
        temp = themap.icon[t][2];
        temprect.top = temp * themap.iconsize;
        temprect.bottom = temprect.top + themap.iconsize;

        temp = themap.icon[t][1];
        temprect.left = temp * themap.iconsize;
        temprect.right = temprect.left + themap.iconsize;

        iconhand = NIL;
        iconhand = GetCIcon(themap.icon[t][0]);

        /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
         * NOTE(jpetrie): This if statement originally read:
         *
         *   if ((themap.icon[t][0] == 137) || (139))
         *
         * The comment implies that this is supposed to be a check for the X-marks-the-spot or S-for-secret icons to
         * ensure they're displayed full-size. However, because the right-hand side of the || is non-zero, the
         * expression was always true. Properly testing against the icon ID restores the intended behavior.
         */
        if ((themap.icon[t][0] == 137) || (themap.icon[t][0] == 139)) /*** let x and s be full size ***/
        {
          InsetRect(&temprect, -((32 - themap.iconsize) / 2), -((32 - themap.iconsize) / 2));
        }

        if (scalegeneratedmap)
          temprect = scaled_map_rect(temprect, maprect);

        if (iconhand) {
          PlotCIcon(&temprect, iconhand);
          DisposeCIcon(iconhand);
        }
      }
    }

    WindowManager_SetEnableRecomposite(enable_recomposite);
  }

  point.h = partyx + lookx;
  point.v = partyy + looky;

  itemRect.left = themap.startx - 1;
  itemRect.top = themap.starty - 1;
  itemRect.right = itemRect.left + 320 / themap.iconsize + 1;
  itemRect.bottom = itemRect.top + 320 / themap.iconsize + 1;

  if (themap.isdungeon == tempisdung) {
    if (((themap.isdungeon) && (themap.level == dunglevel)) || ((!themap.isdungeon) && (themap.level == landlevel))) {
      if (PtInRect(point, &itemRect)) /***** show you are here ****/
      {
        icon.left = (point.h - themap.startx) * themap.iconsize - 20;
        icon.top = (point.v - themap.starty) * themap.iconsize - 24;
        icon.right = icon.left + 64;
        icon.bottom = icon.top + 64;

        if (scalegeneratedmap)
          icon = scaled_map_rect(icon, maprect);

        ploticon3(138, icon);
      }
    }
  }

  /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
   * Use the large-screen map note dialog over the enlarged message panel and restore its border.
   */
  show = GetNewDialog(169 + (1000 * screensize), 0L, (WindowPtr)-1L);
  SetPortDialogPort(show);
  BackPixPat(base);
  TextFont(defaultfont);
  move_map_note_dialog(show);
  ForeColor(yellowColor);
  gCurrent = show;
  ShowWindow(GetDialogWindow(show));
  ErasePortRect();
  TextMode(1);

  DrawDialog(show);
  MyrPascalDiStr(2, themap.note);
  GetDialogItem(show, 2, &itemType, &itemHandle, &itemRect);
  InsetRect(&itemRect, -2, -2);
  ForeColor(yellowColor);
  FrameRect(&itemRect);

  indung = tempisdung;

  if (!indung)
    loadland(landlevel, TRUE);
  else
    loadland(dunglevel, TRUE);

  flashmessage((StringPtr) "Click Mouse", 350 + leftshift, 100, 0, 30005);

  xy(0);
  DisposeDialog(show);

out:

  SetPort(GetWindowPort(look));
  ForeColor(blackColor);
  BackColor(whiteColor);
  viewtype = oldview;
  if (indung) {
    if (viewtype == 1)
      UpdateWindow(FALSE);
  }
  updatemain(FALSE, -1);
}
