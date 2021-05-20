#include <Types.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <ToolUtils.h>
#include <Memory.h>
#include <SegLoad.h>
#include <Files.h>
#include <OSUtils.h>
#include <DiskInit.h>
#include <Packages.h>
#include <Traps.h>
#include <Serial.h>
#include <Devices.h>
#include <stdio.h>
#include <string.h>
#include "Sample.h"
#include "SerialHelper.h"
#include <stdlib.h>

#include "quickdraw_renderer.h"

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"

#define WINDOW_WIDTH 510
#define WINDOW_HEIGHT 302

#define DISP_BUF_SIZE WINDOW_WIDTH * WINDOW_HEIGHT * 8
/* GMac is used to hold the result of a SysEnvirons call. This makes
   it convenient for any routine to check the environment. */
SysEnvRec	gMac;				/* set up by Initialize */

/* GHasWaitNextEvent is set at startup, and tells whether the WaitNextEvent
   trap is available. If it is false, we know that we must call GetNextEvent. */
Boolean		gHasWaitNextEvent;	/* set up by Initialize */

/* GInBackground is maintained by our osEvent handling routines. Any part of
   the program can check it to find out if it is currently in the background. */
Boolean		gInBackground;		/* maintained by Initialize and DoEvent */

 #define MAC_APP_DEBUGGING
/* The following globals are the state of the window. If we supported more than
   one window, they would be attatched to each document, rather than globals. */

/* Here are declarations for all of the C routines. In MPW 3.0 we can use
   actual prototypes for parameter type checking. */
void EventLoop ( );
void DoEvent( EventRecord *event );
void GetGlobalMouse( Point *mouse );
void DoUpdate( WindowPtr window );
void DoActivate( WindowPtr window, Boolean becomingActive );
void DoContentClick( WindowPtr window );
void AdjustMenus( void );
void DoMenuCommand( long menuResult );
Boolean DoCloseWindow( WindowPtr window );
void Terminate( void );
void Initialize( void );
Boolean GoGetRect( short rectID, Rect *theRect );
void ForceEnvirons( void );
Boolean IsAppWindow( WindowPtr window );
Boolean IsDAWindow( WindowPtr window );
Boolean TrapAvailable( short tNumber, TrapType tType );
void AlertUser( void );

void write_log(char* text) {

	writeSerialPort(boutRefNum, text);
}

/* Define HiWrd and LoWrd macros for efficiency. */
#define HiWrd(aLong)	(((aLong) >> 16) & 0xFFFF)
#define LoWrd(aLong)	((aLong) & 0xFFFF)

Boolean initialState = true;

BitMap RealMap;
int n;

void my_log_cb(lv_log_level_t level, const char * file, uint32_t line, const char * fn_name, const char * dsc)
{

    char messageType[32];
  /*Send the logs via serial port*/
  if(level == LV_LOG_LEVEL_ERROR) sprintf(messageType, "ERROR: ");
  if(level == LV_LOG_LEVEL_WARN)  sprintf(messageType, "WARNING: ");
  if(level == LV_LOG_LEVEL_INFO)  sprintf(messageType, "INFO: ");
  if(level == LV_LOG_LEVEL_TRACE) sprintf(messageType, "TRACE: ");

  char log[255];
  sprintf(log, "%s: file: %s, line: %d, fn_name: %s, dsc: %s", messageType, file, line, fn_name, dsc);
  writeSerialPort(boutRefNum, log);
}


#pragma segment Main
void main()
{	
	Initialize();					/* initialize the program */

	UnloadSeg((Ptr) Initialize);	/* note that Initialize must not be in Main! */

    WindowPtr window = FrontWindow();

    quickdraw_init(WINDOW_WIDTH, WINDOW_HEIGHT);

    RealMap = qd.thePort->portBits;

    writeSerialPort(boutRefNum, "call lv_log_register_print_cb");
    lv_log_register_print_cb(my_log_cb);

    writeSerialPort(boutRefNum, "call lv_init");
    /*LittlevGL init*/
    lv_init();

    writeSerialPort(boutRefNum, "call fbdev_init");
    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_buf_t disp_buf;
    writeSerialPort(boutRefNum, "call lv_disp_buf_init");
    lv_disp_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    lv_disp_drv_t disp_drv;
    writeSerialPort(boutRefNum, "call lv_disp_drv_init");
    lv_disp_drv_init(&disp_drv);
    disp_drv.buffer = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    writeSerialPort(boutRefNum, "call lv_disp_drv_register");
    lv_disp_drv_register(&disp_drv);
    writeSerialPort(boutRefNum, "call lv_label_create");

    /*Create a "Hello world!" label*/
    lv_obj_t * label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label, "Hello world!");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

    #ifdef MAC_APP_DEBUGGING

	    writeSerialPort(boutRefNum, "call into event loop");
	#endif

	EventLoop();					/* call the main event loop */
}


Boolean clicked = false;
#pragma segment Main
void EventLoop()
{
	RgnHandle cursorRgn;
	Boolean	gotEvent;
	EventRecord	event;
	Point mouse;
	cursorRgn = NewRgn();

	Boolean firstOrMouseMove = true;
	int lastMouseHPos = 0;
	int lastMouseVPos = 0;

    OpenPort(&gMainOffScreen.BWPort);
    SetPort(&gMainOffScreen.BWPort);
    SetPortBits(&gMainOffScreen.BWBits);
    // EraseRect(&gMainOffScreen.bounds);

	initialState = false;

	do {

        lv_tick_inc(5);
        lv_task_handler();
	    // OpenPort(&gMainOffScreen.BWPort);
	    // SetPort(&gMainOffScreen.BWPort);
	    // SetPortBits(&gMainOffScreen.BWBits);
	    // EraseRect(&gMainOffScreen.bounds);
	    //WindowPtr window = FrontWindow();

	    // SetPort(window);

	    // our offscreen bitmap is the same size as our port rectangle, so we
	    // get away with using the portRect sizing for source and destination
	    // CopyBits(&gMainOffScreen.bits->portBits, &window->portBits, &window->portRect, &window->portRect, srcCopy, 0L);

		GetGlobalMouse(&mouse);


		// as far as i can tell, there is no way to event on mouse movement with mac libraries,
		// so we are just going to track on our own, and create our own events.
		// this seems kind of a bummer to not pass this to event handling code, but to make
		// it work we would need to create a dummy event, etc, so we will just directly 
		// call the input_motion command
        if (lastMouseHPos != mouse.h || lastMouseVPos != mouse.v) {

        	#ifdef MAC_APP_DEBUGGING

        		writeSerialPort(boutRefNum, "input_motion!");
        	#endif

        	firstOrMouseMove = true;

            Point tempPoint;
            SetPt(&tempPoint, mouse.h, mouse.v);
            GlobalToLocal(&tempPoint);

        	if (clicked) {

                // UG_TouchUpdate(mouse.h, mouse.v, TOUCH_STATE_PRESSED);
            }
        }

        lastMouseHPos = mouse.h;
        lastMouseVPos = mouse.v;

		if (gHasWaitNextEvent) {

			gotEvent = WaitNextEvent(everyEvent, &event, MAXLONG, cursorRgn);
		} else {

			SystemTask();
			gotEvent = GetNextEvent(everyEvent, &event);
		}

		if (gotEvent) {

			#ifdef MAC_APP_DEBUGGING

        		writeSerialPort(boutRefNum, "calling to DoEvent");
        	#endif

			DoEvent(&event);

			#ifdef MAC_APP_DEBUGGING

        		writeSerialPort(boutRefNum, "done with DoEvent");
        	#endif
		}

        // only re-render if there is an event, prevents screen flickering
        //if (gotEvent || firstOrMouseMove) {

        	firstOrMouseMove = false;
        //}

	} while ( true );	/* loop forever; we quit via ExitToShell */
} /*EventLoop*/


/* Do the right thing for an event. Determine what kind of event it is, and call
 the appropriate routines. */


static int quickdraw_handle_event(EventRecord *event) { 
    // see: inside macintosh: toolbox essentials 2-4
    // and  inside macintosh toolbox essentials 2-79

    WindowPtr window;
    FindWindow(event->where, &window); 
    // char *logb;
    // sprintf(logb, "quickdraw_handle_event event %d", event->what);
    // writeSerialPort(boutRefNum, logb);

    switch (event->what) {
        case updateEvt: {
                return 1;
            }
            break;
        case osEvt: { 
            // the quicktime osEvts are supposed to cover mouse movement events
            // notice that we are actually calling input_motion in the EventLoop for the program
            // instead, as handling this event directly does not appear to work for whatever reason
            // TODO: research this
            writeSerialPort(boutRefNum, "osEvt");

                switch (event->message) {

                    case mouseMovedMessage: {

                        #ifdef QUICKDRAW_EVENTS_DEBUGGING
                            
                            writeSerialPort(boutRefNum, "mouseMovedMessage");
                        #endif

                        Point tempPoint;
                        SetPt(&tempPoint, event->where.h, event->where.v);
                        GlobalToLocal(&tempPoint);
                        

                        int x = tempPoint.h;
                        int y = tempPoint.v;

                        // event->where should have coordinates??? or is it just a pointer to what the mouse is over?
                        // TODO need to figure this out
                        // mu_input_mousemove(ctx, x, y); // TODO figure out mouse coordinates - not sure if this is right
                        if (clicked) {

                            // UG_TouchUpdate(x, y, TOUCH_STATE_PRESSED);
                        }
                        break;
                    }

                    return 1;
                }
            }
            break;
        
        case mouseUp: 
            #ifdef QUICKDRAW_EVENTS_DEBUGGING

                writeSerialPort(boutRefNum, "mouseUp!!!");
            #endif
        case mouseDown: {

            #ifdef QUICKDRAW_EVENTS_DEBUGGING

                writeSerialPort(boutRefNum, "mouseUp/Down");
            #endif
            
            short part = FindWindow(event->where, &window);

			switch (part) {
                case inContent: {
                    // event->where should have coordinates??? or is it just a pointer to what the mouse is over?
                    // TODO need to figure this out
                    #ifdef QUICKDRAW_EVENTS_DEBUGGING

                        writeSerialPort(boutRefNum, "mouseUp/Down IN DEFAULT ZONE!!!!");
                    #endif

                    // this converts the offset of the window to the actual location of the mouse within the window
                    Point tempPoint;
                    SetPt(&tempPoint, event->where.h, event->where.v);
                    GlobalToLocal(&tempPoint);
                    
                    if (!event->where.h) {
                        
                        #ifdef QUICKDRAW_EVENTS_DEBUGGING

                            writeSerialPort(boutRefNum, "no event location for mouse!!!!");
                        #endif

                        return 1;
                    }

                    int x = tempPoint.h;
                    int y = tempPoint.v;

                    #ifdef QUICKDRAW_EVENTS_DEBUGGING

                        char logx[255];
                        sprintf(logx, "mouse location at time of click h: %d,  v: %d", x, y);
                        writeSerialPort(boutRefNum, logx);
                    #endif

                    // mu_input_mousemove(ctx, x, y); // you can enable this if you don't want to use motion tracking
                    // in the Mac event loop handler as in the nuklear quickdraw sample, and this will allow mouse clicks to
                    // work properly, but will not allow hover states to work
                    
                    if (event->what == mouseDown) {

                        clicked = true;

                        // UG_TouchUpdate(x, y, TOUCH_STATE_PRESSED);
                    } else {

                        clicked = false;

                        // UG_TouchUpdate(x, y, TOUCH_STATE_RELEASED);
                    }
                }
                break;
                return 1;
            }
            
            break;
        case keyDown:
		case autoKey: {/* check for menukey equivalents */

                // char charKey = event->message & charCodeMask;
                // int key = (int)charKey;


                // #ifdef QUICKDRAW_EVENTS_DEBUGGING

                //     writeSerialPort(boutRefNum, "keyDown/autoKey");

                //     char logy[255];
                //     sprintf(logy, "key pressed: key: '%c', 02x: '%02X', return: '%02X', %d == %d ??", key, key, returnKey, (int)(key), (int)(returnKey));
                //     writeSerialPort(boutRefNum, logy);
                // #endif

                // const Boolean isKeyDown = event->what == keyDown;

                // if (event->modifiers & cmdKey) {/* Command key down */

                //     if (isKeyDown) {

                //         // AdjustMenus();						/* enable/disable/check menu items properly */
                //         // DoMenuCommand(MenuKey(key));
                //     }
                    
                //     if (key == 'c') {
                        
                //         // // input_key(ctx, KEY_COPY, 1);
                //     } else if (key == 'v') {
                        
                //         // // input_key(ctx, KEY_PASTE, 1);
                //     } else if (key == 'x') {
                        
                //         // // input_key(ctx, KEY_CUT, 1);
                //     } else if (key == 'z') {
                        
                //         // // input_key(ctx, KEY_TEXT_UNDO, 1);
                //     } else if (key == 'r') {
                        
                //         // // input_key(ctx, KEY_TEXT_REDO, 1);
                //     } 
                // } else if (key == eitherShiftKey) {
                    
                //     // input_key(ctx, KEY_SHIFT, isKeyDown);
                // } else if (key == deleteKey) {
                    
                //     // input_key(ctx, KEY_DEL, isKeyDown);
                // } else if (key == enterKey) {
                    
                //     // input_key(ctx, KEY_ENTER, isKeyDown);
                // } else if (key == returnKey) {
                    
                //     // input_key(ctx, KEY_ENTER, isKeyDown);
                // } else if (key == tabKey) {
                    
                //     // input_key(ctx, KEY_TAB, isKeyDown);
                // } else if (key == leftArrowKey) {
                    
                //     // input_key(ctx, KEY_LEFT, isKeyDown);
                // } else if (key == rightArrowKey) {
                    
                //     // input_key(ctx, KEY_RIGHT, isKeyDown);
                // } else if (key == upArrowKey) {
                    
                //     // input_key(ctx, KEY_UP, isKeyDown);
                // } else if (key == downArrowKey) {
                    
                //     // input_key(ctx, KEY_DOWN, isKeyDown);
                // } else if (key == backspaceKey) {
                    
                //     // input_key(ctx, KEY_BACKSPACE, isKeyDown);
                // } else if (key == escapeKey) {
                    
                //     // // input_key(ctx, KEY_TEXT_RESET_MODE, isKeyDown);
                // } else if (key == pageUpKey) {
                 
                //     // input_key(ctx, KEY_SCROLL_UP, isKeyDown);
                // } else if (key == pageDownKey) {
                    
                //     // input_key(ctx, KEY_SCROLL_DOWN, isKeyDown);
                // } else if (key == homeKey) {

                //     // // input_key(ctx, KEY_TEXT_START, isKeyDown);
                //     // input_key(ctx, KEY_SCROLL_START, isKeyDown);
                // } else if (key == endKey) {

                //     // // input_key(ctx, KEY_TEXT_END, isKeyDown);
                //     // input_key(ctx, KEY_SCROLL_END, isKeyDown);
                // } else {

                //     #ifdef QUICKDRAW_EVENTS_DEBUGGING

                //         writeSerialPort(boutRefNum, "default keydown/autokey event");
                //     #endif

                //     if (key > 127) {
                //         // invalid char
                //         mu_input_text(ctx, "?");
                //     } else {
                //         char ascii_c[2] = {(char)key, '\0'};
                //         mu_input_text(ctx, ascii_c);
                //     }
                // }

                // return 1;
            }
			break;
        default: {
                #ifdef QUICKDRAW_EVENTS_DEBUGGING

                    writeSerialPort(boutRefNum, "default unhandled event");
                #endif
            
                return 1; 
            }
            break;
        }
    }
}

#pragma segment Main
void DoEvent(EventRecord *event) {

	short part;
    short err;
	WindowPtr window;
	Boolean	hit;
	char key;
	Point aPoint;

	switch ( event->what ) {

        case mouseUp:

	    	#ifdef MAC_APP_DEBUGGING
	        	writeSerialPort(boutRefNum, "mouseup");
        	#endif

            part = FindWindow(event->where, &window);
            switch (part)
            {
                case inContent:
                    quickdraw_handle_event(event);
                    break;
                default:
                	break;
            }
            break;
		case mouseDown:


	    	#ifdef MAC_APP_DEBUGGING
	        	writeSerialPort(boutRefNum, "mousedown");
	        #endif

			part = FindWindow(event->where, &window);
			switch ( part ) {
				case inMenuBar:				/* process a mouse menu command (if any) */
					AdjustMenus();
					DoMenuCommand(MenuSelect(event->where));
					break;
				case inSysWindow:			/* let the system handle the mouseDown */
					SystemClick(event, window);
					break;
				case inContent:
					if ( window != FrontWindow() ) {
						SelectWindow(window);

					}
                    quickdraw_handle_event(event);
					break;
				case inDrag:				/* pass screenBits.bounds to get all gDevices */
					DragWindow(window, event->where, &qd.screenBits.bounds);
					break;
				case inGrow:
					break;
				case inZoomIn:
				case inZoomOut:
					hit = TrackBox(window, event->where, part);
					if ( hit ) {
						SetPort(window);				/* the window must be the current port... */
						EraseRect(&window->portRect);	/* because of a bug in ZoomWindow */
						ZoomWindow(window, part, true);	/* note that we invalidate and erase... */
						InvalRect(&window->portRect);	/* to make things look better on-screen */
					}
					break;
			}
			break;
		case keyDown:
		case autoKey:						/* check for menukey equivalents */
        	

	    	#ifdef MAC_APP_DEBUGGING
	        	writeSerialPort(boutRefNum, "key");
	        #endif

			key = event->message & charCodeMask;
			if ( event->modifiers & cmdKey )	{		/* Command key down */
				if ( event->what == keyDown ) {
					AdjustMenus();						/* enable/disable/check menu items properly */
					DoMenuCommand(MenuKey(key));
				}
			}

            quickdraw_handle_event(event);
			break;
		case activateEvt:
			
    		#ifdef MAC_APP_DEBUGGING
        		writeSerialPort(boutRefNum, "activate");
        	#endif
			DoActivate((WindowPtr) event->message, (event->modifiers & activeFlag) != 0);
			break;
		case updateEvt:
			
			#ifdef MAC_APP_DEBUGGING
        		writeSerialPort(boutRefNum, "update");
        	#endif
			DoUpdate((WindowPtr) event->message);
			break;
		/*	1.01 - It is not a bad idea to at least call DIBadMount in response
			to a diskEvt, so that the user can format a floppy. */
		case diskEvt:
			
    		#ifdef MAC_APP_DEBUGGING
        		writeSerialPort(boutRefNum, "disk");
        	#endif
			if ( HiWord(event->message) != noErr ) {
				SetPt(&aPoint, kDILeft, kDITop);
				err = DIBadMount(aPoint, event->message);
			}
			break;

		case osEvt:
			
    		#ifdef MAC_APP_DEBUGGING
        		writeSerialPort(boutRefNum, "os");
        	#endif

			// this should be trigger on mousemove but does not -- if we can figure that out, we should call through to 
			// quickdraw_handle_event, and allow it to handle the mousemove events
		/*	1.02 - must BitAND with 0x0FF to get only low byte */
			switch ((event->message >> 24) & 0x0FF) {		/* high byte of message */
				case kSuspendResumeMessage:		/* suspend/resume is also an activate/deactivate */
					gInBackground = (event->message & kResumeMask) == 0;
					DoActivate(FrontWindow(), !gInBackground);
					break;
			}
			break;
	}
} /*DoEvent*/

/*	Get the global coordinates of the mouse. When you call OSEventAvail

	it will return either a pending event or a null event. In either case,
	the where field of the event record will contain the current position
	of the mouse in global coordinates and the modifiers field will reflect
	the current state of the modifiers. Another way to get the global
	coordinates is to call GetMouse and LocalToGlobal, but that requires
	being sure that thePort is set to a valid port. */

#pragma segment Main
void GetGlobalMouse(mouse)
	Point	*mouse;
{
	EventRecord	event;
	
	OSEventAvail(kNoEvents, &event);	/* we aren't interested in any events */
	*mouse = event.where;				/* just the mouse position */
} /*GetGlobalMouse*/


/*	This is called when an update event is received for a window.
	It calls DrawWindow to draw the contents of an application window.
	As an effeciency measure that does not have to be followed, it
	calls the drawing routine only if the visRgn is non-empty. This
	will handle situations where calculations for drawing or drawing
	itself is very time-consuming. */

#pragma segment Main
void DoUpdate(window)
	WindowPtr	window;
{
	if ( IsAppWindow(window) ) {
		BeginUpdate(window);		
		EndUpdate(window);
	}
} /*DoUpdate*/


/*	This is called when a window is activated or deactivated.
	In Sample, the Window Manager's handling of activate and
	deactivate events is sufficient. Other applications may have
	TextEdit records, controls, lists, etc., to activate/deactivate. */

#pragma segment Main
void DoActivate(window, becomingActive)
	WindowPtr	window;
	Boolean		becomingActive;
{
	if ( IsAppWindow(window) ) {
		if ( becomingActive )
			/* do whatever you need to at activation */ ;
		else
			/* do whatever you need to at deactivation */ ;
	}
} /*DoActivate*/



/* Draw the contents of the application window. We do some drawing in color, using
   Classic QuickDraw's color capabilities. This will be black and white on old
   machines, but color on color machines. At this point, the windowÕs visRgn
   is set to allow drawing only where it needs to be done. */

static Rect				okayButtonBounds;


/*	Enable and disable menus based on the current state.
	The user can only select enabled menu items. We set up all the menu items
	before calling MenuSelect or MenuKey, since these are the only times that
	a menu item can be selected. Note that MenuSelect is also the only time
	the user will see menu items. This approach to deciding what enable/
	disable state a menu item has the advantage of concentrating all
	the decision-making in one routine, as opposed to being spread throughout
	the application. Other application designs may take a different approach
	that is just as valid. */

#pragma segment Main
void AdjustMenus()
{
	WindowPtr	window;
	MenuHandle	menu;

	window = FrontWindow();

	menu = GetMenuHandle(mFile);
	if ( IsDAWindow(window) )		/* we can allow desk accessories to be closed from the menu */
		EnableItem(menu, iClose);
	else
		DisableItem(menu, iClose);	/* but not our traffic light window */

	menu = GetMenuHandle(mEdit);
	if ( IsDAWindow(window) ) {		/* a desk accessory might need the edit menuÉ */
		EnableItem(menu, iUndo);
		EnableItem(menu, iCut);
		EnableItem(menu, iCopy);
		EnableItem(menu, iClear);
		EnableItem(menu, iPaste);
	} else {						/* Ébut we donÕt use it */
		DisableItem(menu, iUndo);
		DisableItem(menu, iCut);
		DisableItem(menu, iCopy);
		DisableItem(menu, iClear);
		DisableItem(menu, iPaste);
	}

	menu = GetMenuHandle(mLight);
	if ( IsAppWindow(window) ) {	/* we know that it must be the traffic light */
		EnableItem(menu, iStop);
		EnableItem(menu, iGo);
	} else {
		DisableItem(menu, iStop);
		DisableItem(menu, iGo);
	}
} /*AdjustMenus*/


/*	This is called when an item is chosen from the menu bar (after calling
	MenuSelect or MenuKey). It performs the right operation for each command.
	It is good to have both the result of MenuSelect and MenuKey go to
	one routine like this to keep everything organized. */

#pragma segment Main
void DoMenuCommand(menuResult)
	long		menuResult;
{
	short		menuID;				/* the resource ID of the selected menu */
	short		menuItem;			/* the item number of the selected menu */
	short		itemHit;
	Str255		daName;
	short		daRefNum;
	Boolean		handledByDA;

	menuID = HiWord(menuResult);	/* use macros for efficiency to... */
	menuItem = LoWord(menuResult);	/* get menu item number and menu number */
	switch ( menuID ) {
		case mApple:
			switch ( menuItem ) {
				// case iAbout:		/* bring up alert for About */
                default:
					itemHit = Alert(rAboutAlert, nil);
					break;

                /*
				default:			// all non-About items in this menu are DAs
					// type Str255 is an array in MPW 3
					GetItem(GetMHandle(mApple), menuItem, daName);
					daRefNum = OpenDeskAcc(daName);
					break;
                */
			}
			break;
		case mFile:
			switch ( menuItem ) {
				case iClose:
					DoCloseWindow(FrontWindow());
					break;
				case iQuit:
					Terminate();
					break;
			}
			break;
		case mEdit:					/* call SystemEdit for DA editing & MultiFinder */
			handledByDA = SystemEdit(menuItem-1);	/* since we donÕt do any Editing */
			break;
		case mLight:
			switch ( menuItem ) {
				case iStop:
					break;
				case iGo:
					break;
			}
			break;

        case mHelp:
			switch ( menuItem ) {
				case iQuickHelp:
                    itemHit = Alert(rAboutAlert, nil);                        
					break;
				case iUserGuide:
                {
					// AlertUser();

                    // write data to serial port
                    // Configure PCE/macplus to map serial port to ser_b.out. This port can then be used for debug output
                    // by using: tail -f ser_b.out
                    // OSErr res = writeSerialPort(boutRefNum, "Hello World");
                    // if (res < 0)
                    //    AlertUser();

                    // http://www.mac.linux-m68k.org/devel/macalmanac.php
                    short* ROM85      = (short*) 0x028E; 
                    short* SysVersion = (short*) 0x015A; 
                    short* ScrVRes    = (short*) 0x0102; 
                    short* ScrHRes    = (short*) 0x0104; 
                    unsigned long*  Time       = (unsigned long*) 0x020C; 

                    char str2[255];
                    sprintf(str2, "ROM85: %d - SysVersion: %d - VRes: %d - HRes: %d - Time: %lu", *ROM85, *SysVersion, *ScrVRes, *ScrHRes, *Time);
                    // writeSerialPort(boutRefNum, str2);               

                    Boolean is128KROM = ((*ROM85) > 0);
                    Boolean hasSysEnvirons = false;
                    Boolean hasStripAddr = false;
                    Boolean hasSetDefaultStartup = false;
                    if (is128KROM)
                    {
                        UniversalProcPtr trapSysEnv = GetOSTrapAddress(_SysEnvirons);
                        UniversalProcPtr trapStripAddr = GetOSTrapAddress(_StripAddress);
                        UniversalProcPtr trapSetDefaultStartup = GetOSTrapAddress(_SetDefaultStartup);
                        UniversalProcPtr trapUnimpl = GetOSTrapAddress(_Unimplemented);

                        hasSysEnvirons = (trapSysEnv != trapUnimpl);
                        hasStripAddr = (trapStripAddr != trapUnimpl);
                        hasSetDefaultStartup = (trapSetDefaultStartup != trapUnimpl);
                    }
                    
                    sprintf(str2, "is128KROM: %d - hasSysEnvirons: %d - hasStripAddr: %d - hasSetDefaultStartup - %d", 
                                    is128KROM, hasSysEnvirons, hasStripAddr, hasSetDefaultStartup);
                    // writeSerialPort(boutRefNum, str2);               

					break;
                }
			}
			break;
	}
	HiliteMenu(0);					/* unhighlight what MenuSelect (or MenuKey) hilited */
} /*DoMenuCommand*/

/* Close a window. This handles desk accessory and application windows. */

/*	1.01 - At this point, if there was a document associated with a
	window, you could do any document saving processing if it is 'dirty'.
	DoCloseWindow would return true if the window actually closed, i.e.,
	the user didnÕt cancel from a save dialog. This result is handy when
	the user quits an application, but then cancels the save of a document
	associated with a window. */

#pragma segment Main
Boolean DoCloseWindow(window)
	WindowPtr	window;
{
	/* if ( IsDAWindow(window) )
		CloseDeskAcc(((WindowPeek) window)->windowKind);
	else */ if ( IsAppWindow(window) )
		CloseWindow(window);
	return true;
} /*DoCloseWindow*/


/* Clean up the application and exit. We close all of the windows so that
 they can update their documents, if any. */
 
/*	1.01 - If we find out that a cancel has occurred, we won't exit to the
	shell, but will return instead. */

#pragma segment Main
void Terminate()
{
	WindowPtr	aWindow;
	Boolean		closed;
	
	closed = true;
	do {
		aWindow = FrontWindow();				/* get the current front window */
		if (aWindow != nil)
			closed = DoCloseWindow(aWindow);	/* close this window */	
	}
	while (closed && (aWindow != nil));
	if (closed)
		ExitToShell();							/* exit if no cancellation */
} /*Terminate*/


#pragma segment Initialize
void Initialize()
{
	Handle		menuBar;
	WindowPtr	window;
	long		total, contig;
	EventRecord event;
	short		count;

	gInBackground = false;

	InitGraf((Ptr) &qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();

	for (count = 1; count <= 3; count++) {
		EventAvail(everyEvent, &event);
	}

	window = (WindowPtr) NewPtr(sizeof(WindowRecord));

	if ( window == nil ) {
		AlertUser();
	}

	window = GetNewWindow(rWindow, (Ptr) window, (WindowPtr) -1);

	menuBar = GetNewMBar(rMenuBar);			/* read menus into menu bar */

	if ( menuBar == nil ) {

		AlertUser();
	}

	SetMenuBar(menuBar);					/* install menus */
	DisposeHandle(menuBar);
	AppendResMenu(GetMenuHandle(mApple), 'DRVR');	/* add DA names to Apple menu */    
	DrawMenuBar();
	
} /*Initialize*/


#pragma segment Main
Boolean IsAppWindow(window)
	WindowPtr	window;
{
	short windowKind;

	if ( window == nil ) {
		return false;
	}	/* application windows have windowKinds = userKind (8) */

	windowKind = ((WindowPeek) window)->windowKind;

	return (windowKind = userKind);
} /*IsAppWindow*/


/* Check to see if a window belongs to a desk accessory. */

#pragma segment Main
Boolean IsDAWindow(window)
	WindowPtr	window;
{
	if ( window == nil ) {
		return false;
	}
		/* DA windows have negative windowKinds */
	return ((WindowPeek) window)->windowKind < 0;
} /*IsDAWindow*/


#pragma segment Initialize
Boolean TrapAvailable(tNumber,tType)
	short		tNumber;
	TrapType	tType;
{    
	if ( ( tType == ToolTrap ) &&
		( gMac.machineType > envMachUnknown ) &&
		( gMac.machineType < envMacII ) ) {		/* it's a 512KE, Plus, or SE */
		tNumber = tNumber & 0x03FF;
		if ( tNumber > 0x01FF )		 {			/* which means the tool traps */
			tNumber = _Unimplemented;			/* only go to 0x01FF */
		}
	}

	return NGetTrapAddress(tNumber, tType) != GetTrapAddress(_Unimplemented);
} /*TrapAvailable*/


#pragma segment Main
void AlertUser() {
	short itemHit;

	SetCursor(&qd.arrow);
	itemHit = Alert(rUserAlert, nil);
	ExitToShell();
} /* AlertUser */