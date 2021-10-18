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

#include "FileSystem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <algorithm>
#include <iostream>
#include <ostream>
#include <fstream>
#include <filesystem>
#include <string.h>

namespace fs = std::filesystem;

bool DirectoryParser::isValidDirectory(const std::string& sPath)
{
    fs::path fsPath = sPath;
    std::error_code ec;
    bool bGood = fs::is_directory(fsPath, ec);
    return bGood;
}

const std::string DirectoryParser::empty = {};
DirectoryParser::DirectoryParser() : mnOnFile(0)
{

}
DirectoryParser::~DirectoryParser()
{

}



bool DirectoryParser::setDirectoryPath(const std::string& sPath, const std::string& sExtension, const std::string& sKey)
{
    
    mBasePath = sPath;
    mFiles.clear();
    fs::path fsPath = sPath;
    std::error_code ec;
    bool bGood = fs::is_directory(fsPath, ec);
    if (bGood) {
        for (auto it = fs::directory_iterator(fsPath); it != fs::directory_iterator(); ++it) {
            if (fs::exists(*it)) {
                const std::filesystem::path path = it->path();

                std::string sName = path.string();
				if (true != sKey.empty()) {
					if (std::string::npos != sName.find(sExtension) && std::string::npos != sName.find(sKey))
						mFiles.push_back(sName);
				}
                else if (std::string::npos != sName.find(sExtension))
                    mFiles.push_back(sName);
            }
        }
        mnOnFile = 0;
        bGood = mFiles.size() > 0;
    }

    return bGood;
}

bool DirectoryParser::getFile(std::string& filePath)
{
    bool bHasFile = false;
    if (mFiles.size() >= 1) {
        mnOnFile = 0;
        filePath = mFiles[mnOnFile++];
        bHasFile = true;
    }
    return bHasFile;
}
bool DirectoryParser::getNextFile(std::string& filePath)
{
    bool bHasFile = (mnOnFile < mFiles.size());
    if (bHasFile) {
        filePath = mFiles[mnOnFile];
        mnOnFile += 1;
    }
    return bHasFile;
}

#include <cstddef>



namespace filesystem
{
    bool createDirectory(const std::string& sPath)
    {
        std::error_code ec;
        fs::path dirn(sPath);
        bool bIsDir = fs::is_directory(dirn, ec);
        if (ec.value() != 0) {
            fs::create_directory(dirn);
        }
        return true;
    }
    void deleteDirectory(const std::string& sPath)
    {
        std::error_code ec;
        fs::path dirn(sPath);
        fs::remove_all(dirn, ec);
    }
    Handle createFile(const std::string& sFullName)
{
    Handle nHandle;
    int nErr = _sopen_s(&nHandle, sFullName.c_str(), O_CREAT | O_WRONLY | O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (0 != nErr) {
        nHandle = -1;
    }
    return nHandle;
}

Handle appendFile(const std::string& sFullName)
{
    Handle nHandle;
    int nErr = _sopen_s(&nHandle, sFullName.c_str(), O_APPEND | O_WRONLY | O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (0 != nErr) {
        nHandle = -1;
    }
    return nHandle;
}

Handle openFile(const std::string& sFullName)
{
    Handle nHandle;
    int nErr = _sopen_s(&nHandle, sFullName.c_str(), O_RDWR | O_BINARY, _SH_DENYNO, _S_IREAD);
    if (0 != nErr) {
        nHandle = -1;
    }
    return nHandle;
}

unsigned int setAt(Handle& nHandle, unsigned int fileOffset, bool bEnd)
{
    unsigned int at = 0;
    if (nHandle != -1) {
        int from = (bEnd) ? SEEK_END : SEEK_SET;
        int pos = _lseek(nHandle, fileOffset, from);
        if (pos == -1) {
            closeFile(nHandle);
        }
        at = pos;
    }
    return at;
}
unsigned int getAt(Handle& nHandle)
{
    unsigned int at = 0;
    if (nHandle != -1) {
        int pos = _lseek(nHandle, 0, SEEK_CUR);
        if (pos == -1) {
            closeFile(nHandle);
        }
        at = pos;
    }
    return at;
}
bool readFile(Handle& nHandle, unsigned int nBytes, unsigned char* pBuffer)
{
    unsigned int nRead = 0;
    if (nHandle != -1) {
        nRead = _read(nHandle, pBuffer, nBytes);
    }
    return nRead == nBytes;
}
/*
bool writeBytes(Handle& nHandle, unsigned int cntBytes, const void* pBytes)
{
    if (nHandle != -1) {
        _write(nHandle, pBytes, (unsigned int) cntBytes);
    }

}*/
void closeFile(Handle& nHandle)
{
    if (nHandle != -1) {
        _close(nHandle);
        nHandle = -1;
    }
}

bool copyFile(const std::string& srcPath, const std::string& destPath)
{
    fs::path fsSrc = srcPath;
    fs::path fsDst = destPath;
    std::error_code ec;
    bool bGood = true;
    if (false == fs::is_regular_file(fsDst)) {
        bGood = fs::copy_file(fsSrc, fsDst, ec);
    }
    return bGood;
}

unsigned char* loadFile(const std::string& path, unsigned int& length)
{
    unsigned char* pFileBytes = NULL;

    /* Open the file */
    Handle nHandle = openFile(path);
    if (nHandle > 0) {
        length = 0;
        return pFileBytes;
    }

    /* Get the file length, allocate the data and read */
    int pos = _lseek(nHandle, 0, SEEK_END);
    length = getAt(nHandle);
    pFileBytes = (unsigned char*)malloc(length * sizeof(unsigned char));
    if (NULL != pFileBytes) {
        setAt(nHandle, 0);
        unsigned int nRead = readFile(nHandle, length, pFileBytes);
        if (nRead != length) {
            length = 0;
            free(pFileBytes);
            pFileBytes = NULL;
        }
    }
    else {
        length = 0;
    }
    closeFile(nHandle);

    return pFileBytes;
}

}