/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: panelCB */
#define  PANEL_ACQUIRE                    2       /* control type: command, callback function: RunTest */
#define  PANEL_QUITBUTTON                 3       /* control type: command, callback function: QuitCallback */
#define  PANEL_GRAPH                      4       /* control type: graph, callback function: (none) */
#define  PANEL_RESULT                     5       /* control type: textBox, callback function: (none) */
#define  PANEL_RAMPUP                     6       /* control type: radioButton, callback function: Runup */
#define  PANEL_OVERSHOOT                  7       /* control type: radioButton, callback function: Overshoot */
#define  PANEL_ACCURACY                   8       /* control type: radioButton, callback function: Accuracy */
#define  PANEL_PWM                        9       /* control type: radioButton, callback function: pwm */
#define  PANEL_CROSSTALK                  10      /* control type: radioButton, callback function: Crosstalk */
#define  PANEL_GRAPH_DIGITAL              11      /* control type: graph, callback function: (none) */
#define  PANEL_LOAD_VFLR                  12      /* control type: command, callback function: LoadVFLR */
#define  PANEL_ZOOM_POSITION              13      /* control type: scale, callback function: ZoomPosition */
#define  PANEL_RUNALLTESTS                14      /* control type: command, callback function: RunAllTests */
#define  PANEL_ZOOM_START                 15      /* control type: string, callback function: (none) */
#define  PANEL_ZOOM_LENGTH                16      /* control type: string, callback function: (none) */
#define  PANEL_INIT_HARDWARE              17      /* control type: command, callback function: InitHardware */
#define  PANEL_PINTEST                    18      /* control type: radioButton, callback function: PinTest */
#define  PANEL_SERIAL_NUMBER              19      /* control type: string, callback function: (none) */


     /* Control Arrays: */

#define  CTRLARRAY                        1

     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK Accuracy(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Crosstalk(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK InitHardware(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK LoadVFLR(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Overshoot(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK panelCB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PinTest(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK pwm(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RunAllTests(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RunTest(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Runup(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ZoomPosition(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif