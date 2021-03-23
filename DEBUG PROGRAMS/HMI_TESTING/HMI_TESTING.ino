//<File !Start!>
// FILE: [HMI.ino]
// Created by GUIslice Builder version: [0.13.b006]
//
// GUIslice Builder Generated File
//
// For the latest guides, updates and support view:
// https://github.com/ImpulseAdventure/GUIslice
//
//<File !End!>
//
// ARDUINO NOTES:
// - GUIslice_config.h must be edited to match the pinout connections
//   between the Arduino CPU and the display controller (see ADAGFX_PIN_*).
//

// ------------------------------------------------
// Headers to include
// ------------------------------------------------
#include "GUIslice.h"
#include "GUIslice_drv.h"

// Include any extended elements
//<Includes !Start!>
//<Includes !End!>

// ------------------------------------------------
// Headers and Defines for fonts
// Note that font files are located within the Adafruit-GFX library folder:
// ------------------------------------------------
//<Fonts !Start!>
//<Fonts !End!>

// ------------------------------------------------
// Defines for resources
// ------------------------------------------------
//<Resources !Start!>
//<Resources !End!>

// ------------------------------------------------
// Enumerations for pages, elements, fonts, images
// ------------------------------------------------
//<Enum !Start!>
enum {E_PG_MAIN};
enum {E_ELEM_TEXT1};
// Must use separate enum for fonts with MAX_FONT at end to use gslc_FontSet.
enum {E_FONT_TXT5,MAX_FONT};
//<Enum !End!>

// ------------------------------------------------
// Instantiate the GUI
// ------------------------------------------------

// ------------------------------------------------
// Define the maximum number of elements and pages
// ------------------------------------------------
//<ElementDefines !Start!>
#define MAX_PAGE                1

#define MAX_ELEM_PG_MAIN 1 // # Elems total on page
#define MAX_ELEM_PG_MAIN_RAM MAX_ELEM_PG_MAIN // # Elems in RAM
//<ElementDefines !End!>

// ------------------------------------------------
// Create element storage
// ------------------------------------------------
gslc_tsGui                      m_gui;
gslc_tsDriver                   m_drv;
gslc_tsFont                     m_asFont[MAX_FONT];
gslc_tsPage                     m_asPage[MAX_PAGE];

//<GUI_Extra_Elements !Start!>
gslc_tsElem                     m_asPage1Elem[MAX_ELEM_PG_MAIN_RAM];
gslc_tsElemRef                  m_asPage1ElemRef[MAX_ELEM_PG_MAIN];

#define MAX_STR                 100

//<GUI_Extra_Elements !End!>

// ------------------------------------------------
// Program Globals
// ------------------------------------------------

// Save some element references for direct access
//<Save_References !Start!>
//<Save_References !End!>

// Define debug message function
static int16_t DebugOut(char ch) { if (ch == (char)'\n') Serial.println(""); else Serial.write(ch); return 0; }

// ------------------------------------------------
// Callback Methods
// ------------------------------------------------
//<Button Callback !Start!>
//<Button Callback !End!>
//<Checkbox Callback !Start!>
//<Checkbox Callback !End!>
//<Keypad Callback !Start!>
//<Keypad Callback !End!>
//<Spinner Callback !Start!>
//<Spinner Callback !End!>
//<Listbox Callback !Start!>
//<Listbox Callback !End!>
//<Draw Callback !Start!>
//<Draw Callback !End!>
//<Slider Callback !Start!>
//<Slider Callback !End!>
//<Tick Callback !Start!>
//<Tick Callback !End!>

// ------------------------------------------------
// Create page elements
// ------------------------------------------------
bool InitGUI()
{
	gslc_tsElemRef* pElemRef = NULL;

	//<InitGUI !Start!>
	gslc_PageAdd(&m_gui,E_PG_MAIN,m_asPage1Elem,MAX_ELEM_PG_MAIN_RAM,m_asPage1ElemRef,MAX_ELEM_PG_MAIN);

	// NOTE: The current page defaults to the first page added. Here we explicitly
	//       ensure that the main page is the correct page no matter the add order.
	gslc_SetPageCur(&m_gui,E_PG_MAIN);
	
	// Set Background to a flat color
	gslc_SetBkgndColor(&m_gui,GSLC_COL_BLACK);

	// -----------------------------------
	// PAGE: E_PG_MAIN
	
	
	// Create E_ELEM_TEXT1 text label
	pElemRef = gslc_ElemCreateTxt(&m_gui,E_ELEM_TEXT1,E_PG_MAIN,(gslc_tsRect){114,187,24,10},
	(char*)"",0,E_FONT_TXT5);
	//<InitGUI !End!>

	return true;
}

void setup()
{
	// ------------------------------------------------
	// Initialize
	// ------------------------------------------------
	Serial.begin(9600);
	// Wait for USB Serial
	//delay(1000);  // NOTE: Some devices require a delay after Serial.begin() before serial port can be used

	gslc_InitDebug(&DebugOut);

	if (!gslc_Init(&m_gui,&m_drv,m_asPage,MAX_PAGE,m_asFont,MAX_FONT)) { return; }

	// ------------------------------------------------
	// Load Fonts
	// ------------------------------------------------
	//<Load_Fonts !Start!>
	if (!gslc_FontSet(&m_gui,E_FONT_TXT5,GSLC_FONTREF_PTR,NULL,1)) { return; }
	//<Load_Fonts !End!>

	// ------------------------------------------------
	// Create graphic elements
	// ------------------------------------------------
	InitGUI();

	//<Startup !Start!>
	//<Startup !End!>

}

// -----------------------------------
// Main event loop
// -----------------------------------
void loop()
{

	// ------------------------------------------------
	// Update GUI Elements
	// ------------------------------------------------
	
	//TODO - Add update code for any text, gauges, or sliders
	
	// ------------------------------------------------
	// Periodically call GUIslice update function
	// ------------------------------------------------
	gslc_Update(&m_gui);
	
}
