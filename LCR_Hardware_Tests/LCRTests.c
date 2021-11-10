//==============================================================================
//
// Title:		LCRTest
// Purpose:		This application will test LCR system.
//
// Created on:	6/22/2021 at 1:28:24 PM by Mike Akopyan.
// Copyright:	VulcanForms Inc. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <reganlys.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "LCRTests.h"
#include "toolbox.h"

#include "DAQTaskInProject.h"
#include "DAQTaskInProject1.h"
#include <NIDAQmx.h>
#include "../OPCDll/OPCDll/OPCDll.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

extern int sendToLCR(char *name);
extern int sendFileToLCR(char *name);

//==============================================================================
// Constants

#define MAX_NUM_DISPLAY_CHANNELS 21
#define NUM_ANALOG_CHANNELS 21
#define NUM_ANALOG_CHANNELS_TO_CHECK 21
#define NUM_DIGITAL_CHANNELS_TO_CHECK 21
#define NUM_DIGITAL_CHANNELS 1
#define TEST_PLC_IP_ADDRESS "192.168.210.50:4840"

#define USE_PLC
#define USE_LCR
//==============================================================================
// Types

//==============================================================================
// Static global variables

static int PanelHandle = 0;

//static TaskHandle analogTask = 0;
//static TaskHandle digitalTask = 0;

static NumberOfAnalogSamples = 1000;
static float64 AnalogSampleRate = 1E6;	// 1 MHz is maximum

static NumberOfDigitalSamples = 300000;
static int DigitalSampleRate = 5000000;

const double EncoderFrequency = 150000;
	
//==============================================================================
// Static functions
int LoadDigitalData(const char *filePath);
int LoadAnalogData(const char *filePath);

//==============================================================================
// Global variables

static int analogPlotHandles[MAX_NUM_DISPLAY_CHANNELS];
static int digitalPlotHandles[MAX_NUM_DISPLAY_CHANNELS];


void WriteLog(const char *msg)
{
	time_t time_stamp = time(NULL);
	FILE *f = fopen("c:/Temp/LCRSystemTest.txt","a");
	if (f!=NULL) {
		fprintf(f,"%s %s",strtok(ctime(&time_stamp),"\n"),msg);
		fclose(f);
	}
	SetCtrlVal (PanelHandle, PANEL_RESULT, msg);	
}

//==============================================================================
// Global functions

/// HIFN The main entry-point function.
int main (int argc, char *argv[])
{
	int error = 0;
	
    //CreateDAQTaskInProject(&analogTask);	
	//CreateDAQTaskInProject1(&digitalTask);
	
	/* initialize and load resources */
	nullChk (InitCVIRTE (0, argv, 0));
	errChk (PanelHandle = LoadPanel (0, "LCRTests.uir", PANEL));


#ifdef USE_PLC
	if (!OPC_Init(TEST_PLC_IP_ADDRESS)) {
		MessagePopup("Cannot Init OPC",TEST_PLC_IP_ADDRESS);
		 goto Error;
	}

	// Prepare PLC for "open layer" command
    OPC_SetOpenLayer(0);

    OPC_SetEncoderFrequency(4);	// 15 KHz
    OPC_SetEncoderSelection(1);	// X

    OPC_SetSelectedBuildLayout(0);
    OPC_SetSelectedPrintNumber(0);
    OPC_SetSelectedLayer(1);

    OPC_SetCounterReset(1);
    OPC_SetEnablePulseGenerator(1);

	OPC_SetModulationEnable(1);
	
    // Set Auto mode
    OPC_SetEnableModeManual(0);
    OPC_SetEnableModeAuto(0);
	Delay(0.1);
    OPC_SetEnableModeAuto(1);
    Delay(0.1);
    OPC_SetEnableModeAuto(0);
#endif
	
	for (int i=0; i<MAX_NUM_DISPLAY_CHANNELS; i++)
	{
		analogPlotHandles[i] = 0;
		digitalPlotHandles[i] = 0;
	}
									 
	SetCtrlVal(PanelHandle,PANEL_RAMPUP,1);
	
	//LoadDigitalData("c:/Temp/DigitalData2.csv");
	//LoadAnalogData("c:/Temp/AnalogData2.csv");
	
	/* display the panel and run the user interface */
	errChk (DisplayPanel (PanelHandle));
	errChk (RunUserInterface ());

Error:
	/* clean up */
	if (PanelHandle > 0)
		DiscardPanel (PanelHandle);
	return 0;
}

//==============================================================================
// UI callback function prototypes

/// HIFN Exit when the user dismisses the panel.
int CVICALLBACK panelCB (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	if (event == EVENT_CLOSE) {
     	//DAQmxClearTask(analogTask);
		//DAQmxClearTask(digitalTask);
		QuitUserInterface (0);
	}
	return 0;
}

void lineFit(double *x, double *y, int n, double *slope, double *offset)
{
   double xsum=0,x2sum=0,ysum=0,xysum=0;
   for (int i=0; i<n; i++) {
    xsum=xsum+x[i];
    ysum=ysum+y[i];
    x2sum=x2sum+pow(x[i],2);
    xysum=xysum+x[i]*y[i];
   }
   *slope=(n*xysum-xsum*ysum)/(n*x2sum-xsum*xsum);
   *offset=(x2sum*ysum-xsum*xysum)/(x2sum*n-xsum*xsum);
}

void generateData(double *x, double slope, double offset, int nPoints, double *result)
{
	for (int i=0; i<nPoints; i++) {
		result[i] = offset + slope*x[i];
	}
}

int checkPWM(const uint32_t  *data, int size, int channel, float64 *maxTimeDeviation)
{
	uint32_t channelBit = 1 << channel;
	uint32_t encoderABit = 1 << 30;
	uint32_t encoderBBit = 1 << 29;
	int *encoderTransitions;
	int *channelTransitions;
	int *encoderCounts;
	int encoderTransitionsSize = 0;
	int chanelTransitionsSize = 0;
	int retValue = 0;
	float64 maxTimeDeviationLimit = 1.0;	// usec
	*maxTimeDeviation = 0.0;
	double encoderToIndex = 5E6/(4.0*150E3);	// Conversion from encoder counts to data index
	encoderTransitions = malloc(sizeof(int)*size/2);
	channelTransitions = malloc(sizeof(int)*size/2);
	encoderCounts = malloc(sizeof(int)*size);
	for (int i=1; i<size; i++) {
		if (     (data[i-1] & encoderABit) == 0 && (data[i] & encoderABit) != 0) {
			encoderTransitions[encoderTransitionsSize++] = i;
		}
		else if ((data[i-1] & encoderABit) != 0 && (data[i] & encoderABit) == 0) {
			encoderTransitions[encoderTransitionsSize++] = i;
		}		
		if (     (data[i-1] & encoderBBit) == 0 && (data[i] & encoderBBit) != 0) {
			encoderTransitions[encoderTransitionsSize++] = i;
		}
		else if ((data[i-1] & encoderBBit) != 0 && (data[i] & encoderBBit) == 0) {
			encoderTransitions[encoderTransitionsSize++] = i;
		}
		
		if ((data[i-1] & channelBit) == 0 && (data[i] & channelBit) != 0) {
			channelTransitions[chanelTransitionsSize++] = i;
		}
		else if ((data[i-1] & channelBit) != 0 && (data[i] & channelBit) == 0) {
			channelTransitions[chanelTransitionsSize++] = -i;
		}
		encoderCounts[i] = encoderTransitionsSize;
	}
	if (chanelTransitionsSize==0)
		retValue = 1;
	else {
		if (channelTransitions[0] < 200 * encoderToIndex) {	// There is command at e = 200 with D = 0. Should not generate pulses
			retValue = 2;
		}
		//else {
		//	FILE *f = fopen("c:/Temp/PWM.csv","w");
		//	for (int i=0; i<chanelTransitionsSize-1; i++) {
		//		int index = channelTransitions[i] > 0 ? channelTransitions[i] : -channelTransitions[i];
		//		int duty = -1;
		//		if (channelTransitions[i]>0) {
		//			duty = -channelTransitions[i+1] - channelTransitions[i];
		//		}
		//		fprintf(f,"%d,%d,%d\n",channelTransitions[i],encoderCounts[index],duty);
		//	}
		//	fclose(f);
		//}
	}

	// Check PWM turn on and PWM duty cycle
	int varyDutyMaxIndex = 0;
	for (int i=0; i<chanelTransitionsSize; i++) {
		if (encoderCounts[abs(channelTransitions[i])]>51290) {
			varyDutyMaxIndex = i;
			break;
		}
	}
	for (int i=1; i<256; i++) {
		int nominalEncoder = 100 + 200*i;
		BOOL found = FALSE;
		BOOL dutyOK = TRUE;
		for (int j=0; j<=varyDutyMaxIndex; j++) {
			if (channelTransitions[j]>0) {
				int val = abs(encoderCounts[channelTransitions[j]]-nominalEncoder);
				if (val<2) {
					found = TRUE;
					int duty = -channelTransitions[j+1] - channelTransitions[j];
					int nominalDuty = 5 * i;
					float dt = abs(duty - nominalDuty)/5.0;	// usec
					if (i!=255 && dt>*maxTimeDeviation)
						*maxTimeDeviation = dt;
					if (dt>maxTimeDeviationLimit) {
						if (i!=255)
							dutyOK = FALSE;
					}
					else {	// When Duty = 255, LCR output should stay longer
						if (i==255)
							dutyOK = FALSE;
					}
					break;
				}
			}
		}
		if (found==FALSE) {
			retValue = 3;
		}
		if (dutyOK==FALSE) {
			retValue = 4;
		}
	}
	// Check PWM period
	int varyPeriodMinIndex = 0;
	for (int i=0; i<chanelTransitionsSize; i++) {
		if (encoderCounts[abs(channelTransitions[i])]>=60000) {
			varyPeriodMinIndex = i;
			break;
		}
	}
	for (int i=1; i<256; i++) {
		int nominalEncoder = 60000 + 200*(i-1);
		BOOL found = FALSE;
		BOOL periodOK = TRUE;
		//FILE *ff = fopen("c:/Temp/debug.txt","w");
		for (int j=varyPeriodMinIndex; j<chanelTransitionsSize; j++) {
			if (channelTransitions[j]>0) {
				int val = abs(encoderCounts[channelTransitions[j]]-nominalEncoder);
				//fprintf(ff,"%d,%d\n",j,encoderCounts[channelTransitions[j]]-nominalEncoder);
				if (val<2) {
					found = TRUE;
					int onTime = -channelTransitions[j+1] - channelTransitions[j];
					int nominaPeriod = 5 * i;
					float nominalOnTime = nominaPeriod/2.0;
					float dt = abs(onTime - nominalOnTime)/5.0;	// usec
					//fprintf(ff,"%d vs %.1f\n",onTime, nominalOnTime);
					if (dt>*maxTimeDeviation)
						*maxTimeDeviation = dt;
					if (dt>maxTimeDeviationLimit) {
							periodOK = FALSE;
					}
					break;
				}
			}
		}
		//fclose(ff);
		if (found==FALSE) {
			retValue = 5;
		}
		if (periodOK==FALSE) {
			retValue = 6;
		}
	}	
	free(encoderTransitions);
	free(channelTransitions);
	free(encoderCounts);
	return retValue;
}

BOOL checkOvershoot(float64  *data, int size, float64 *overshoot)
{
	float64 maxOvershoot = 0.120;	// 120 mV
	float64 nominalValue = 10.0;	// Volts
	float64 maxV = 0.0;
	for (int i=0; i<size; i++) {
		if (data[i]>maxV)
			maxV = data[i];
	}
	*overshoot = maxV-nominalValue;
	if (*overshoot > maxOvershoot)
		return FALSE;
	return TRUE;
}

BOOL checkAccuracy(float64  *data, int size, float64 *deltaPlus, float64 *deltaMinus)
{
	float64 dPlus = 0.0;
	float64 dMinus = 0.0;
	float64 maxValue = 10.0;	// Volts
	const double encoderFrequency = 150000;
	double timeScale = (double)AnalogSampleRate/(4*encoderFrequency);		// To convert from encoder counts to analog data index
	int firstCenter = 750*timeScale;
	double timeStep = 500*timeScale * 19143.0/19162.0;	// PLC timing is not accurate?
	int avgSize = 20;
	double maxDeviation = 0.20; 	// Volts
	//FILE *f = fopen("c:/Temp/Indices.txt","w");
	for (int step=0; step<256; step++)
	{
		int center = firstCenter + step * timeStep;
		//fprintf(f,"%d\n",center);
		double avg = 0.0;
		int nAvg = 0;
		for (int i = center-avgSize/2; i<center+avgSize/2; i++)
		{
			avg += data[i];
			nAvg++;
		}

		avg /= nAvg;
		double nominal = maxValue/256.0 * step;
		double d = avg - nominal;
		if (d<0.0)
		{
			if (d<dMinus)
				dMinus = d;
		}
		else
		{
			if (d>dPlus)
				dPlus = d;
		}		
	}
	//fclose(f);
	*deltaPlus = dPlus;
	*deltaMinus = dMinus;
	if (dPlus>maxDeviation || -dMinus>maxDeviation)
		return FALSE;
	return TRUE;
}

BOOL checkCrosstalk(float64  *data, int size, float64 *minVal, float64 *maxVal)
{
	float64 maxRange = 0.13;	// 130 mV
	int upStart = -1;
	int upEnd = -1;
	for (int i=0; i<size; i++) {
		if (data[i]>1.0 && upStart<0) {
			upStart = i;
			break;
		}
	}
	for (int i=size-1; i>=0; i--) {
		if (data[i]>1.0 && upEnd<0) {
			upEnd = i;
			break;
		}
	}
	if (upStart<0 || upEnd<0)
		return FALSE;
	upStart -= 10;
	upEnd += 10;
	if (upStart<0)
		upStart = 0;
	if (upEnd>=size)
		upEnd = size-1;
	float64 min = 10.0;
	float64 max = -10.0;
	if (upStart>0) {
		for (int i=0; i<=upStart; i++) {
			if (data[i]<min)
				min = data[i];
			if (data[i]>max)
				max = data[i];
		}
	}
	if (upEnd<size-1) {
		for (int i=upEnd; i<size; i++) {
			if (data[i]<min)
				min = data[i];
			if (data[i]>max)
				max = data[i];
		}
	}
	*minVal = min;
	*maxVal = max;
	if (max>maxRange || -min>maxRange)
		return FALSE;
	else
		return TRUE;
}

void disableControlBoxes(int panel)
{
	int radioButtonsIDs[] = {PANEL_RAMPUP,PANEL_OVERSHOOT,PANEL_PWM,PANEL_CROSSTALK,PANEL_ACCURACY};
	for (int i=0; i<sizeof(radioButtonsIDs)/sizeof(int); i++) {
		SetInputMode(panel,radioButtonsIDs[i],0);
	}
}

void restCheckBoxes(int panel, int keepOn)
{
	int zero = 0;
	int radioButtonsIDs[] = {PANEL_RAMPUP,PANEL_OVERSHOOT,PANEL_PWM,PANEL_CROSSTALK,PANEL_ACCURACY};
	for (int i=0; i<sizeof(radioButtonsIDs)/sizeof(int); i++) {
		if (radioButtonsIDs[i] != keepOn)
			SetCtrlVal(panel,radioButtonsIDs[i],zero);
	}
}

void SaveAnalogData(const char *name, float64 *data, int size, int numChannels, int samplingRate)
{
	FILE *f = fopen(name,"wb");
	float dt = 1.0/(float)samplingRate;
	if (f!=0) {
		fwrite(&dt,sizeof(float),1,f);
		fwrite(data,sizeof(float64),size*numChannels,f);
		fclose(f);
	}
}

void SaveAnalogDataTxt(const char *name, float64 *data, int size, int numChannels, int samplingRate)
{
	FILE *f = fopen(name,"w");
	float dt = 1.0/(float)samplingRate;
	float time = 0.0f;
	if (f!=0) {
		for (int i=0; i<size; i++, time += dt) {
			fprintf(f,"%.4E",time);
			for (int chan=0; chan<numChannels; chan++)
				fprintf(f,",%.4E",data[size*chan+i]);
			fprintf(f,"\n");
		}
		fclose(f);
	}
}

void SaveDigitalPortData(const char *name, uint32_t *data, int size, int samplingRate)
{
	FILE *f = fopen(name,"wb");
	float dt = 1.0/(float)samplingRate;
	if (f!=0) {
		fwrite(&dt,sizeof(float),1,f);
		fwrite(data,sizeof(uint32_t),size,f);		
		fclose(f);
	}
}

void SaveDigitalPortDataTxt(const char *name, uint32_t *data, int size, int samplingRate)
{
	FILE *f = fopen(name,"w");
	float dt = 1.0/(float)samplingRate;
	float time = 0.0f;
	if (f!=0) {
		for (int i=0; i<size; i++, time += dt) {
			fprintf(f,"%.4E,%d",time,data[i]);
			fprintf(f,",%d",(data[i]>>30)&0x1);
			fprintf(f,",%d",(data[i]>>29)&0x1);
			for (int l=0; l<21; l++)
				fprintf(f,",%d",(data[i]>>l)&0x1);
		}
		fclose(f);
	}
}

void SaveDigitalData(uInt32 *digitalData, int size, int digitalSampleRate)
{
	FILE *f = fopen("C:/Temp/DigitalData.dat","wb");
	float dt = 1.0/(float)digitalSampleRate;
	if (f!=0) {
		fwrite(&dt,sizeof(float),1,f);
		fwrite(digitalData,sizeof(uInt32),size,f);
		fclose(f);
	}
}

int CVICALLBACK DataAcqAnalogThreadFunction (void *functionData);
static float64 *AnalogData = 0;

int CVICALLBACK DataAcqDigitalThreadFunction (void *functionData);
static uint32_t *DigitalPortData = 0;

int CVICALLBACK DataAcqAnalogThreadFunction (void *functionData)
{
	TaskHandle task = 0;
	char text[100];
	int error = 0;
	int read;
	error = CreateDAQTaskInProject(&task);
	if (error<0) {
		MessagePopup("Analog AcqTread","Create task failed");
		return error;
	}
	error = DAQmxCfgSampClkTiming(task, "", 
		AnalogSampleRate, DAQmx_Val_Rising, 
		DAQmx_Val_FiniteSamps, NumberOfAnalogSamples);
	if (error<0) {
		MessagePopup("Analog AcqTread","Set timing failed");
		return error;
	}
	error = DAQmxStartTask(task);
	if (error<0) {
		sprintf(text,"Start task error %d",error);
		MessagePopup("Analog AcqTread",text);
		return error;
	}
	error = DAQmxReadAnalogF64(task,NumberOfAnalogSamples,10.0,
											DAQmx_Val_GroupByChannel,
											AnalogData,NUM_ANALOG_CHANNELS*NumberOfAnalogSamples,
											&read,NULL);
	if (error<0) {
		sprintf(text,"Read data status %d",error);
		MessagePopup("Analog AcqThread",text);
	}
	DAQmxClearTask(task);

    return error;
}

int CVICALLBACK DataAcqDigitalThreadFunction (void *functionData)
{
	TaskHandle task = 0;
	char text[100];
	int error = 0;
	int read;

	error = CreateDAQTaskInProject1(&task);
	if (error<0) {
		MessagePopup("Digital AcqTread","Create task failed");
		return error;
	}
	if (error<0)
		return error;
	error = DAQmxCfgSampClkTiming(task, "", 
			DigitalSampleRate, DAQmx_Val_Rising, 
			DAQmx_Val_FiniteSamps, NumberOfDigitalSamples);
	if (error<0) {
		MessagePopup("Digital AcqTread","Set timing failed");
		return error;
	}
	error = DAQmxStartTask(task);
	if (error<0) {
		sprintf(text,"Start task error %d",error);
		MessagePopup("Digital AcqTread",text);
		return error;
	}
	error = DAQmxReadDigitalU32(task,NumberOfDigitalSamples,10.0,
												 DAQmx_Val_GroupByChannel,
												 DigitalPortData,NumberOfDigitalSamples,
												 &read,NULL);
	if (error<0) {
		sprintf(text,"Read data status %d. read: %d",error, read);
		MessagePopup("Digital AcqThread",text);
	}
	DAQmxClearTask(task);

    return error;
}

int CVICALLBACK QuitCallback (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			QuitUserInterface(0);
			break;
	}
	return 0;
}

int CVICALLBACK Runup (int panel, int control, int event,
					   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			restCheckBoxes(panel,PANEL_RAMPUP);
			break;
	}
	return 0;
}

int CVICALLBACK Overshoot (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			restCheckBoxes(panel,PANEL_OVERSHOOT);
			break;
	}
	return 0;
}

int CVICALLBACK Accuracy (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			restCheckBoxes(panel,PANEL_ACCURACY);
			break;
	}
	return 0;
}

int CVICALLBACK pwm (int panel, int control, int event,
					 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			restCheckBoxes(panel,PANEL_PWM);
			break;
	}
	return 0;
}

int CVICALLBACK Crosstalk (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			restCheckBoxes(panel,PANEL_CROSSTALK);
			break;
	}
	return 0;
}

int CVICALLBACK sampleRateChange (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int rateKHz;
			GetCtrlVal(panel, control, &rateKHz);
			//DigitalSampleRate = 1000 * rateKHz;
			break;
	}
	return 0;
}

void displayAnalogData(const double *data, int nSamples, int start, int end)
{
	const long	colors[] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN,
						VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE,
						VAL_DK_GREEN,VAL_DK_CYAN,VAL_DK_MAGENTA, VAL_DK_YELLOW,
						VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE}; // VAL_BLACK

	char plotName[100];
	static BOOL first = TRUE;
	int nPlots = NUM_ANALOG_CHANNELS;
	if (first)
	{
		first = FALSE;
		for (int i=0; i<MAX_NUM_DISPLAY_CHANNELS; i++)
			analogPlotHandles[i] = 0;
	}
	double xGain = 1E6/AnalogSampleRate;
	SetCtrlAttribute (PanelHandle, PANEL_GRAPH, ATTR_XAXIS_GAIN, xGain);
	double xOffset = 5*start;
	SetCtrlAttribute (PanelHandle, PANEL_GRAPH, ATTR_XAXIS_OFFSET, xOffset);
	for (int chan=0; chan<nPlots; chan++)
	{
		int index = chan % (sizeof(colors)/sizeof(long));
		long color = colors[index];
		if (analogPlotHandles[chan]!=0)
			DeleteGraphPlot(PanelHandle,PANEL_GRAPH,   analogPlotHandles[chan], VAL_IMMEDIATE_DRAW);
		analogPlotHandles[chan] = PlotY(PanelHandle, PANEL_GRAPH,
		  	&data[chan*nSamples+start],
		  	end-start+1, VAL_DOUBLE,
		  	VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1, color);
		sprintf(plotName,"Chan %d",chan+1);
		SetPlotAttribute (PanelHandle, PANEL_GRAPH, analogPlotHandles[chan],
						  ATTR_PLOT_LG_TEXT, plotName);	
	}
}

void enableZoom(BOOL enable)
{
	int iDs[] = {PANEL_ZOOM_START,PANEL_ZOOM_LENGTH,PANEL_ZOOM_POSITION};
	for (int i=0; i<sizeof(iDs)/sizeof(int); i++) {
		SetInputMode(PANEL,iDs[i],enable);
	}
}

void displayDigitalData(const uint32_t *data, int start, int end)
{
	const int digitalBits[] = {30,29,0,1};
	const char *legend[] = {"Encoder A","Encoder B","Laser 1","Laser 2"};
	static uint32_t bit[sizeof(digitalBits)/sizeof(int)];
	int nPlots = sizeof(digitalBits)/sizeof(int);
	long	colors[] = {VAL_RED, VAL_GREEN, VAL_CYAN,
						VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE,
						VAL_DK_GREEN,VAL_DK_CYAN,VAL_DK_MAGENTA, VAL_DK_YELLOW,
						VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE}; // VAL_BLACK
	int *digitalPortBits;
	int size = end - start + 1;
	double xOffset = start;
	
	for (int i=0; i<sizeof(digitalBits)/sizeof(int); i++)
		bit[i] = 1<<digitalBits[i];
	SetCtrlAttribute (PanelHandle, PANEL_GRAPH_DIGITAL, ATTR_XAXIS_OFFSET, xOffset);
	//SetCtrlAttribute (PanelHandle, PANEL_GRAPH_DIGITAL, ATTR_ENABLE_ZOOM_AND_PAN, 1);
	//SetCtrlAttribute (PanelHandle, PANEL_GRAPH_DIGITAL, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT); 
	digitalPortBits = malloc(size*sizeof(int));
	for (int i=0; i<sizeof(digitalBits)/sizeof(int); i++)
	{
		if (digitalPlotHandles[i]!=0)
			DeleteGraphPlot(PanelHandle,PANEL_GRAPH_DIGITAL,   digitalPlotHandles[i], VAL_IMMEDIATE_DRAW);
		int index = i % (sizeof(colors)/sizeof(long));
		long color = colors[index];
		for (int j=0; j<size; j++) {
			uint32_t d = data[j+start];
			int b = (d&bit[i])!=0 ? 1: 0;
			digitalPortBits[j] = b + (nPlots-1-i) * 3;
		}
		digitalPlotHandles[i] = PlotY(PanelHandle, PANEL_GRAPH_DIGITAL, &digitalPortBits[0],
			size,
			VAL_INTEGER, VAL_THIN_LINE, VAL_EMPTY_SQUARE,
			VAL_SOLID, 1, color);
		SetPlotAttribute (PanelHandle, PANEL_GRAPH_DIGITAL, digitalPlotHandles[i],
						  ATTR_PLOT_LG_TEXT, legend[i]);
	}
	free(digitalPortBits);
}

int LoadDigitalData(const char *filePath)
{
	float time;
	uint32_t datum;
	FILE *f = fopen(filePath,"r");
	if (f==0)
		return 1;
	int nLines = 0;
	while (fscanf(f,"%f,%d\n",&time,&datum)==2)
		nLines++;
	fseek(f,0,SEEK_SET);
	DigitalPortData = malloc(nLines*sizeof(uint32_t));
	for (int i=0; i<nLines; i++)
	{
		fscanf(f,"%f,%d\n",&time,&datum);
		DigitalPortData[i] = datum;
	}
	fclose(f);
	return 0;
}

int LoadAnalogData(const char *filePath)
{
	char line [1000];
	float time;
	float d[21];
	FILE *f = fopen(filePath,"r");
	if (f==0)
		return 1;
	int nLines = 0;
    while(fgets(line,sizeof line,f)!= NULL)
		nLines++;
	fseek(f,0,SEEK_SET);
	AnalogData = malloc(21*nLines*sizeof(float64));
	for (int i=0; i<nLines; i++)
	{
		fscanf(f,"%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
			   &time,&d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9],
								 &d[10],&d[11],&d[12],&d[13],&d[14],&d[15],&d[16],&d[17],&d[18],&d[19],
								 &d[20]);
		for (int chan=0; chan<21; chan++)
			AnalogData[nLines*chan + i] = d[chan];
	}
	fclose(f);
	return 0;
}

void setPartNumberEtc(const char *filePath)
{
	char str[100];
	char *token;
	char *fname;
	char *delim = "\\";
	int buildLayout;
	int printNumber;
	int layer;
	strcpy(str,filePath);
	fname = token = strtok(str, delim);
	if (fname == NULL)
		fname = str;
	else {
		while ((token=strtok(NULL,delim))!=NULL)
				fname = token;
	}
	fname[8] = 0;
	fname[13] = 0;
	fname[20] = 0;
	buildLayout = atoi(&fname[3]);
	printNumber = atoi(&fname[9]);
	layer = atoi(&fname[15]);
	
    OPC_SetSelectedBuildLayout(buildLayout);
    OPC_SetSelectedPrintNumber(printNumber);
    OPC_SetSelectedLayer(layer);
}

int loadAndRunVFLR(char *filePath, int nTrajectories)
{
	int opcStatus;
	int displayStart = 0;
	int displayEnd = 0;
	int analogThreadID = 0;
	int digitalThreadID = 0;
	int digitalToAnalogRateRatio = 5;
	NumberOfAnalogSamples = 2000000;
	NumberOfDigitalSamples = digitalToAnalogRateRatio*NumberOfAnalogSamples;
	AnalogSampleRate = 1E6;
	displayStart = 0;
	displayEnd = NumberOfAnalogSamples/100-1;
	enableZoom(TRUE);
	WriteLog("Sending file\n");
#ifdef USE_LCR
	int ftpError = sendFileToLCR(filePath);
	if (ftpError!=0)
	{
		char text[50];
		sprintf(text,"Error %d in sending file to LCR",ftpError);
		MessagePopup("FTP Error",text);
		return 1;
	}
#endif	
#ifdef USE_PLC
	setPartNumberEtc(filePath);
#endif
	if (AnalogData!=0)
		free(AnalogData);
	AnalogData = malloc(NUM_ANALOG_CHANNELS*NumberOfAnalogSamples*sizeof(float64));

	if (DigitalPortData!=0)
		free(DigitalPortData);
	DigitalPortData = malloc(NumberOfDigitalSamples*sizeof(uint32_t));

#ifdef USE_PLC
	OPC_SetOpenLayer(1);
#endif
	for (int trajectory=0; trajectory<nTrajectories; trajectory++)
	{
		char str[100];
		char analogFileName[200];
		char digitalFileName[200];
		sprintf(analogFileName,"C:/Temp/AnalogData%d.dat",trajectory);
		sprintf(digitalFileName,"C:/Temp/PortData%d.dat",trajectory);
		CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,
										DataAcqAnalogThreadFunction, NULL, &analogThreadID);
		CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,
							   			DataAcqDigitalThreadFunction, NULL, &digitalThreadID);

		Delay(0.2);
#ifdef USE_PLC
		for (int k = 0; k < 5; k++)
		{
			opcStatus = OPC_SetCounterReset(0);
			if (opcStatus==0)
				break;
			Delay(0.5);
			WriteLog("Counter reset retry to set to 0\n");
		}		
#endif
		sprintf(str,"Trajectory %d\n",trajectory);
		WriteLog(str);
		CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, analogThreadID, 0);
		SaveAnalogData(analogFileName,AnalogData,NumberOfAnalogSamples,NUM_ANALOG_CHANNELS,AnalogSampleRate);

		CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, digitalThreadID, 0);
		SaveDigitalPortData(digitalFileName,DigitalPortData,NumberOfDigitalSamples,DigitalSampleRate);
#ifdef USE_PLC
		for (int k = 0; k < 5; k++)
		{
			opcStatus = OPC_SetCounterReset(1);
			if (opcStatus==0)
				break;
			Delay(0.5);
			WriteLog("Counter reset retry to set to 1\n");
		}
#endif
		{
			displayAnalogData(AnalogData,NumberOfAnalogSamples,displayStart,displayEnd);
			displayDigitalData(DigitalPortData,
							   digitalToAnalogRateRatio*displayStart,
							   digitalToAnalogRateRatio*(displayEnd+1)-1);
		}
	}
	WriteLog("Test completed\n");
#ifdef USE_PLC	
	OPC_SetOpenLayer(0);
#endif
	return 0;
}

int readNumberOfTrajectories(char *filePath)
{
	//const uint8_t laserBit = 0b00000001;
    const uint8_t powerBit = 0b00000010;
    const uint8_t pwmDutyBit = 0b00000100;
    const uint8_t pwmPeridBit = 0b00001000;
    //const uint8_t SOTBit = 0b00010000;
    const uint8_t EOTBit = 0b00100000;
	int bytesToSkip;
	uint8_t format;
	uint8_t nBlocks;
	uint32_t blockOffset;
	uint32_t blockSize;
	uint8_t *lcr;
	int nT = 0;
	FILE *f = fopen(filePath,"rb");
	if (f<0)
		return -1;
	fseek(f,2000,SEEK_SET);	// Skip text header
	fread(&format,1,1,f);
	if (format!=1)
		return -2;
	fread(&nBlocks,1,1,f);
	if (nBlocks!=21)
		return -3;
	fread(&blockOffset,4,1,f);
	fread(&blockSize,4,1,f);
	fseek(f,(long)blockOffset,SEEK_SET);
	lcr = malloc((size_t)blockSize);
	fread(lcr,1,(size_t)blockSize,f);
	fclose(f);
	nT = 0;
	for (int i=0; i<blockSize; i++)
	{
		uint8_t command = *(lcr+i);
		bytesToSkip = 3; // skip encoder position
		if (command&powerBit)
			bytesToSkip++;
		if (command&pwmDutyBit)
			bytesToSkip++;
		if (command&pwmPeridBit)
			bytesToSkip++;
		if (command&EOTBit)
			nT++;
		i += bytesToSkip;
	}
	free(lcr);
	return nT;
}

int CVICALLBACK LoadVFLR (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	char filePath[1000];
	switch (event)
	{
		case EVENT_COMMIT:
			if (FileSelectPopup("c:/Temp","*.vflr","VFLR Files (*.vflr)","Select VFLR file",
								VAL_LOAD_BUTTON,0,0,1,0,filePath)==1)
			{
				int nT = readNumberOfTrajectories(filePath);
				if (nT<0)
				{
					MessagePopup("VFLR file","File format error");
					return 1;
				}
				loadAndRunVFLR(filePath,nT);
			}
			break;
	}
	return 0;
}

int CVICALLBACK ZoomPosition (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	uint32_t start;
	char zoomStartTxt[20];
	char zoomLengthTxt[20];

	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel, PANEL_ZOOM_START, zoomStartTxt);
			GetCtrlVal(panel, PANEL_ZOOM_LENGTH, zoomLengthTxt);
			int zoomStart = atoi(zoomStartTxt);
			int zoomLength = atoi(zoomLengthTxt);
			GetCtrlVal(panel, control, &start);
			start = zoomStart + zoomLength*start/1000;
			start = 5 * (start/5);
			int displaySize = zoomLength;
			if (DigitalPortData!=0) {
				//char txt[60];
				//sprintf(txt,"%d\n",start);
				//WriteLog(txt);
				displayDigitalData(DigitalPortData,start,start+displaySize-1);
			}
			if (AnalogData!=0) {
				displayAnalogData(AnalogData,NumberOfAnalogSamples,start/5,(start+displaySize)/5-1);			
			}
			break;
	}
	return 0;
}

static int DisplayStart = 0;
static int DisplayEnd = 999;
static int testID = -1;
static int ftpError = 0;
static BOOL	useAnalogTask = FALSE;
static BOOL useDigitalTask = FALSE;
static BOOL readDigitalPort = FALSE;
static int analogThreadID = 0;
static int digitalThreadID = 0;
static 	int radioButtonValue;

int ExecuteTest(void)
{
	double	slopes[NUM_ANALOG_CHANNELS];
	double  offsets[NUM_ANALOG_CHANNELS];
	double  *time = 0;
	double  *fitResult = 0;
	double  *xResult = 0;
	int 	nPointsToUse[NUM_ANALOG_CHANNELS] = {NUM_ANALOG_CHANNELS*0};
	int		fitStart[NUM_ANALOG_CHANNELS] = {NUM_ANALOG_CHANNELS*0};
	long		colors[] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN,
						VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE,
						VAL_DK_GREEN,VAL_DK_CYAN,VAL_DK_MAGENTA, VAL_DK_YELLOW,
						VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE}; // VAL_BLACK
	char 	txtResults[2000];
	char    txtOneResult[100];
	BOOL fitLine = FALSE;
	int testResult = 0;

	char errorMsg[100];
	strcpy(errorMsg,"");
	enableZoom(FALSE);
	if (useAnalogTask) {
		if (AnalogData!=NULL)
			free(AnalogData);
		AnalogData = malloc(NUM_ANALOG_CHANNELS*NumberOfAnalogSamples*sizeof(float64));
		memset(AnalogData,0,NUM_ANALOG_CHANNELS*NumberOfAnalogSamples*sizeof(float64));
		WriteLog("Start Analog acq thread\n");
		CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,
									   DataAcqAnalogThreadFunction, NULL, &analogThreadID);
		if (readDigitalPort) {
			if (DigitalPortData!=NULL)
				free(DigitalPortData);
			DigitalPortData = malloc(NumberOfDigitalSamples*sizeof(uint32_t));
			memset(DigitalPortData,0xAA,NumberOfDigitalSamples*sizeof(uint32_t));
			WriteLog("Start Digital acq thread\n");
			CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,
									   DataAcqDigitalThreadFunction, NULL, &digitalThreadID);
		}
	}
	if (useDigitalTask) {
		NumberOfDigitalSamples = 1000000;
		if (DigitalPortData!=NULL)
			free(DigitalPortData);
		DigitalPortData = malloc(NumberOfDigitalSamples*sizeof(uint32_t));
		memset(DigitalPortData,0xAA,NumberOfDigitalSamples*sizeof(uint32_t));
		WriteLog("Start Digital acq thread\n");
		CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,
									   DataAcqDigitalThreadFunction, NULL, &digitalThreadID);
	}
	
#ifdef USE_PLC
	Delay(1.0);
	WriteLog("Open Layer\n");
	OPC_SetOpenLayer(1);
	Delay(1.0);
	WriteLog("Release Counter Reset\n");
	OPC_SetCounterReset(0);
	Delay(0.1);
#endif
	double xGain = 1E6/AnalogSampleRate;
	SetCtrlAttribute (PanelHandle, PANEL_GRAPH, ATTR_XAXIS_GAIN, xGain);

	time = malloc(NumberOfAnalogSamples*sizeof(double));
	fitResult = malloc(NumberOfAnalogSamples*sizeof(double));
	xResult = malloc(NumberOfAnalogSamples*sizeof(double));
	for (int i=0; i<NumberOfAnalogSamples; i++)
		time[i] = (1.0/AnalogSampleRate)*i;

	if (useAnalogTask) {
		double timeScale = (double)AnalogSampleRate/(4*EncoderFrequency);
		const char *analogFileName = "C:/Temp/AnalogData.csv";
		const char *digitalFileName = "C:/Temp/PortData.csv";
		CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, analogThreadID, 0);
		WriteLog("Saving analog data\n");
		SaveAnalogData(analogFileName,AnalogData,NumberOfAnalogSamples,NUM_ANALOG_CHANNELS,AnalogSampleRate);
		if (readDigitalPort) {
			CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, digitalThreadID, 0);
			WriteLog("Saving digital data\n");
			SaveDigitalPortDataTxt(digitalFileName,DigitalPortData,NumberOfDigitalSamples,DigitalSampleRate);
		}
		strcpy(txtResults,"");

		if (testID == PANEL_CROSSTALK)
			strcpy(txtResults,"Crosstalk\n");
		if (testID == PANEL_OVERSHOOT)
			strcpy(txtResults,"Overshoot\n");
		if (testID == PANEL_RAMPUP) {
			fitLine = TRUE;
			for (int i=0; i<NUM_ANALOG_CHANNELS_TO_CHECK; i++) {
				fitStart[i] = timeScale * (500 * i + 510 + 200);
				nPointsToUse[i] = timeScale * (2550 - 400);
			}
		}
		for (int chan=0; chan<NUM_ANALOG_CHANNELS_TO_CHECK; chan++) {
			char plotName[20];
			if (fitLine) {
				lineFit(&time[fitStart[chan]],
						&AnalogData[chan*NumberOfAnalogSamples+fitStart[chan]],
						nPointsToUse[chan],&slopes[chan],&offsets[chan]);
				generateData(&time[fitStart[chan]],
							 slopes[chan],offsets[chan],nPointsToUse[chan],fitResult);
				for (int i=0; i<nPointsToUse[chan]; i++)
					xResult[i] = (1E6/AnalogSampleRate) * (fitStart[chan] + i);
				if (testID == PANEL_RAMPUP) {
					double nominalSlope = 2330;
					double slopeTolerance = 100;
					if (fabs(slopes[chan]-nominalSlope)>slopeTolerance) {
						testResult = 1;
						sprintf(txtOneResult,"Chan # %d Slope: %.2f FAILED\n",chan+1,slopes[chan]);
						strcat(txtResults,txtOneResult);
					}
				}
			}
			int index = chan % (sizeof(colors)/sizeof(long));
			long color = colors[index];
			if (analogPlotHandles[chan]!=0)
				DeleteGraphPlot(PanelHandle,PANEL_GRAPH,   analogPlotHandles[chan], VAL_IMMEDIATE_DRAW);
			analogPlotHandles[chan] = PlotY(PanelHandle, PANEL_GRAPH,
				  	&AnalogData[chan*NumberOfAnalogSamples+DisplayStart],
				  	DisplayEnd-DisplayStart+1, VAL_DOUBLE,
				  	VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1, color);
			sprintf(plotName,"Chan %d",chan+1);
			SetPlotAttribute (PanelHandle, PANEL_GRAPH, analogPlotHandles[chan],
					  ATTR_PLOT_LG_TEXT, plotName);
			if (testID == PANEL_CROSSTALK) {
				BOOL pass;
				float64 minVal;
				float64 maxVal;
				pass = checkCrosstalk(&AnalogData[chan*NumberOfAnalogSamples],
											NumberOfAnalogSamples,
											&minVal,&maxVal);
				if (pass)
					sprintf(txtOneResult,"%.1f - %.1f\n",1000.0*minVal,1000.0*maxVal);
				else {
					sprintf(txtOneResult,"%.1f - %.1f FAIL\n",1000.0*minVal,1000.0*maxVal);
					testResult = 1;
				}
				strcat(txtResults,txtOneResult);				
			}
			if (testID == PANEL_OVERSHOOT) {
				BOOL pass;
				float64 overshoot;
				pass = checkOvershoot(&AnalogData[chan*NumberOfAnalogSamples],
											NumberOfAnalogSamples,
											&overshoot);
				if (pass)
					sprintf(txtOneResult,"%.1f\n",1000.0*overshoot);
				else {
					sprintf(txtOneResult,"%.1f FAIL\n",1000.0*overshoot);
					testResult = 1;
				}
				strcat(txtResults,txtOneResult);
			}
			if (testID == PANEL_ACCURACY) {
				BOOL pass;
				float64 deltaPlus,deltaMinus;
				pass = checkAccuracy(&AnalogData[chan*NumberOfAnalogSamples],
											NumberOfAnalogSamples,
											&deltaPlus,&deltaMinus);
				if (pass)
					sprintf(txtOneResult,"%.3f %.3f\n",deltaMinus,deltaPlus);
				else {
					sprintf(txtOneResult,"%.3f %.3f FAIL\n",deltaMinus,deltaPlus);
					testResult = 1;
				}
				strcat(txtResults,txtOneResult);
			}
		    //if (fitLine) {
			//	PlotXY(PanelHandle, PANEL_GRAPH,xResult,fitResult,
			//		   nPointsToUse[chan], VAL_DOUBLE, VAL_DOUBLE,
			//		   VAL_THIN_LINE, VAL_EMPTY_SQUARE,
			//		   VAL_SOLID, 1, VAL_YELLOW);
			//}
		}
		if (readDigitalPort) {
			PlotY(PanelHandle, PANEL_GRAPH_DIGITAL, &DigitalPortData[0],
				   NumberOfDigitalSamples, VAL_INTEGER, VAL_THIN_LINE, VAL_EMPTY_SQUARE,
				   VAL_SOLID, 1, VAL_RED);
		}
		if (strlen(txtResults)>0)
			WriteLog(txtResults);
	}
	if (useDigitalTask) {
		CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, digitalThreadID, 0);
		WriteLog("Saving digital port data\n");
		SaveDigitalPortDataTxt("C:/Temp/PortData.csv",DigitalPortData,NumberOfDigitalSamples,DigitalSampleRate);
		strcpy(txtResults,"");
		if (testID == PANEL_PWM) {
			for (int chan=0; chan<NUM_DIGITAL_CHANNELS_TO_CHECK; chan++) {
				strcpy(txtOneResult,"");
				int error;
				float64 maxTimeDeviation;
				error = checkPWM(&DigitalPortData[0],
											NumberOfDigitalSamples,
											chan,
											&maxTimeDeviation);
				//if (error==0)
				//	sprintf(txtOneResult,"%.1f usec\n",maxTimeDeviation);
				if (error!=0) {
					sprintf(txtOneResult,"%.1f usec - FAIL CODE: %d\n",maxTimeDeviation,error);
					testResult = 1;
				}
				strcat(txtResults,txtOneResult);
			}
		}
		if (strlen(txtResults)>0)
			WriteLog(txtResults);
		//for (int i=0; i<1; i++) {
		//	if (digitalPlotHandles[i]!=0)
		//		DeleteGraphPlot(PanelHandle,PANEL_GRAPH_DIGITAL, digitalPlotHandles[i], VAL_IMMEDIATE_DRAW);
		//	digitalPlotHandles[i] = PlotY(PanelHandle, PANEL_GRAPH_DIGITAL, &DigitalPortData[0],
		//		   NUMBER_OF_DIGITAL_SAMPLES, VAL_INTEGER, VAL_THIN_LINE, VAL_EMPTY_SQUARE,
		//		   VAL_SOLID, 1, VAL_RED);
		//}
		displayDigitalData(DigitalPortData,0,5*(DisplayEnd+1)-1);
	}
	free(time);
	free(fitResult);
	free(xResult);

	Delay(0.2);
#ifdef USE_PLC	
	OPC_SetCounterReset(1);
	Delay(1.0);
	OPC_SetOpenLayer(0);
#endif
	if (testResult == 0)
		WriteLog("Test passed\n");
	else
		WriteLog("Test failed\n");
	return testResult;
}

int SendFileToLCR(const char *testName)
{
	ftpError = sendToLCR(testName);
	if (ftpError!=0)
	{
		char text[50];
		sprintf(text,"Error %d in sending file to LCR",ftpError);
		MessagePopup("FTP Error",text);
	}
	return ftpError;
}

int RampupTest(void)
{
	testID = PANEL_RAMPUP;
	NumberOfAnalogSamples = 60000;
	AnalogSampleRate = 1E6;
	DisplayStart = 0;
	DisplayEnd = NumberOfAnalogSamples/2-1;
	WriteLog("Start Rampup test\n");
	WriteLog("Sending file\n");
	useAnalogTask = TRUE;
	useDigitalTask = FALSE;
	readDigitalPort = FALSE;
#ifdef USE_LCR
	if (SendFileToLCR("Rampup")!=0)
		return 1;
#endif
	return ExecuteTest();
}

int OvershootTest(void)
{
	NumberOfAnalogSamples = 1000;
	testID = PANEL_OVERSHOOT;
	AnalogSampleRate = 1E6;
	DisplayStart = 265;
	DisplayEnd = 615;
	WriteLog("Start Overshoot test\n");
	WriteLog("Sending file\n");
	useAnalogTask = TRUE;
	useDigitalTask = FALSE;
	readDigitalPort = FALSE;
#ifdef USE_LCR
	if (SendFileToLCR("Overshoot")!=0)
		return 1;
#endif
	return ExecuteTest();
}

int PwmTest(void)
{
	testID = PANEL_PWM;
	DisplayStart = 0;
	DisplayEnd = 2000-1;
	WriteLog("Start PWM test\n");
	WriteLog("Sending file\n");
	useAnalogTask = FALSE;
	useDigitalTask = TRUE;
	readDigitalPort = FALSE;
#ifdef USE_LCR
	if (SendFileToLCR("Pwm")!=0)
		return 1;
#endif
	return ExecuteTest();
}

int AnalogAccuracyTest(void)
{
	testID = PANEL_ACCURACY;
	NumberOfAnalogSamples = 20000;
	AnalogSampleRate = 90000;
	DisplayStart = 0;
	DisplayEnd = NumberOfAnalogSamples-1;
	WriteLog("Start Accuracy test\n");
	WriteLog("Sending file\n");
	useAnalogTask = TRUE;
	useDigitalTask = FALSE;
	readDigitalPort = FALSE;
#ifdef USE_LCR
	if (SendFileToLCR("Accuracy")!=0)
		return 1;
#endif
	return ExecuteTest();
}

int CrosstalkTest(void)
{
	testID = PANEL_CROSSTALK;
	NumberOfAnalogSamples = 8000;
	AnalogSampleRate = 1E5;
	DisplayStart = 0;
	DisplayEnd = NumberOfAnalogSamples-1;
	WriteLog("Start Crosstalk test\n");
	WriteLog("Sending file\n");
	useAnalogTask = TRUE;
	useDigitalTask = FALSE;
	readDigitalPort = FALSE;
#ifdef USE_LCR
	if (SendFileToLCR("Crosstalk")!=0)
		return 1;
#endif
	return ExecuteTest();
}

int CVICALLBACK RunTest (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:

		GetCtrlVal(panel, PANEL_RAMPUP, &radioButtonValue);		
		if (radioButtonValue==1) {
			RampupTest();
		}
		GetCtrlVal(panel, PANEL_OVERSHOOT, &radioButtonValue);		
		if (radioButtonValue==1) {
			OvershootTest();
		}
		GetCtrlVal(panel, PANEL_PWM, &radioButtonValue);		
		if (radioButtonValue==1) {
			PwmTest();
		}
		GetCtrlVal(panel, PANEL_ACCURACY, &radioButtonValue);		
		if (radioButtonValue==1) {
			AnalogAccuracyTest();
		}
		GetCtrlVal(panel, PANEL_CROSSTALK, &radioButtonValue);		
		if (radioButtonValue==1) {
			CrosstalkTest();
		}
	}			
	return 0;
}

int CVICALLBACK RunAllTests (int panel, int control, int event,
							 void *callbackData, int eventData1, int eventData2)
{
	int radioButtonsIDs[] = {PANEL_RAMPUP, PANEL_OVERSHOOT, PANEL_ACCURACY,      PANEL_PWM, PANEL_CROSSTALK};
	int (*fun_ptr[]) () =   {&RampupTest,   &OvershootTest,  &AnalogAccuracyTest, &PwmTest,  &CrosstalkTest,};
	switch (event)
	{
		case EVENT_COMMIT:
			for (int i=0; i<sizeof(radioButtonsIDs)/sizeof(int); i++) {
				SetCtrlVal(panel,radioButtonsIDs[i],0);
			}	
			for (int test = 0; test<sizeof(radioButtonsIDs)/sizeof(int); test++)
			{
				SetCtrlVal(panel,radioButtonsIDs[test],1);
				if (test>0)
					SetCtrlVal(panel,radioButtonsIDs[test-1],0);
				if (fun_ptr[test]()!=0)
					MessagePopup("Test Failed","Sequence will be aborted");;
			}
			break;
	}
	return 0;
}
