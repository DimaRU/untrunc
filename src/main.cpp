/*
    Untrunc - main.cpp

    Untrunc is GPL software; you can freely distribute,
    redistribute, modify & use under the terms of the GNU General
    Public License; either version 2 or its successor.

    Untrunc is distributed under the GPL "AS IS", without
    any warranty; without the implied warranty of merchantability
    or fitness for either an expressed or implied particular purpose.

    Please see the included GNU General Public License (GPL) for
    your rights and further details; see the file COPYING. If you
    cannot, write to the Free Software Foundation, 59 Temple Place
    Suite 330, Boston, MA 02111-1307, USA.  Or www.fsf.org

    Copyright 2010 Federico Ponchio

                            */

#include <iostream>
#include <string>

#include "mp4.h"
#include "atom.h"
#include "common.h"

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#ifdef __linux__
#include <sys/statfs.h>
#endif
#include <sys/mount.h>

using namespace std;

int listCommand(const char * directoryName);
int recoveryCommand(const char * donorName, const char * directoryName);
int recovery(const char * baseDirectory, const char * donorName, const char * name);

int main(int argc, const char * argv[]) {
    // argv[1] = command
    // argv[2] = recovery directory

    if (argc < 3) return EXIT_FAILURE;
    
    std::cout << "Version:2\n";
    if (strcmp(argv[1], "list") == 0) {
        return listCommand(argv[2]);
    } else if (strcmp(argv[1], "recovery") == 0) {
        return recoveryCommand(argv[2], argv[3]);
    } else {
        return EXIT_FAILURE;
    }
}

int listCommand(const char * directoryName) {
    struct dirent *ent;
    int fileCount = 0;
    uint64_t freeSpace = 0;
    struct statfs vfs;

    auto dir = opendir(directoryName);
    if (dir == NULL) {
        std::cout << "Fail to open " << directoryName << std::endl;
        perror ("");
        return EXIT_FAILURE;
    }
    
    while ((ent = readdir (dir)) != NULL) {
        if (ent->d_type != DT_DIR) continue;
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 || ent->d_name[0] != '.') continue;
        auto filename = std::string(directoryName) + "/" + std::string(ent->d_name) + "/0.mp4";
        
        struct stat stat_buf;
        auto result = stat(filename.c_str(), &stat_buf);
        if (result != 0){
            perror(filename.c_str());
            continue;
        }

        result = statfs(filename.c_str(), &vfs);
        if (result != 0) {
            perror(filename.c_str());
            continue;
        }
        freeSpace = vfs.f_bavail * vfs.f_bsize;
        
        fileCount++;
        std::cout << "Entry:" << std::string(directoryName) + "/" + std::string(ent->d_name) << ":" << stat_buf.st_size << std::endl;
    }
    
    closedir (dir);
    std::cout << "Files/free:" << fileCount << "/" << freeSpace << std::endl;
    return 0;
}

int recoveryCommand(const char * donorName, const char * directoryName) {
    auto fileName = std::string(directoryName) + "/0.mp4";
    string tempFilename;
    // recovery

    try {
        Mp4 mp4;
        g_mp4 = &mp4;  // singleton is overkill, this is good enough

        mp4.parseOk(donorName, false);
        mp4.repair(fileName.c_str());
        tempFilename = mp4.getPathRepaired(donorName, fileName.c_str());
    }
    catch (const char* e) {
        cerr << e << '\n';
        return 1;
    }
    catch (string e) {
        cerr << e << '\n';
        return 1;
    }

    if (rename(tempFilename.c_str(), fileName.c_str()) != 0) {
        perror(fileName.c_str());
        return 1;
    }
    // rename dir
    auto pos = std::string(directoryName).find_last_of('.');
    auto newDirectory = std::string(directoryName).erase(pos, 1);
    cout << newDirectory << endl;
    
    if (rename(directoryName, newDirectory.c_str()) != 0) {
        perror(newDirectory.c_str());
        return 1;
    }
    return 0;
}
