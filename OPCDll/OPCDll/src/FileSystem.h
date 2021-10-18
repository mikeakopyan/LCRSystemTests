
/*************************************************************************
*
* VULCANFORMS CONFIDENTIAL
* __________________
*  Copyright, VulcanForms Inc.
*  [2016] - [2020] VulcanForms Incorporated
*  All Rights Reserved.
*
*  "VulcanForms", "Vulcan", "Fusing the Future"
*       are trademarks of VulcanForms, Inc.
*
* NOTICE:  All information contained herein is, and remains
* the property of VulcanForms Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to VulcandForms Incorporated
* and its suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from VulcanForms Incorporated.
*/
#pragma once

#include <string>
#include <vector>


class DirectoryParser
{
public:
	static bool isValidDirectory(const std::string& sPath);

public:
	DirectoryParser();
	~DirectoryParser();
	
	bool setDirectoryPath(const std::string& path, const std::string& sExtension, const std::string& sKey = empty);

	bool getFile(std::string& filePath);
	bool getNextFile(std::string& filePath);

	static const std::string empty;
private:
	std::string mBasePath;
	std::vector<std::string> mFiles;
	unsigned int mnOnFile;
};

namespace filesystem
{

typedef int Handle;
bool createDirectory(const std::string& sPath);
void deleteDirectory(const std::string& sPath);

Handle createFile(const std::string& sFullName);
Handle appendFile(const std::string& sFullName);
Handle openFile(const std::string& sFullName);
unsigned int setAt(Handle& nHandle, unsigned int fileOffset, bool bFromEnd = false);
unsigned int getAt(Handle& nHandle);
bool readFile(Handle& nHandle, unsigned int nBytes, unsigned char* pBuffer);
void closeFile(Handle& nHandle);
bool copyFile(const std::string& srcPath, const std::string& destPath);
unsigned char* loadFile(const std::string& path, unsigned int& length);

}