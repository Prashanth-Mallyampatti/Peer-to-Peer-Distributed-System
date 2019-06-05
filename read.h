// vim: set et ts=4
#ifndef _READ_
#define _READ_
#include <iostream>
#include "fstream"
#include <string>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "vector"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

using std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::string;
using std::ifstream;


class FileHandler {
public:
    /* Got the idea to getFileModTime from stackoverflow and 
     * http://www.cplusplus.com/reference/ctime/strftime*/
    static string  getFileModTime(const char *file_path) {
        struct stat fileAttrib;
        stat(file_path, &fileAttrib);
        char date[30];
        strftime(date, 30, "%c %Z", localtime(&(fileAttrib.st_ctime)));
        return string(date);
    }

    static void getStrArray(vector<string>& vectStr, const string& filename, int& size,
                            string& modTime) {
        ifstream fp(filename.c_str());
        if (!fp) {
            cout << "File not found. Exiting..." << endl;
            exit(EXIT_FAILURE);
        }

        std::string str;
        while (getline(fp, str)) {
            size += str.size();
            vectStr.push_back(str);
        }
	fp.close();
        modTime = getFileModTime(filename.c_str());
    }

    static void writeStr(string file_content, string fname) {
	FILE* fp = fopen(fname.c_str(), "w");
	fputs(file_content.c_str(), fp);
	fclose(fp);
    }

    // http://www.martinbroadhurst.com/list-the-files-in-a-directory-in-c.html
    static void read_directory(vector<string>& rfc_names) {
        string dir_name("RFC");
        DIR* dirp = opendir(dir_name.c_str());
        struct dirent * dp;
        while ((dp = readdir(dirp)) != NULL) {
            if (!(string(dp->d_name) == "." or string(dp->d_name) == ".."))
                rfc_names.push_back(dir_name + "/" + string(dp->d_name));
        }
        closedir(dirp);
    }
};
#endif
