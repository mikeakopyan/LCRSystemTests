#include <utility.h>
#include <string.h>
#include "..\..\OPCDll\OPCDll\OPCDll.h"
#include "toolbox.h"

#define FTP_CONFIG_FILE         "C:/Projects/LCRSystemTests/LCR_Hardware_Tests/push_config.xml";
#define LCR_HARDWARE_TEST_FILES "C:\\Projects\\LCRsystemTests\\LCRTestsFiles\\"

int sendToLCR(char *name)
{
	int ret = 0;
	//int handle;
	char path[1000];
	char *configFile = FTP_CONFIG_FILE;
	//strcpy(path,"C:\\Projects\\push\\bin\\push.exe -f C:\\Projects\\LCRTestsFiles\\");
	strcpy(path,LCR_HARDWARE_TEST_FILES);
	strcat(path,name);
	strcat(path,"\\*.vflr");
	ret = FTP_push(configFile,path);
	//ret =  LaunchExecutableEx (path, LE_SHOWMINIMIZED, &handle);
	Delay(2.0);	// Wait for ftp to complete
	return ret;
}

int sendFileToLCR(char *filePath)
{
	int ret = 0;
	//char path[1000];
	//size_t n=strlen(filePath);
	//size_t slashPos = -1;
	//for (size_t i=n-1; i>=0; i--)
	//{
	//	if (filePath[i]=='\\')
	//	{
	//		slashPos = i;
	//		break;
	//	}
	//}
	//if (slashPos<0)
	//	return -1;
	//for (int i=0; i<slashPos; i++)
	//	path[i] = filePath[i];
	//path[slashPos] = 0;
	//strcat(path,"\\*.vflr");
	//MessagePopup("Debug",path);
	char *configFile = FTP_CONFIG_FILE;
	ret = FTP_push(configFile,filePath);
	Delay(2.0);	// Wait for ftp to complete
	return ret;
}
