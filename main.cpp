// dxlAPRS_scanner - dxlAPRS extension for scanning frequencies

// Copyright (C) Jean-Michael Grobel (DO2JMG) <do2jmg@wettersonde.net>

// Released under GNU GPL v3 or later

#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <thread> 
#include <algorithm>
#include <iomanip> 

#include "tools.h"

using namespace std;

struct scanner_config {
  bool verbous = false;
  int port = 0; 
  int steps = 0;
  int startfrequency = 0;
  int countermaxnoise = 100;
  int counterminpeaks = 3;
  int minbandwidth = 5;
  int maxbandwidth = 22;
  int squelch = 65; 
  int timer_peaks = 30; // refresh frequency list after 60 seconds
  int timer_holding = 120; // frequencies clear after 120 seconds
  int level = 5;
  string filename;
  string blacklist;
};

struct frequency_list {
  int frequency;
  int bandwidth;
  int timestamp;
  string serial;
};

scanner_config config;
tools tool;

vector<int> peaks;
vector<frequency_list> frequencies;
vector<int> blacklist;

double round_to(double value, double precision = 1.0) {
    return round(value / precision) * precision;
}


int round_double(double value) {
    double result = round_to(value, 0.01);

    return result;
} 

bool frequencyisonlist(double frequency) {
  if (peaks.size() > 0) {
    for (long unsigned i = 0; i < (peaks.size()); i++) {
      if (peaks[i] == frequency) {
        return true;
      }
    }
  }
  return false;
}
int receivedata() {
  int serSockDes;
  struct sockaddr_in serAddr, cliAddr;
  socklen_t cliAddrLen;
  char buffer[1500] = {0};

  if ((serSockDes = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    tool.debug("socket creation error...", false);
    exit(-1);
  }

  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(config.port);
  serAddr.sin_addr.s_addr = INADDR_ANY;

  if ((bind(serSockDes, (struct sockaddr*)&serAddr, sizeof(serAddr))) < 0) {
    tool.debug("binding error...", false);
    close(serSockDes);
    exit(-1);
  }

  cliAddrLen = sizeof(cliAddr);

  if (config.verbous) tool.debug("Start frequency " + to_string(config.startfrequency) + "hz", false);
  if (config.verbous) tool.debug("Steps : " + to_string(config.steps) + "hz", true);


  while (true) {
    int readStatus = recvfrom(serSockDes, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliAddr, &cliAddrLen);


    if (buffer[0] == 'S' && buffer[1] == 'D' && buffer[2] == 'R' && buffer[3] == 'L' && readStatus > 0) { // receive data from sdrtst
      tool.debug("------------- Receiving data from port " + to_string(config.port), false);

      int dlen = sizeof(buffer);

      vector<int> db;
      vector<int> db_avg;
      
      // add data to array

      for (int i = 12; i < (dlen); i++) {
        if (buffer[i] > 0) {
          db.push_back(buffer[i]); 
        }
      }
      
      // calculateing average

      tool.debug("Calculateing average", false);

      int avg_all = 0;
      int avg_min = 1024;
      int avg_max = 0;

      int dbc = 0;
      int dbs = 0;

      for (auto i = db.begin(); i != db.end(); ++i) {
        if (*i < avg_min) { avg_min = *i; }
        if (*i > avg_max) { avg_max = *i; }

        if (dbc <= config.countermaxnoise) {
          dbs += *i;
          dbc++;
        } else {
          db_avg.push_back(dbs / config.countermaxnoise);
          if (config.verbous) tool.debug("db noise block : " + to_string(dbs / config.countermaxnoise), false);

          dbs = 0;
          dbc = 0;
        }
      }

      for (auto i = db_avg.begin(); i != db_avg.end(); ++i) { avg_all += *i; }

      avg_all = (avg_all / db_avg.size());

      if (config.verbous) tool.debug("db avg : " + to_string(avg_all), false);
      if (config.verbous) tool.debug("db min : " + to_string(avg_min), false);
      if (config.verbous) tool.debug("db max : " + to_string(avg_max), false);

      // detecting peaks
      tool.debug("Detecting peaks", false);

      //-----------------------------------------------

      float db_spike3 = 10.0;
      int spike_wl3 = 3; 
      int spike_wl5 = 5;

      int dbsize = db.size();

      for (long unsigned i = 0; i < (db.size()); i++) {
        if ( db[i] - db[(i-spike_wl5+dbsize)%dbsize] > db_spike3
            && db[i] - db[(i-spike_wl3+dbsize)%dbsize] > db_spike3
            && db[i] - db[(i+spike_wl3+dbsize)%dbsize] > db_spike3
            && db[i] - db[(i+spike_wl5+dbsize)%dbsize] > db_spike3
            ) {
              db[i] = (db[(i-spike_wl3+dbsize)%dbsize] +db[(i+spike_wl3+dbsize)%dbsize])/2.0;
          }
      }

      //-----------------------------------------------

      for (long unsigned i = 0; i < (db.size()); i++) { // add peaks to the list
        if (db[i] > (avg_all + ((avg_all - avg_min)) - config.level)) {
          int ifq = (i)*config.steps + (config.startfrequency);
         
          ifq = ifq/1000.0;  // hz in khz
        
          if (frequencyisonlist(ifq) == false) { // frequency is not on the list
            peaks.push_back(ifq);
            if (config.verbous) tool.debug("detected peak at " + to_string(ifq) + "hz db = " + to_string(db[i]), false);
          }
        }
      } 
    
    } else {
      close(serSockDes);
      exit(-1);
    }
  }
  close(serSockDes);
  return 0;
}

// Evaluate peaks

int getpeaks() {
  double fql, fqh;
  double fq;
  int pib, fqc;

  while (true) {

    // Load blacklist

    blacklist.clear(); // clear blacklist

    fstream fblacklist;

    fblacklist.open(config.blacklist,ios::in);
    if (fblacklist.is_open()) { 
      string line;
      while(getline(fblacklist, line)) {
        if (line.length() > 1) {
          blacklist.push_back(stoi(line));
          if (config.verbous) tool.debug(line, false);
        }
      }
    }

    //--------------------------

    sort(std::begin(peaks), std::end(peaks)); // sort peaks

    int i = 0;

    if (peaks.size() > 0) {
      tool.debug("------------- peaks...", false);

      fqc = peaks.size();

      i = 0;
      pib = 0;   
       while ((i < fqc) && (frequencies.size() < 400)) {
        fql = peaks[pib];
        if (config.verbous) tool.debug("smallest frequency " + to_string((int)fql), false);

        while((pib < fqc-1) && ((peaks[pib+1] - peaks[pib]) < 5.0)) { pib++; }

        if (config.verbous) tool.debug("greatest frequency "+to_string(peaks[pib])+", at index "+to_string(pib),false);

        if ((pib - i) >= config.counterminpeaks)  // 
        {
          if (config.verbous) tool.debug("strong signals detected with more or equal " + to_string(config.counterminpeaks + 1) + " nearby frequency",false);

          frequency_list new_frequency;
          fqh = peaks[pib];
          fq = (fqh - fql);
          new_frequency.bandwidth = fq;
          if (new_frequency.bandwidth > config.maxbandwidth) {new_frequency.bandwidth = config.maxbandwidth; }

          fq = fq / 2;
          fq = (fq > (floor(fq)+0.5f)) ? ceil(fq) : floor(fq);

          double rounded_frequency = round_double(((fql + fq)/1000) * 100.0) / 100.0; 

          bool ison = false;  
 
          for (i = 0; i < frequencies.size(); i++) { 
            if ((rounded_frequency*1000) == frequencies[i].frequency) { //check frequency is on the list
              frequencies[i].timestamp = tool.gettimestamp();
              ison = true;
            }
          }

      
          for (i = 0; i < blacklist.size(); i++) { 
            int frequency_check = rounded_frequency*1000;
            if (to_string(frequency_check) == to_string(blacklist[i])) { //check frequency is on the blacklist
              ison = true;
            }
          }

          if (ison == false && new_frequency.bandwidth > config.minbandwidth) {
            new_frequency.frequency = (rounded_frequency*1000); // add frequency to the list
            new_frequency.timestamp = tool.gettimestamp();
            frequencies.push_back(new_frequency);
          }
        }
        pib++;
        i = pib;
        if (config.verbous) tool.debug("go ahead on index " + to_string(i), false);
      }
      tool.debug(to_string(frequencies.size()) + " frequencies saved in the list", false);
      string out = "# created with dxlAPRS_scanner\n\n";

      for (auto& element : frequencies) { // creating the frequencieslist for dxlAPRS
        tool.debug(to_string(element.frequency) + " " + to_string(element.bandwidth) + " " + to_string(element.timestamp), false);
        string out_frequency = to_string(element.frequency);

        out.append("f " + out_frequency.insert(3, ".") + " 5 " + to_string(config.squelch) + " 0 " + to_string(element.bandwidth * 1000) + "\n");
      }
      for (i = 0; i < frequencies.size(); i++) { 
        if ( (tool.gettimestamp() - frequencies[i].timestamp) > config.timer_holding) { //check timestamp and remove from list
          frequencies.erase(frequencies.begin() + i);
        }
      }

      string out_startfrequency = to_string(config.startfrequency/1000);
      string out_stopfrequency = to_string((config.startfrequency/1000)+2000);

      out.append("s " + out_startfrequency.insert(3, ".") + " " + out_stopfrequency.insert(3, ".") + " " + to_string(config.steps) + " 6 3000\n");

      tool.file_write(config.filename, out);

      out = "";
    }

    peaks.clear();

    sleep(config.timer_peaks);
  }
}


int main(int argc, char** argv) {
  tool.debug("dxlAPRS_scanner - dxlAPRS extension for scanning frequencies", false);
  tool.debug("Copyright (C) Jean-Michael Grobel (DO2JMG) <do2jmg@do2jmg.eu>", true);

  sleep(3);

  for (int i = 0; i<argc; i++) {
    if (strcmp(argv[i],"-v") == 0) {
      config.verbous = true;
    }
    if (strcmp(argv[i],"-p") == 0) { // port sdrtst -L <ip>:<port>
      if(i+1 < argc) {
        config.port = stoi(argv[i+1]);
      } else {
        tool.debug("Error : Port", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-s") == 0) {
      if(i+1 < argc) {
        config.steps = stoi(argv[i+1]);
        if (config.steps < 1500) {config.steps = 1500; } // 1500hz is minimum
      } else {
        tool.debug("Error : Steps", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-f") == 0) {
      if(i+1 < argc) {
        config.startfrequency = stoi(argv[i+1]); // start frequency in hz
      } else {
        tool.debug("Error : Startfrequency", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-o") == 0) { // channel file (sdrcfg.txt)
      if(i+1 < argc) {
        config.filename = argv[i+1];
      } else {
        tool.debug("Error : Filename", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-t") == 0) { // timer for holding frequencies 
      if(i+1 < argc) {
        config.timer_holding = stoi(argv[i+1]);
      } else {
        tool.debug("Error : Holding timer", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-b") == 0) { // blacklist-file
      if(i+1 < argc) {
        config.blacklist = argv[i+1];
      } else {
        tool.debug("Error : Blacklist", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-q") == 0) { // squelch for channel file
      if(i+1 < argc) {
        config.squelch = stoi(argv[i+1]);
      } else {
        tool.debug("Error : Squelch", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-n") == 0) {
      if(i+1 < argc) {
        config.level = stoi(argv[i+1]);
      } else {
        tool.debug("Error : Level", false);
        return 0;
      }
    } 
  }

  thread thread_receivedata (receivedata); // thread for receiving data via udp
  thread thread_getpeaks (getpeaks); //thread for calculating frequencies       

  thread_receivedata.join();                
  thread_getpeaks.join();             
}