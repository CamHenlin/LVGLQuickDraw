
#include <Quickdraw.h>
#include <Scrap.h>
#include <Serial.h>
#include "SerialHelper.h"


static struct context* quickdraw_init(unsigned int width, unsigned int height);

static void quickdraw_render(WindowPtr window);
static int quickdraw_font_get_text_width(const char *text, int len);
static int text_height();



/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */


// constant keyboard mappings for convenenience
// See Inside Macintosh: Text pg A-7, A-8
int homeKey = (int)0x01;
int enterKey = (int)0x03;
int endKey = (int)0x04;
int helpKey = (int)0x05;
int backspaceKey = (int)0x08;
int deleteKey = (int)0x7F;
int tabKey = (int)0x09;
int pageUpKey = (int)0x0B;
int pageDownKey = (int)0x0C;
int returnKey = (int)0x0D;
int rightArrowKey = (int)0x1D;
int leftArrowKey = (int)0x1C;
int downArrowKey = (int)0x1F;
int upArrowKey = (int)0x1E;
int eitherShiftKey = (int)0x0F;
int escapeKey = (int)0x1B;

// #define QUICKDRAW_GRAPHICS_DEBUGGING
// #define QUICKDRAW_EVENTS_DEBUGGING

typedef struct {
    Ptr Address;
    long RowBytes;
    GrafPtr bits;
    Rect bounds;
    
    BitMap  BWBits;
    GrafPort BWPort;
    
    Handle  OrigBits;
    
} ShockBitmap;

void NewShockBitmap(ShockBitmap *theMap, short width, short height) {

    theMap->bits = 0L;
    SetRect(&theMap->bounds, 0, 0, width, height);
    
    theMap->BWBits.bounds = theMap->bounds;
    theMap->BWBits.rowBytes = ((width+15) >> 4)<<1;         // round to even
    theMap->BWBits.baseAddr = NewPtr(((long) height * (long) theMap->BWBits.rowBytes));

    theMap->BWBits.baseAddr = StripAddress(theMap->BWBits.baseAddr);
    
    OpenPort(&theMap->BWPort);
    SetPort(&theMap->BWPort);
    SetPortBits(&theMap->BWBits);

    SetRectRgn(theMap->BWPort.visRgn, theMap->bounds.left, theMap->bounds.top, theMap->bounds.right, theMap->bounds.bottom);
    SetRectRgn(theMap->BWPort.clipRgn, theMap->bounds.left, theMap->bounds.top, theMap->bounds.right, theMap->bounds.bottom);
    EraseRect(&theMap->bounds);
      
    theMap->Address = theMap->BWBits.baseAddr;
    theMap->RowBytes = (long) theMap->BWBits.rowBytes;
    theMap->bits = (GrafPtr) &theMap->BWPort;
}

ShockBitmap gMainOffScreen;

static struct quickdraw {
    unsigned int width;
    unsigned int height;
} quickdraw;

int widthFor12ptFont[128] = {
    0,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    8,
    10,
    10,
    10,
    0,
    10,
    10,
    10,
    11,
    11,
    9,
    11,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    10,
    4,
    6,
    7,
    10,
    7,
    11,
    10,
    3,
    5,
    5,
    7,
    7,
    4,
    7,
    4,
    7,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    4,
    4,
    6,
    8,
    6,
    8,
    11,
    8,
    8,
    8,
    8,
    7,
    7,
    8,
    8,
    6,
    7,
    9,
    7,
    12,
    9,
    8,
    8,
    8,
    8,
    7,
    6,
    8,
    8,
    12,
    8,
    8,
    8,
    5,
    7,
    5,
    8,
    8,
    6,
    8,
    8,
    7,
    8,
    8,
    6,
    8,
    8,
    4,
    6,
    8,
    4,
    12,
    8,
    8,
    8,
    8,
    6,
    7,
    6,
    8,
    8,
    12,
    8,
    8,
    8,
    5,
    5,
    5,
    8,
    8
};

// note: if this produces a greater value than the actual length of the text, 
// the cursor will walk off to the right
// too small, it will precede the end of the text
// TODO: fully convert
// TODO: assuming system font for v1, support other fonts in v2
// doing this in a "fast" way by using a precomputed table for a 12pt font
static int quickdraw_font_get_text_width(const char *text, int len) {

    // writeSerialPort(boutRefNum, "quickdraw_font_get_text_width");

    if (!text || len == 0) {

        return 0;
    }

    int width = 0;

    for (int i = 0; i < len; i++) {

        width += widthFor12ptFont[(int)text[i]];
    }

    return width;
}

static int text_height() {

  return 12;
}

// it us up to our "main" function to call this code
static struct context* quickdraw_init(unsigned int width, unsigned int height) {

    NewShockBitmap(&gMainOffScreen, width, height);
}

static void quickdraw_shutdown(void) {

    memset(&quickdraw, 0, sizeof(quickdraw));
}
        

        
