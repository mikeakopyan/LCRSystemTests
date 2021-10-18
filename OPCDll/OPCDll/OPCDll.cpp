#include <stdlib.h>
#include <stdio.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <open62541.h>

#define DllImport   __declspec( dllexport )

extern int push(const char* configFile, const char* filesPath);

UA_Client* client;
UA_ClientConfig* puaconfig;
std::string ip;
//std::string OPCServer("192.168.210.62:4840");
bool opcConnected = false;

#include "OPCDll.h"
#ifdef __cplusplus
extern "C" {
#endif

    bool reconnectOPCUA(UA_ClientConfig& UAConfig, UA_Client& UAClient, std::string& ip);

	int OPC_Init(const char* address)
	{
        client = UA_Client_new();
        puaconfig = UA_Client_getConfig(client);
        ip = "opc.tcp://" + std::string(address);

        // Andy Nonymous  Look but no touch, ugh, breaking the rules and touching
        UA_ClientConfig_setDefault(puaconfig);
        UA_StatusCode retvalConnection = UA_Client_connect(client, ip.c_str());

        /* Attempted connection to a server */
        if (retvalConnection != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            opcConnected = false;
        }
        else {
            opcConnected = true;
        }
        return (int)opcConnected;
	}

    bool OPC_GetBooleanValue(const char* nodeName)
    {
        char name[200];
        strcpy_s(name, nodeName);
        bool bValue = false;
        UA_StatusCode retval = 0;
        UA_Variant* pVal = UA_Variant_new();
        retval = UA_Client_readValueAttribute(client,
            UA_NODEID_STRING(6, &name[0]),
            pVal);
        if (retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(pVal)
            && pVal->type == &UA_TYPES[UA_TYPES_BOOLEAN])
        {
            memcpy(&bValue, pVal->data, sizeof(boolean));
        }
        else if (UA_STATUSCODE_BADINTERNALERROR == retval) {
            bool bConnected = reconnectOPCUA(*puaconfig, *client, ip);
            if (bConnected) {
                std::this_thread::sleep_for(std::chrono::microseconds(550));
            }
            else {
                UA_Client_disconnect(client);
                UA_Client_delete(client);
            }
        }
        return bValue;
    }


    int OPC_SetBooleanValue(const char* nodeName, bool value)
    {
        char name[200];
        strcpy_s(name, nodeName);
        UA_DataType boolDataType = UA_TYPES[UA_TYPES_BOOLEAN];
        UA_StatusCode retval = 0;
        UA_Variant* pNewVal = UA_Variant_new();
        UA_Variant_setScalar(pNewVal, &value, &boolDataType);
        memcpy(pNewVal->data, &value, sizeof(boolean));
        retval = UA_Client_writeValueAttribute(client,
            UA_NODEID_STRING(6, &name[0]),
            pNewVal);
        return retval;
    }

    int OPC_SetByteValue(const char* nodeName, uint8_t value)
    {
        char name[200];
        strcpy_s(name, nodeName);
        UA_DataType byteDataType = UA_TYPES[UA_TYPES_BYTE];
        UA_StatusCode retval = 0;
        UA_Variant* pNewVal = UA_Variant_new();
        UA_Variant_setScalar(pNewVal, &value, &byteDataType);
        memcpy(pNewVal->data, &value, sizeof(uint8_t));
        retval = UA_Client_writeValueAttribute(client,
            UA_NODEID_STRING(6, &name[0]),
            pNewVal);
        return retval;
    }

    int OPC_SetUInt16Value(const char* nodeName, uint16_t value)
    {
        char name[200];
        strcpy_s(name, nodeName);
        UA_DataType uint16DataType = UA_TYPES[UA_TYPES_UINT16];
        UA_StatusCode retval = 0;
        UA_Variant* pNewVal = UA_Variant_new();
        UA_Variant_setScalar(pNewVal, &value, &uint16DataType);
        memcpy(pNewVal->data, &value, sizeof(uint16_t));
        retval = UA_Client_writeValueAttribute(client,
            UA_NODEID_STRING(6, &name[0]),
            pNewVal);
        return retval;
    }

    int OPC_SetUInt32Value(const char* nodeName, uint32_t value)
    {
        char name[200];
        strcpy_s(name, nodeName);
        UA_DataType uint32DataType = UA_TYPES[UA_TYPES_UINT32];
        UA_StatusCode retval = 0;
        UA_Variant* pNewVal = UA_Variant_new();
        UA_Variant_setScalar(pNewVal, &value, &uint32DataType);
        memcpy(pNewVal->data, &value, sizeof(uint32_t));
        retval = UA_Client_writeValueAttribute(client,
            UA_NODEID_STRING(6, &name[0]),
            pNewVal);
        return retval;
    }

    int OPC_EnableModeAutoState(void)
    {
        bool bValue = OPC_GetBooleanValue("::AsGlobalPV:LCR_OpcData_ToLCR.EnableMode_AUTO");
        return bValue ? 1 : 0;
    }

    int OPC_EnableModeManualState(void)
    {
        bool bValue = OPC_GetBooleanValue("::AsGlobalPV:LCR_OpcData_ToLCR.EnableMode_MANUAL");
        return bValue ? 1 : 0;
    }

    int OPC_EnablePulseGeneratorState(void)
    {
        bool bValue = OPC_GetBooleanValue("::AsGlobalPV:MoveEnable");
        return bValue ? 1 : 0;
    }
    
    int OPC_SetEnablePulseGenerator(int state)
    {
        return OPC_SetBooleanValue("::AsGlobalPV:MoveEnable",
            state != 0);
    }

    int OPC_CounterResetState(void)
    {
        bool bValue = OPC_GetBooleanValue("::AsGlobalPV:CounterReset");
        return bValue ? 1 : 0;
    }

    int OPC_SetEnableModeAuto(int state)
    {
        return OPC_SetBooleanValue("::AsGlobalPV:LCR_OpcData_ToLCR.EnableMode_AUTO",
            state != 0);
    }

    int OPC_SetEnableModeManual(int state)
    {
        return OPC_SetBooleanValue("::AsGlobalPV:LCR_OpcData_ToLCR.EnableMode_MANUAL",
            state != 0);
    }

    int OPC_SetModulationEnable(int state)
    {
        return OPC_SetBooleanValue("::AsGlobalPV:ModulationEnable",
            state != 0);
    }

    int OPC_SetSelectedPrintNumber(int selection)
    {
        return OPC_SetUInt16Value("::AsGlobalPV:LCR_OpcData_ToLCR.SelectedPrintNumber", (uint16_t)selection);
    }

    int OPC_SetSelectedLayer(int selection)
    {
        return OPC_SetUInt32Value("::AsGlobalPV:LCR_OpcData_ToLCR.SelectedLayer", (uint32_t)selection);
    }

    int OPC_SetSelectedBuildLayout(int selection)
    {
        return OPC_SetUInt16Value("::AsGlobalPV:LCR_OpcData_ToLCR.SelectedBuildLayout", (uint32_t)selection);
    }

    int OPC_SetEncoderFrequency(int selection)
    {
        return OPC_SetByteValue("::AsGlobalPV:FrequencySelection", (uint8_t)selection);
    }

    int OPC_SetEncoderSelection(int state)
    {
        return OPC_SetBooleanValue("::AsGlobalPV:EncoderSelection",
            state != 0);
    }

    int OPC_SetCounterReset(int state)
    {
        return OPC_SetBooleanValue("::AsGlobalPV:CounterReset",
            state != 0);
    }

	int OPC_SetOpenLayer(int selection)
	{
        return OPC_SetBooleanValue("::AsGlobalPV:LCR_OpcData_ToLCR.OpenLayer",
            selection!=0);
	}

    bool reconnectOPCUA(UA_ClientConfig& UAConfig, UA_Client& UAClient, std::string& ip)
    {
        bool bReconnected = false;
        UA_Client_disconnect(&UAClient);
        std::this_thread::sleep_for(std::chrono::microseconds(850));
        UA_ClientConfig_setDefault(&UAConfig);
        UA_StatusCode retval = UA_Client_connect(&UAClient, ip.c_str());
        /* Attempted re-connection to a server */
        if (retval == UA_STATUSCODE_GOOD) {
            bReconnected = true;
        }
        return bReconnected;
    }

    int FTP_push(const char* configFile, const char* filesPath)
    {
        return push(configFile, filesPath);
    }
#ifdef __cplusplus
}
#endif