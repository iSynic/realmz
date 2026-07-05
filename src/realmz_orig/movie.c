#include "prototypes.h"
#include "variables.h"

static void moviewithgeometry(short textID, short dataid, short justify, short classicgeometry);

/************************ movie ************************/
void movie(short textID, short dataid, short justify) // � 'About...' dialog box.
{
  moviewithgeometry(textID, dataid, justify, FALSE);
}

void aboutmovie(short textID, short dataid, short justify)
{
  moviewithgeometry(textID, dataid, justify, TRUE);
}

static void moviewithgeometry(short textID, short dataid, short justify, short classicgeometry)
{
  int32_t mouseuptime = 0;
  int32_t newcount;
  TEHandle textHand;
  Rect movieRect;
  Rect txtRect;
  WindowRef about; // � Pointer to dialog.
  short movieTop;
  short movieLeft;
  short x;
  Handle handle;
  StScrpHandle styleHdl;
  GWorldPtr backdrop;
  PixMapHandle backdroppix;
  PixPatHandle backpat;

  GetGWorld(&savedPort, &savedDevice); // � Save the old port info.

  /* *** CHANGED FROM ORIGINAL IMPLEMENTATION ***
   * Restore Classic scrolling text geometry for Info menu movies.
   */
  if (classicgeometry) {
    movieRect.top = txtRect.top = 0;
    movieRect.left = txtRect.left = 0;
    movieRect.bottom = txtRect.bottom = 320;
    movieRect.right = txtRect.right = 320;
    movieTop = (depth - 320) / 2;
    movieLeft = (width - 320) / 2;
  } else {
    movieRect = lookrect;
    txtRect = lookrect;
    movieTop = GlobalTop;
    movieLeft = GlobalLeft;
  }

  // � Setup scrolling window for text.
  about = GetNewWindow(133, nil, (WindowPtr)-1);
  SetPort(GetWindowPort(about));
  if (!classicgeometry)
    SizeWindow(about, movieRect.right - movieRect.left, movieRect.bottom - movieRect.top, 0);
  ForeColor(blackColor);
  BackColor(whiteColor);
  backpat = GetPixPat(dataid);
  BackPixPat(backpat);
  TextSize(10);
  TextFont(genevafont);
  TextMode(1);
  MoveWindow(about, movieLeft, movieTop, TRUE);
  ShowWindow(about);
  EraseRect(&movieRect);

  GetGWorld(&savedPort, &savedDevice); /**** create offworld text area *****/
  NewGWorld(&backdrop, 8, &movieRect, 0L, 0L, 0L);
  SetGWorld(backdrop, NIL);
#ifndef PC // Myriad
  backdroppix = GetPortPixMap(backdrop);
  LockPixels(backdroppix);
#endif
  BackPixPat(backpat);
  ForeColor(blackColor);
  BackColor(whiteColor);
  TextMode(1);

  EraseRect(&movieRect);

  // � Create styled TERecord.
  textHand = TEStyleNew(&txtRect, &txtRect);

  // � Read the TEXT resource.
  handle = GetResource('TEXT', textID);
  HLock(handle); // � Lock handle.

  // � Get the style handle.
  styleHdl = (StScrpHandle)(GetResource('styl', textID));
#ifdef PC
  AcamCvtStScrpHandle(styleHdl);
#endif

  TEStyleInsert(*handle, GetResourceSizeOnDisk(handle), styleHdl, textHand); // � move text into text record.

  TESetAlignment(justify, textHand);
  HUnlock(handle); // � Unlock handle.
  ReleaseResource((Handle)handle); // Myriad : more clean...
  ReleaseResource((Handle)styleHdl); // Myriad : more clean...

  TEUpdate(&txtRect, textHand); // � Draw text in viewRect.
  InsetRect(&txtRect, -5, -1); // � Leave margins for text.

  ShowWindow(about); // � Show dialog box now.

backtoit:

  if (!Button()) {
    BitMap* src = GetPortBitMapForCopyBits(backdrop);
    BitMap* dst = GetPortBitMapForCopyBits(GetWindowPort(about));

    newcount = TickCount();
    // � scroll up a pixel
    TEScroll(0, -1, textHand);
    EraseRect(&movieRect);
    TEUpdate(&txtRect, textHand); // � Draw text in viewRect.
    CopyBits(src, dst, &movieRect, &movieRect, 0, NIL);
    do {

    } while (TickCount() < newcount + 3);

    SystemTask();
    x = GetNextEvent(everyEvent, &gTheEvent);
    MyrCheckMemory(2);
#ifdef PC // Myriad
    DoCorrectBugMADRepeat();
#endif

    switch (gTheEvent.what) {
      case mouseDown:
        if (gTheEvent.when <= mouseuptime)
          goto out;
        break;

      case mouseUp:
        mouseuptime = gTheEvent.when + GetDblTime();
        break;
    }

    while (StillDown()) {
      BitMap* src = GetPortBitMapForCopyBits(backdrop);
      BitMap* dst = GetPortBitMapForCopyBits(GetWindowPort(about));
      GetMouse(&point);
      if (point.v < gTheEvent.where.v)
        TEScroll(0, -25, textHand);
      else if (point.v > gTheEvent.where.v)
        TEScroll(0, 25, textHand);

      gTheEvent.where.v = point.v;
      EraseRect(&movieRect);
      TEUpdate(&txtRect, textHand); // � Draw text in viewRect.
      CopyBits(src, dst, &movieRect, &movieRect, 0, NIL);
    }

  } else {
  }

  goto backtoit;

out:
#ifndef PC
  UnlockPixels(backdroppix);
#endif
  DisposeGWorld(backdrop);
  DisposePixPat(backpat);
  TEDispose(textHand); // � Reclaim heap space.
  DisposeWindow(about); // � Get rid of dialog box.
  SetGWorld(savedPort, savedDevice); // � Restore the old port.
}
