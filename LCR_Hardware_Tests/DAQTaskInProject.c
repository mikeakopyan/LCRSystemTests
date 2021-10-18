//**************************************************************************
//* WARNING: This file was automatically generated.  Any changes you make  *
//*          to this file will be lost if you generate the file again.     *
//**************************************************************************
#include <ansi_c.h>
#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( (DAQmxError=(functionCall))<0 ) goto Error; else

//**************************************************************************
//* This generated function configures your DAQmx task.                    *
//*                                                                        *
//* Follow these steps to use this generated function:                     *
//*   1) Define a task handle variable in your program.                    *
//*   2) Call the generated function.                                      *
//*   3) Use the returned task handle as a parameter to other DAQmx        *
//*      functions.                                                        *
//*   4) Clear the task handle when you finish.                            *
//*                                                                        *
//*         TaskHandle task = 0;                                           *
//*         CreateDAQTask(&task);                                          *
//*         <use the DAQmx task handle>                                    *
//*         DAQmxClearTask(task);                                          *
//*                                                                        *
//**************************************************************************
int32 CreateDAQTaskInProject(TaskHandle *taskOut1)
{
	int32 DAQmxError = DAQmxSuccess;
    TaskHandle taskOut;

	DAQmxErrChk(DAQmxCreateTask("DAQTaskInProject", &taskOut));

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai0", "Voltage_A0",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai1", "Voltage_A1",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai2", "Voltage_A2",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai3", "Voltage_A3",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai4", "Voltage_A4",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai5", "Voltage_A5",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai6", "Voltage_A6",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai7", "Voltage_A7",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai8", "Voltage_A8",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai9", "Voltage_A9",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai10", "Voltage_A10",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai11", "Voltage_A11",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai12", "Voltage_A12",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai13", "Voltage_A13",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai14", "Voltage_A14",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot2/ai15", "Voltage_A15",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot3/ai0", "Voltage_A16",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot3/ai1", "Voltage_A17",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot3/ai2", "Voltage_A18",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot3/ai3", "Voltage_A19",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCreateAIVoltageChan(taskOut, "PXI1Slot3/ai4", "Voltage_A20",
		DAQmx_Val_Diff, -10, 10, DAQmx_Val_Volts, ""));	

	DAQmxErrChk(DAQmxCfgSampClkTiming(taskOut, "", 
		1000000, DAQmx_Val_Rising, 
		DAQmx_Val_FiniteSamps, 1000));

	DAQmxErrChk(DAQmxCfgDigEdgeStartTrig(taskOut, "PFI0", DAQmx_Val_Falling));

    *taskOut1 = taskOut;

Error:
	return DAQmxError;
}

