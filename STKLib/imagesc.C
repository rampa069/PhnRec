/***************************************************************************
 *   copyright            : (C) 2004 by Lukas Burget,UPGM,FIT,VUT,Brno     *
 *   email                : burget@fit.vutbr.cz                            *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <Xm/Protocols.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <X11/xpm.h>
#include <X11/keysym.h>
#include <math.h>
#include "imagesc.h"

const char *cm_gray[] = {
"#000000","#040404","#080808","#0C0C0C","#101010","#141414","#181818","#1C1C1C",
"#202020","#242424","#282828","#2D2D2D","#313131","#353535","#393939","#3D3D3D",
"#414141","#454545","#494949","#4D4D4D","#515151","#555555","#595959","#5D5D5D",
"#616161","#656565","#696969","#6D6D6D","#717171","#757575","#797979","#7D7D7D",
"#828282","#868686","#8A8A8A","#8E8E8E","#929292","#969696","#9A9A9A","#9E9E9E",
"#A2A2A2","#A6A6A6","#AAAAAA","#AEAEAE","#B2B2B2","#B6B6B6","#BABABA","#BEBEBE",
"#C2C2C2","#C6C6C6","#CACACA","#CECECE","#D2D2D2","#D7D7D7","#DBDBDB","#DFDFDF",
"#E3E3E3","#E7E7E7","#EBEBEB","#EFEFEF","#F3F3F3","#F7F7F7","#FBFBFB","#FFFFFF"
};

const char *cm_color[] = {
"#00008F","#00009F","#0000AF","#0000BF","#0000CF","#0000DF","#0000EF","#0000FF",
"#0010FF","#0020FF","#0030FF","#0040FF","#0050FF","#0060FF","#0070FF","#0080FF",
"#008FFF","#009FFF","#00AFFF","#00BFFF","#00CFFF","#00DFFF","#00EFFF","#00FFFF",
"#10FFFF","#20FFEF","#30FFDF","#40FFCF","#50FFBF","#60FFAF","#70FF9F","#80FF8F",
"#8FFF80","#9FFF70","#AFFF60","#BFFF50","#CFFF40","#DFFF30","#EFFF20","#FFFF10",
"#FFFF00","#FFEF00","#FFDF00","#FFCF00","#FFBF00","#FFAF00","#FF9F00","#FF8F00",
"#FF8000","#FF7000","#FF6000","#FF5000","#FF4000","#FF3000","#FF2000","#FF1000",
"#FF0000","#EF0000","#DF0000","#CF0000","#BF0000","#AF0000","#9F0000","#8F0000"
};

#define CONVER_DATA_FROM(type) \
  for (i = 0; i < x * y; i++) data_dbl[i] = ((type *) data)[i]

XpmImage *CreateXpmImage(void *data, int x, int y, const char* data_type,
                         double (*transf)(double), const char **colormap)
{
   XpmImage *xpmImage;
   double maxValue, minValue;
   int i, ncolors = 64;
   double *data_dbl;
   char *type;

   data_dbl = malloc(x * y * sizeof(double));
   type = strdup(data_type);

   if (!data_dbl || !type) return NULL;

   if (strcmp(type, "signed char") && !strncmp(type, "signed ", 7)) type += 7;
   i = strlen(type);
   if (i > 3 && !strcmp(type + i - 4, " int")) type[i-4] = '\n';

   if (!strcmp(type, "double")) memcpy(data_dbl, data, x * y * sizeof(double));
   else if (!strcmp(type, "float"))         CONVER_DATA_FROM(float);
   else if (!strcmp(type, "int") ||
           !strcmp(type, "signed"))        CONVER_DATA_FROM(int);
   else if (!strcmp(type, "unsigned"))      CONVER_DATA_FROM(unsigned);
   else if (!strcmp(type, "long"))          CONVER_DATA_FROM(long);
   else if (!strcmp(type, "unsigned long")) CONVER_DATA_FROM(unsigned long);
   else if (!strcmp(type, "short"))          CONVER_DATA_FROM(short);
   else if (!strcmp(type, "unsigned short")) CONVER_DATA_FROM(unsigned short);
   else if (!strcmp(type, "char"))          CONVER_DATA_FROM(char);
   else if (!strcmp(type, "signed char"))   CONVER_DATA_FROM(signed char);
   else if (!strcmp(type, "unsigned char")) CONVER_DATA_FROM(unsigned char);
   free(type);

   xpmImage = (XpmImage *) malloc(sizeof(XpmImage) +
                                  ncolors * sizeof(XpmColor) +
                                  x * y * sizeof(unsigned));
   if (!xpmImage) {
     free(data_dbl);
     return NULL;
   }

   xpmImage->width   = x;
   xpmImage->height  = y;
   xpmImage->cpp     = 0;
   xpmImage->ncolors = ncolors;
   xpmImage->colorTable = (XpmColor *) (xpmImage + 1);
   xpmImage->mpData       = (unsigned *) (xpmImage->colorTable + ncolors);

   memset(xpmImage->colorTable, 0, ncolors * sizeof(XpmColor));

   for (i=0; i < ncolors; i++) {
     xpmImage->colorTable[i].c_color = (char *) colormap[i];
   }

   if (transf) for (i=0; i < x * y; i++) data_dbl[i] = transf(data_dbl[i]);

   maxValue = minValue = data_dbl[0];

   for (i=1; i < x * y; i++) {
     if (data_dbl[i] < minValue) minValue = data_dbl[i];
     if (data_dbl[i] > maxValue) maxValue = data_dbl[i];
   }

   for (i=0; i < x * y; i++) {
     xpmImage->mpData[i] = (unsigned)(0.5 + (ncolors-1) * (data_dbl[i] - minValue)
                                          / (maxValue - minValue));
   }

   free(data_dbl);
   return xpmImage;
}

void draw_pixmap(Widget w, XtPointer client_data, XtPointer call_data)
{
    GC gc; XGCValues vals;
    XpmImage *xpmImage = (XpmImage*) client_data;
//    Pixmap pixmap;
    XImage *image, *si;

    gc=XtGetGC(w, 0 ,&vals);


    XpmCreateImageFromXpmImage(XtDisplay(w), xpmImage, &image, &si, NULL);

    XPutImage(XtDisplay(w), XtWindow(w), gc, image, 0, 0, 0, 0,
              xpmImage->width, xpmImage->height);

    XDestroyImage(image);

//    XpmCreatePixmapFromXpmImage(XtDisplay(w), XtWindow(w),
//                                xpmImage, &pixmap, NULL, NULL);
//    XCopyArea(XtDisplay(w),pixmap,XtWindow(w), gc, 0, 0,
//              xpmImage->width, xpmImage->height, 0, 0);
//    XFreePixmap(XtDisplay(w),pixmap);

    XtReleaseGC(w, gc);

}

void quitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  *(Boolean *)client_data = True;
}

void KeyAction(Widget w, XtPointer cd, XEvent *event, Boolean *co)
{
  KeySym keysym;
  keysym = XKeycodeToKeysym(XtDisplay(w), event->xkey.keycode, 0);
  if (keysym == XK_Escape) *(Boolean *) cd = True;
}

void imagesc(void *data, int x, int y, const char* type,
             double (*transf)(double), const char **colormap, char *title)
{
  static XtAppContext context;
  Widget drawing_area;
  Widget toplevel;
  Atom wm_delete;
  XEvent event;
  XpmImage *xpmImage;
  Boolean exitFlag = False;
  static Display *display = NULL;
  Arg al[10];
  int ac;
  int argc = 0;

  xpmImage = CreateXpmImage(data, x, y, type, transf, colormap);

  /* create the toplevel shell */
  XtToolkitInitialize();
  if (display == NULL) {
    context = XtCreateApplicationContext();
    display = XtOpenDisplay(context, NULL, NULL, "", NULL, 0, &argc, NULL);
  }
  toplevel =XtAppCreateShell(title, "", applicationShellWidgetClass, display, NULL, 0);

  /* set window size. */
  ac=0;
  XtSetArg(al[ac],XmNmaxWidth,  x); ac++;
  XtSetArg(al[ac],XmNmaxHeight, y); ac++;
  XtSetArg(al[ac],XmNminWidth,  x); ac++;
  XtSetArg(al[ac],XmNminHeight, y); ac++;
  XtSetArg(al[ac],XmNdeleteResponse, XmDO_NOTHING); ac++;
  XtSetValues(toplevel,al,ac);

  ac=0;
  drawing_area=XmCreateDrawingArea(toplevel,"drawing_area",al,ac);
  XtManageChild(drawing_area);
  XtAddCallback(drawing_area,XmNexposeCallback,draw_pixmap,(XtPointer) xpmImage);
  XtAddEventHandler(drawing_area, KeyReleaseMask, false, KeyAction, (XtPointer) &exitFlag);
  XtRealizeWidget(toplevel);
  wm_delete = XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", False);
  XmAddWMProtocolCallback(toplevel, wm_delete, quitCB, (XtPointer) &exitFlag);
  XmActivateWMProtocol(toplevel, wm_delete);

  while (!exitFlag) {
    XtAppNextEvent(context, &event);
    XtDispatchEvent(&event);
  }

  XtDestroyWidget(drawing_area);
  XtDestroyWidget(toplevel);
  free(xpmImage);
}
