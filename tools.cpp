#include <iostream>
#include <string>
#include <vector>
#include <sstream>  
#include <fstream>
#include <ctime>
#include "tools.h"

using namespace std;

void tools::debug(string strData, bool newline) {
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

void tools::file_write(const string strFilename, string strData) {
  fstream fFileData;
  fFileData.open(strFilename, ios::out);
  fFileData << strData;
  fFileData.close();
}

int tools::gettimestamp() {
  std::time_t t = std::time(0);  
  std::cout << t << " seconds since 01-Jan-1970\n";

  return t;
}