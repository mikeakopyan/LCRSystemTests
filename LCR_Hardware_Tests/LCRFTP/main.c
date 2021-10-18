#include <string>
#include <iostream>
#include <conio.h> // for getch()
#include <stdio.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <thread>

#include "ftp.hh"

#include <stdlib.h>
#include "pugixml.hpp"

template <typename ... Args>
std::string stringFormatted(const std::string& format, Args&& ...args)
{
    int size = std::snprintf(NULL, 0, format.c_str(), std::forward<Args>(args)...);
    size += 1;
    std::string output(size, '\0');
    std::snprintf(&output[0], size, format.c_str(), std::forward<Args>(args)...);
    output.erase(std::find(output.begin(), output.end(), '\0'), output.end());
    return output;
}

class Timer
{
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const
    {
        return std::chrono::duration_cast<second_>
            (clock_::now() - beg_).count();
    }

private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};


class ftp_target
{
public:
    ftp_target() : bEnabled(true) {}
    ~ftp_target() {}
    ftp_target(const ftp_target& ot) : deviceName(ot.deviceName), ipaddress(ot.ipaddress), user(ot.user), pwd(ot.pwd),
        drivePath(ot.drivePath), driveSpace(ot.driveSpace), availableDriveSpace(ot.availableDriveSpace),
        bEnabled(ot.bEnabled) {}
    std::string deviceName;
    std::string ipaddress;
    std::string user;
    std::string pwd;
    std::string drivePath;
    unsigned int driveSpace;
    unsigned int availableDriveSpace;
    bool bEnabled;
};

class Parameters
{
public:
    std::string message() const;

public:
    Parameters() : mRepeatCount(1) { ; }
    Parameters(int argc, char* argv[]);
    ~Parameters() { ; }

    bool isLegal() const { return mbLegal; }

    bool getInputDirPath(std::string& path) { path = mInputDirectoryPath; return mbLegal; }
    bool getInputFilePath(std::string& fpath) { fpath = mInputFilePath; return true; }
    uint32_t getRepeat() const { return mRepeatCount; }

private:
    std::string mInputFilePath;
    std::string mInputDirectoryPath;
    uint32_t mRepeatCount;
    bool mbLegal;
};

std::string Parameters::message() const
{
    return std::string("\nUsage: /f <path to file> /r <repeat send count>");
}

Parameters::Parameters(int argc, char* argv[]) : mInputDirectoryPath(), mbLegal(false)
{
    mbLegal = false;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (argv[i][0] == '/' || argv[i][0] == '-') {
                switch (argv[i][1]) {
                case 'd':
                    ++i;
                    mInputDirectoryPath = argv[i];
                    break;

                case 'f':
                    ++i;
                    mbLegal = true;
                    mInputFilePath = argv[i];
                    break;

                case 'r':
                    ++i;
                    mRepeatCount = atoi(argv[i]);
                    break;

                default:
                    std::cout << message().c_str() << std::endl;
                    return;
                }
            }
        }
        mbLegal = !mInputFilePath.empty();
    }
    else {
        std::cout << message().c_str() << std::endl;
    }
    if (!mbLegal) {
        std::cout << message().c_str() << std::endl;
    }
}


void sendFile(const std::string& ip, const std::string& usr, const std::string& pwd,
    const std::string& srcpath, const std::string& destPath, std::string file)
{
    ftp_t ftp(ip.c_str(), 21);
    ftp.login(usr.c_str(), pwd.c_str());

    bool bChanged = true;
    const char* pPath = srcpath.c_str();
    if (false && destPath.size() > 1) {
        char cPWD[1024];
        ftp.current_directory(1024, &cPWD[0]);
        if (0 != std::strcmp(cPWD, destPath.c_str())) {
            ftp.change_directory(destPath.c_str(), bChanged);
            ftp.current_directory(1024, (char*) &cPWD[0]);
            bChanged = 0 == strcmp(cPWD, destPath.c_str());
        }
    }
    else {
        ftp.change_directory(destPath.c_str(), bChanged);
 //       std::cout << "Changed to " << destPath.c_str() << ((bChanged) ? " success" : " failed") << std::endl;
    }
    if (true == bChanged) {
        ftp.put_file(srcpath.c_str(), file.c_str());
    }
    ftp.logout();
}

void deleteFile(const std::string& ip, const std::string& usr, const std::string& pwd,
    char destDrive, const std::string& file)
{
    ftp_t ftp(ip.c_str(), 21);
    ftp.login(usr.c_str(), pwd.c_str());

    bool bChanged = false;
    ftp.change_directory(stringFormatted("%c:\\", destDrive).c_str(), bChanged);

    if (true == bChanged) {
        ftp.get_file_list();

        for (size_t f = 0; ftp.m_file_nslt.size(); ++f) {
            if (ftp.m_file_nslt[f] == file) {
                std::cout << "From " << ip.c_str() << " deleting: " << file.c_str() << std::endl;
                ftp.delete_file(file.c_str());
                break;
            }
        }
    }
    ftp.logout();
}

unsigned int sizeOfFile(const std::string& path)
{
    unsigned int nBytes = 0;

    filesystem::Handle handle = filesystem::openFile(path);
    if (handle < 0) {
        errno = 0; /* We read errno also from the tcp layer... */
    }
    else {
        /* Get the file length, allocate the data and read */
        nBytes = filesystem::setAt(handle, 0, true);
    }

    filesystem::closeFile(handle);
    return nBytes;
}

void push(ftp_target& target, const std::string& filenm, uint32_t index)
{
    bool bSent = false;
    {
        std::string filename;
        size_t slashAt = filenm.find_last_of('\\');
        if (std::string::npos == slashAt) {
            filename = filenm;
        }
        else {
            slashAt++;
            filename = filenm.substr(slashAt, filenm.size() - slashAt);
        }
        std::string basePath = filename.substr(0, filename.size() - 6);
        filename = stringFormatted("%s_%u.vfdat", basePath.c_str(), index);

        unsigned int nBytes = sizeOfFile(filenm);
        if (target.availableDriveSpace > nBytes) {
            target.availableDriveSpace -= nBytes;
            std::string srcpath = stringFormatted(".\\FTP\\files\\");
            {
                std::string path = "./FTP/files/";
                std::string destiny = path + filename;
                
                filesystem::copyFile(filenm, destiny);
            }
            bSent = true;

            Timer timer;
            timer.reset();
            sendFile(target.ipaddress, target.user, target.pwd, srcpath, target.drivePath, filename);
            printf("Pushed the copy of %s to %s in seconds %0.5f", filename.c_str(),
                target.ipaddress.c_str(), timer.elapsed());
        }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//usage
/////////////////////////////////////////////////////////////////////////////////////////////////////

void usage()
{
    std::cout << "Not sure but what ever you did is wrong. Play again?" << std::endl;
    exit(0);
}

class LCRFTP {
public:
    LCRFTP() {};
    ~LCRFTP() {};

    std::vector<ftp_target> ftp_targets;

    void setupFTP(std::string xmlFileName);
    bool copyToLCR(std::string filename);
};

void LCRFTP::setupFTP(std::string xmlFileName) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xmlFileName.c_str());

    if (result) {

        pugi::xml_node targetset = doc.child("vfconfig").child("targetset");
        if (targetset) {
            pugi::xml_node target_node = targetset.child("target");
            do
            {
                int target_instance = target_node.attribute("id").as_int(0);
                std::string sEnabed = target_node.attribute("enabled").as_string();
                ftp_target target;
                target.bEnabled = (sEnabed == "y") ? true : false;
                target.deviceName = stringFormatted("Target_Index_%d", target_instance);

                pugi::xml_node addr = target_node.child("address");
                target.ipaddress = addr.attribute("ip").as_string();
                target.user = addr.attribute("user").as_string();
                target.pwd = addr.attribute("pwdftp").as_string();
                pugi::xml_node storage = target_node.child("storage");
                target.drivePath = storage.attribute("path").as_string();
                target.driveSpace = storage.attribute("size").as_uint() * 1024;
                target.availableDriveSpace = target.driveSpace;

                ftp_targets.push_back(target);

                target_node = target_node.next_sibling("target");
            } while (target_node);
        }
    }
    else {
        std::cout << "Invalid configuration XML document" << std::endl;
        exit(-1);
    }
}

bool LCRFTP::copyToLCR(std::string filename) {
    uint32_t times = 1;
    for (uint32_t t = 0; t < times; ++t) {
        for (size_t i = 0; i < ftp_targets.size(); ++i) {
            if (ftp_targets[i].bEnabled) {
                push(ftp_targets[i], filename, t + 1);
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////
//main
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    Parameters parameters(argc, argv);
    std::string sDirPath;
    
    if (false == parameters.isLegal()) {
        return 0;
    }
    parameters.getInputDirPath(sDirPath);

    std::string config_path = "./push_config.xml";

    std::vector<ftp_target> ftp_targets;

    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(config_path.c_str());

        if (result) {

            pugi::xml_node targetset = doc.child("vfconfig").child("targetset");
            if (targetset) {
                pugi::xml_node target_node = targetset.child("target");
                do
                {
                    int target_instance = target_node.attribute("id").as_int(0);
                    std::string sEnabed = target_node.attribute("enabled").as_string();
                    ftp_target target;
                    target.bEnabled = (sEnabed == "y") ? true : false;
                    target.deviceName = stringFormatted("Target_Index_%d", target_instance);

                    pugi::xml_node addr = target_node.child("address");
                    target.ipaddress = addr.attribute("ip").as_string();
                    target.user = addr.attribute("user").as_string();
                    target.pwd = addr.attribute("pwdftp").as_string();
                    pugi::xml_node storage = target_node.child("storage");
                    target.drivePath = storage.attribute("path").as_string();
                    target.driveSpace = storage.attribute("size").as_uint() * 1024;
                    target.availableDriveSpace = target.driveSpace;

                    ftp_targets.push_back(target);

                    target_node = target_node.next_sibling("target");
                } while (target_node);
            }
        }
        else {
            std::cout << "Invalid configuration XML document" << std::endl;
            exit(-1);
        }
    }

    filesystem::createDirectory("./FTP");
    filesystem::createDirectory("./FTP/files");

    std::string filename; 
    parameters.getInputFilePath(filename);
    
    uint32_t times = parameters.getRepeat();
    for (uint32_t t = 0; t < times; ++t) {
        for (size_t i = 0; i < ftp_targets.size(); ++i) {
            if (ftp_targets[i].bEnabled) {
                push(ftp_targets[i], filename, t + 1);
            }
        }
    }

#if 0
    DirectoryParser parser;
    parser.setDirectoryPath(sDirPath, ".vfdat");
    for (size_t i = 0; i < ftp_targets.size(); ++i) {
        if (ftp_targets[i].bEnabled) {
            std::string filenm;
            if (parser.getFile(filenm)) {
                do {
                    Timer timer;
                    timer.reset();
                    push(ftp_targets[i], filenm);
                    std::cout << "Pushed " << filenm.c_str() << " to " << 
                        ftp_targets[i].ipaddress.c_str() << " in seconds " << timer.elapsed() << std::endl;
                } while (parser.getNextFile(filenm));
            }
        }
    }
#endif

//     std::cout << "Press any key to continue...";
//     _getch();

  return 0;
}



#if 0
auto spot = std::chrono::system_clock::now();
std::time_t tnow = std::chrono::system_clock::to_time_t(spot);
dtStr[0] = dtStr[19] = 0;
std::strftime(dtStr, 20, "%Y-%m-%d-%H-%M-%S", std::localtime(&tnow));

#endif
