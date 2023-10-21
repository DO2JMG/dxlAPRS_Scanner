// dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

// Copyright (C) Jean-Michael Grobel (DO2JMG) <do2jmg@wettersonde.net>

// Released under GNU GPL v3 or later

#ifndef	tools_H
#define	tools_H

#pragma once

using namespace std;

namespace tools {
        void debug(std::string strData, bool newline);
        void file_write(const string strFilename, string strData);
        void file_write_append(const string strFilename, string strData);
        int gettimestamp();
        vector<string> splitString(const string& str);
        int round_double(double value);
        int countCharacters(const string& text, char targetChar);
        string gettime();
        bool isNumeric(const string& str);
        bool compareNumbers(int num1, int num2, int difference);
        int converttoint(string data);
        string converttostring(int data);
};

#endif