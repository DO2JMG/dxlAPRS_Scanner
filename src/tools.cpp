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
#include <stdexcept>

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

  string gettime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);

    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
  }

  vector<string> splitString(const string& str) {
    //try {
      vector<string> tokens;
    
      stringstream ss(str);
      string token;
      while (getline(ss, token, ',')) {
        tokens.push_back(token);
      }
    
    //  return tokens;
    //} catch (const exception& e) {
    //  cerr << "Error: " << e.what() << endl;
    //}
    return tokens;
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

  bool isNumeric(const string& str) {
    for (char c : str) {
        if (!isdigit(c)) {
            return false; // Found a non-digit character
        }
    }
    return true; // All characters are digits
  }

  bool compareNumbers(int num1, int num2, int difference) {
    if (num1 < num2) {
      int diff = num2 - num1;
      if (diff <= difference) { return true; } 
    } else if (num1 > num2) {
      int diff = num1 - num2;
      if (diff <= difference) { return true; } 
    } else if (num1 == num2) {
      return true;
    }
    return false;
    
  }

  int converttoint(string data) {
    int number = stoi(data);
    return number;
  }
  
  string converttostring(int data) {
    string number = to_string(data);
    return number;
  }

  void findbigsignal(vector<frequency_list> vfl) {
    if (vfl.size() > 0) {
      for (int i = 0; i < vfl.size(); i++) { 
        if (i > 0 && (vfl[i].frequency - vfl[i-1].frequency) < 11 && (vfl[i+1].frequency - vfl[i].frequency ) < 11) {
          vfl[i].bandwidth = vfl[i].bandwidth + 20;
          vfl.erase(vfl.begin() + (i-1));
          vfl.erase(vfl.begin() + (i+1));
        }
      }
      for (int i = 0; i < vfl.size(); i++) { 
        if (i > 0 && (vfl[i].frequency - vfl[i-1].frequency) < 21 && (vfl[i+1].frequency - vfl[i].frequency ) < 21) {
          vfl[i].bandwidth = vfl[i].bandwidth + 40;
          vfl.erase(vfl.begin() + (i-1));
          vfl.erase(vfl.begin() + (i+1));
        }
      }
      
    }
  }
}

