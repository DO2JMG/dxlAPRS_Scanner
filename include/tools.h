#ifndef	tools_H
#define	tools_H

#pragma once

using namespace std;

namespace tools {
        void debug(std::string strData, bool newline);
        void file_write(const string strFilename, string strData);
        void file_write_append(const string strFilename, string strData);
        int gettimestamp();
        vector<std::string> splitString(const std::string& str);
        int round_double(double value);
};

#endif