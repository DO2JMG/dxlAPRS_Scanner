// dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

// Copyright (C) Jean-Michael Grobel (DO2JMG) <do2jmg@wettersonde.net>

// Released under GNU GPL v3 or later

#include <iostream>
#include <string>
#include <vector>
#include <sstream>  
#include <fstream>
#include <ctime>
#include <math.h>
#include "tools.h"

using namespace std;

namespace tools {
  void debug(string strData, bool newline) {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);

    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    cout << "\x1B[92m[\x1B[97m" << buf << "\x1B[92m]\033[0m\t" << strData << "\n";
    if (newline == true) {
      cout << "\n";
    }
  }

  void file_write(const string strFilename, string strData) {
    fstream fFileData;
    fFileData.open(strFilename, ios::out);
    fFileData << strData;
    fFileData.close();
  }

  void file_write_append(const string strFilename, string strData) {
    fstream fFileData;
    fFileData.open(strFilename, std::ios_base::app);
    fFileData << strData;
    fFileData.close();
  }

  int gettimestamp() {
    std::time_t t = std::time(0);  
    return t;
  }

  vector<string> splitString(const string& str) {
      std::vector<std::string> tokens;
      try {
        std::vector<std::string> tokens;
    
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
    
        return tokens;
      } catch( std::logic_error ) {
        return tokens;
      }
  }

  double round_to(double value, double precision = 1.0) {
    return round(value / precision) * precision;
  }

  int round_double(double value) {
      double result = round_to(value, 0.01);

      return result;
  } 

  int countCharacters(const string& text, char targetChar) {
    int count = 0;
    for (char c : text) {
        if (c == targetChar) {
            count++;
        }
    }
    return count;
  }
}