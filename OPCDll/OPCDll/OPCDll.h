#ifndef OPCDLL_INCLUDE
#define OPCDLL_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DllImport
#define DllImport   __declspec( dllimport )
#endif
	// Init library and connect to PLC
	// Return nonzero if connected
	DllImport int OPC_Init(const char* address);

	// Return state of Counter Reset button
	DllImport int OPC_CounterResetState(void);

	// Return state of Enable Pulse Generator button
	DllImport int OPC_EnablePulseGeneratorState(void);
	// Set state of Enable Pulse Generator button
	DllImport int OPC_SetEnablePulseGenerator(int state);

	// Set state of Enable Auto Mode button
	DllImport int OPC_SetCounterReset(int state);

	// Set encoder selection
	// 0 Y-axis, 1 - x-axis
	DllImport int OPC_SetEncoderSelection(int state);

	// Set encoder frequency
	// selection is index: 0 - 6 (0 - 3Hz, 6 - 600 KHz)
	DllImport int OPC_SetEncoderFrequency(int selection);

	// Return state of Enable Auto Mode button
	DllImport int OPC_EnableModeAutoState(void);
	// Set state of Enable Auto Mode button
	// After setting to 1 caller should set it back to 0
	DllImport int OPC_SetEnableModeAuto(int state);

	// Return state of Enable Manual Mode button
	DllImport int OPC_EnableModeManualState(void);
	// Set state of Enable Manual Mode button
	// After setting to 1 caller should set it back to 0
	DllImport int OPC_SetEnableModeManual(int state);

	// Set state of Modulation Enable button
	DllImport  int OPC_SetModulationEnable(int state);

	// Set selected print number
	DllImport int OPC_SetSelectedPrintNumber(int selection);

	// Set selected layer
	DllImport int OPC_SetSelectedLayer(int selection);

	// Set selected buld layout number
	DllImport int OPC_SetSelectedBuildLayout(int selection);

	// Open layer if selection = 1,
	// Close layer if selection = 0
	DllImport int OPC_SetOpenLayer(int selection);

	// Returns the human-readable name of the StatusCode
	DllImport  const char* OPC_Status_Name(int status);

	// Function to push files into LCR system
	// configFile - path to config file
	// filesPath - path to input folder with file type, i.e.
	// c:/temp/*.vflr
	// All files that match file type will be sent to LCR system
	// If filesPath does not have "*", only one file will be sent
	DllImport int FTP_push(const char* configFile, const char* filesPath);

#ifdef __cplusplus
}
#endif

#endif // ifndef OPCDLL_INCLUDE


