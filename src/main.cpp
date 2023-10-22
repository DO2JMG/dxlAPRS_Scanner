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
using namespace tools;

#define bw_RS41 8
#define bw_RS92 12
#define bw_M10M20 21
#define bw_IMET 12
#define bw_DFM 6
#define bw_MEISEI 15

struct scanner_config {
  bool verbous = false;
  int sdrtst_port = 0; 
  int sondeudp_port = 0; 
  int steps = 0;
  int startfrequency = 0;
  int cmn = 100;
  int cmp = 3;
  int minbw = 5;
  int maxbw = 22;
  int squelch = 65; 
  int lowpass = 0;
  int timer_peaks = 30; // refresh frequency list after 60 seconds
  int timer_holding = 180; // frequencies clear after 120 seconds
  int timer_serial = 60;
  int level = 5;
  int tuner_settings = 0;
  int tuner_gain = 0;
  int tuner_auto_gain = 0;
  int tuner_gain_correction = 1;
  int tuner_ppm = 0;
  int max_frequency_fql = 10;
  int max_frequency_fpl = 400;
  string filename;
  string blacklist;
  string whitelist;
};

scanner_config config;

vector<int> peaks;
vector<frequency_list> vfq;
vector<int> vbl;
vector<frequency_list> vwl;

vector<int> vsl;

vector<int> findSpikes(const vector<int>& data, int threshold) {
    vector<int> spikeIndices;

    for (int i = 1; i < data.size() - 1; ++i) {
        if (data[i] > threshold && data[i] > data[i - 1] && data[i] > data[i + 1]) {
            spikeIndices.push_back(i);
        }
    }
    return spikeIndices;
}

bool frequencyisonlist(double fq) {
  if (peaks.size() > 0) {
    for (long unsigned i = 0; i < (peaks.size()); i++) {
      if (peaks[i] == fq) {
        return true;
      }
    }
  }
  return false;
}

int receive_sdrtst() {
  int serSockDes;
  struct sockaddr_in serAddr, cliAddr;
  socklen_t cliAddrLen;
  char buffer[1500] = {0};

  if ((serSockDes = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    debug("socket creation error...", false);
    exit(-1);
  }

  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(config.sdrtst_port);
  serAddr.sin_addr.s_addr = INADDR_ANY;

  if ((bind(serSockDes, (struct sockaddr*)&serAddr, sizeof(serAddr))) < 0) {
    debug("binding error...", false);
    close(serSockDes);
    exit(-1);
  }

  cliAddrLen = sizeof(cliAddr);

  if (config.verbous) debug("Start frequency " + converttostring(config.startfrequency) + "hz", false);
  if (config.verbous) debug("Steps : " + converttostring(config.steps) + "hz", true);


  while (true) {
    int readStatus = recvfrom(serSockDes, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliAddr, &cliAddrLen);


    if (buffer[0] == 'S' && buffer[1] == 'D' && buffer[2] == 'R' && buffer[3] == 'L' && readStatus > 0) { // receive data from sdrtst
      debug("------------- Receiving data from port " + converttostring(config.sdrtst_port), false);

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

      if (config.verbous) debug("DB Count : " + converttostring(db.size()) , true);

      if (config.verbous) debug("Calculateing average", false);

      int avg_all = 0;
      int avg_min = 1024;
      int avg_max = 0;

      int dbc = 0;
      int dbs = 0;

      if (db.size() > 0) {
        for (auto i = db.begin(); i != db.end(); ++i) {
          if (*i < avg_min) { avg_min = *i; }
          if (*i > avg_max) { avg_max = *i; }

          if (dbc <= config.cmn) {
            dbs += *i;
            dbc++;
          } else {
            db_avg.push_back(dbs / config.cmn);
            if (config.verbous) debug("db noise block : " + converttostring(dbs / config.cmn), false);

            dbs = 0;
            dbc = 0;
          }
        }
        
        for (auto i = db_avg.begin(); i != db_avg.end(); ++i) { avg_all += *i; }

        avg_all = (avg_all / db_avg.size());

        if (config.verbous) debug("db avg : " + converttostring(avg_all), false);
        if (config.verbous) debug("db min : " + converttostring(avg_min), false);
        if (config.verbous) debug("db max : " + converttostring(avg_max), false);
      

        vsl = findSpikes(db, (avg_max - (avg_all/2)));

        for (long unsigned i = 0; i < (vsl.size()); i++) {
          int ifq = (vsl[i])*config.steps + (config.startfrequency + 3000);
          db[vsl[i]] == avg_min;
        }

        // detecting peaks
        debug("Detecting peaks", false);


        for (long unsigned i = 0; i < (db.size()); i++) { // add peaks to the list
          if (db[i] > (avg_all + ((avg_all - avg_min)) - config.level)) {

            int ifq = (i)*config.steps + (config.startfrequency + 1500);
          
            ifq = ifq/1000.0;  // hz in khz
          
            if (frequencyisonlist(ifq) == false && peaks.size() < config.max_frequency_fpl) { // frequency is not on the list
              peaks.push_back(ifq);
              if (config.verbous) debug("detected peak at " + converttostring(ifq) + "hz db = " + converttostring(db[i]), false);
            }
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

int receive_sondeudp() {
  int serSockDes;
  struct sockaddr_in serAddr, cliAddr;
  socklen_t cliAddrLen;
  char buffer[1500] = {0};

  if ((serSockDes = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    debug("socket creation error...", false);
    exit(-1);
  }

  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(config.sondeudp_port);
  serAddr.sin_addr.s_addr = INADDR_ANY;

  if ((bind(serSockDes, (struct sockaddr*)&serAddr, sizeof(serAddr))) < 0) {
    debug("binding error...", false);
    close(serSockDes);
    exit(-1);
  }

  cliAddrLen = sizeof(cliAddr);

  while (true) {
    int readStatus = recvfrom(serSockDes, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliAddr, &cliAddrLen);

    if (buffer[0] == 'R' && buffer[1] == 'X' && readStatus > 0) { // receive data from sondeudp
      debug("------------- Receiving data from port " + converttostring(config.sondeudp_port), false);

      string s = buffer;
      
      int cc = countCharacters(s, ',');
  
      if (s.length() > 20 && cc > 1) {

        vector<string> tokens = splitString(s);

        if (tokens.size() >= 2) {
          string tempFQ;
          if (tokens[0].length() > 4) {
            tempFQ = tokens[0].substr(2, tokens[0].length() -5);
          }
      
          for (long unsigned i = 0; i < (vfq.size()); i++) {
            string vfgFQ = converttostring(vfq[i].frequency);

            if (vfgFQ.length() > 4) {
              if (vfgFQ.substr(0, 5) == tempFQ) {

                vfq[i].serial = tokens[2].substr(0, tokens[2].length() -1);
                if (vfq[i].serial.back() == '\n') {
                  vfq[i].serial.pop_back();
                }

                if (tokens[1] == "RS41") {
                  vfq[i].bandwidth = bw_RS41;
                }
                if (tokens[1] == "RS92") {
                  vfq[i].bandwidth = bw_RS92;
                }
                if (tokens[1] == "DFM") {
                  vfq[i].bandwidth = bw_DFM;
                }
                if (tokens[1] == "RS41") {
                  vfq[i].bandwidth = bw_RS41;
                }
                if (tokens[1] == "M10") {
                  vfq[i].bandwidth = bw_M10M20;
                }
                if (tokens[1] == "M20") {
                  vfq[i].bandwidth = bw_M10M20;
                }
                if (tokens[1] == "IMET") {
                  vfq[i].bandwidth = bw_IMET;
                }
                if (tokens[1] == "MEIS") {
                  vfq[i].bandwidth = bw_MEISEI;
                }
              }
            }
            tokens.clear();
            vfgFQ = "";
          }
           tempFQ = "";
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

    vbl.clear(); // clear blacklist

    fstream blf;

    blf.open(config.blacklist,ios::in);
    if (blf.is_open()) { 
      string line;
      while(getline(blf, line)) {
        if (line.length() > 1) {
          if (line.back() == '\n' || line.back() == ' ') {
            line.pop_back();
          }
          vbl.push_back(converttoint(line));
          if (config.verbous) debug("Add to blacklist " + line, false);
        }
      }
      blf.close();
    }
    

    // Load whitelist

    vwl.clear(); // clear whitelist

    fstream wlf;

    wlf.open(config.whitelist,ios::in);
    if (wlf.is_open()) { 
      string line;
      while(getline(wlf, line)) {

        int cc = countCharacters(line, ',');

        if (line.length() >= 10 && cc > 1) {
          frequency_list wfl;

          vector<string> tokens = splitString(line);

          if (line.back() == '\n' || line.back() == ' ') {
            line.pop_back();
          }

          if (isNumeric(tokens[0]) && isNumeric(tokens[1]) && isNumeric(tokens[2]) && tokens.size() > 2) {
            wfl.frequency = converttoint(tokens[0]);
            wfl.bandwidth = converttoint(tokens[1]);
            wfl.afc = converttoint(tokens[2]);

            vwl.push_back(wfl);
          }
          //if (config.verbous) debug(line, false);
        }
      }
      wlf.close();
    }
    

    //--------------------------

    int i = 0;

    if (peaks.size() > 0) {
      debug("------------- peaks...", false);

      sort(std::begin(peaks), std::end(peaks)); // sort peaks

      fqc = peaks.size();

      i = 0;
      pib = 0;   
       while ((i < fqc) && (vfq.size() < 400)) {
        fql = peaks[pib];
        if (config.verbous) debug("smallest frequency " + converttostring((int)fql), false);

        while((pib < fqc-1) && ((peaks[pib+1] - peaks[pib]) < 5.0)) { pib++; }

        if (config.verbous) debug("greatest frequency "+converttostring(peaks[pib])+", at index "+converttostring(pib),false);

        if ((pib - i) >= config.cmp)  // 
        {
          if (config.verbous) debug("strong signals detected with more or equal " + converttostring(config.cmp + 1) + " nearby frequency",false);

          frequency_list nfq;
          fqh = peaks[pib];
          fq = (fqh - fql);
          nfq.bandwidth = fq;
          if (nfq.bandwidth > config.maxbw) {nfq.bandwidth = config.maxbw; }
          if (nfq.bandwidth > 15000) {nfq.afc  = 8; }

          fq = fq / 2;
          fq = (fq > (floor(fq)+0.5f)) ? ceil(fq) : floor(fq);

          double rounded_frequency = round_double(((fql + fq)/1000) * 100.0) / 100.0; 
          rounded_frequency = rounded_frequency *1000;

          bool ison = false;  

          if (vfq.size() > 0) {
            for (i = 0; i < vfq.size(); i++) { 
              if (compareNumbers((int)rounded_frequency, vfq[i].frequency, 10) == true) {
                vfq[i].timestamp = gettimestamp();
                ison = true;
              }
            }
          }

          if (vbl.size() > 0) {
            for (i = 0; i < vbl.size(); i++) { 
              if (compareNumbers((int)rounded_frequency, vbl[i], 10)) {
                ison = true;
                break;
              }
            }
          }

          if (vwl.size() > 0) {
            for (i = 0; i < vwl.size(); i++) { 
              if (compareNumbers((int)rounded_frequency, vwl[i].frequency, 10)) {
                ison = true;
                break;
              }
            }
          }

          if (ison == false && nfq.bandwidth > config.minbw && vfq.size() < config.max_frequency_fql) {
            nfq.frequency = (rounded_frequency); // add frequency to the list
            nfq.timestamp = gettimestamp();
            vfq.push_back(nfq);
          }

          findbigsignal(vfq);
        }
        pib++;
        i = pib;
        if (config.verbous) debug("go ahead on index " + converttostring(i), false);
      }
      debug(converttostring(vfq.size()) + " frequencies saved in the list", false);

      string out = "# created with dxlAPRS_scanner at ";
      out.append(gettime());
      out.append("\n\n");

      if (config.tuner_settings == 1) { // tuner setting enabled
        if (config.tuner_gain_correction != 0) {
        out.append("p 8 " + converttostring(config.tuner_gain_correction) + " \t\t# Tuner gain correction\n");
        }
        out.append("p 3 " + converttostring(config.tuner_auto_gain) + " \t\t# Tuner auto gain\n");
        if (config.tuner_ppm != 0) {
          out.append("p 5 " + converttostring(config.tuner_ppm) + " \t\t# Tuner ppm\n");
        }
        if (config.tuner_gain > 0) {
          out.append("p 4 " + converttostring(config.tuner_gain) + " \t\t# Tuner gain level\n");
        }
        out.append("\n");
      }

      int wlc = 0;

      if (vwl.size() > 0) {
        for (i = 0; i < vwl.size(); i++) { 
          if ((vwl[i].frequency) >= (config.startfrequency / 1000) && (vwl[i].frequency) < ((config.startfrequency / 1000)+2000)) {
            string out_frequency = converttostring(vwl[i].frequency);

            out.append("f " + out_frequency.insert(3, ".") + " " + converttostring(vwl[i].afc) + " " + converttostring(config.squelch) + " " + converttostring(config.lowpass) + " " + converttostring(vwl[i].bandwidth * 1000) + " \t# Whitelist\n");
            wlc++;
          }
        }
      }
      
      if (vfq.size() > 0 && vfq.size() < (config.max_frequency_fql - wlc++) ) {
        for (auto& vfge : vfq) { // creating the frequencieslist for dxlAPRS
          debug(converttostring(vfge.frequency) + " " + converttostring(vfge.bandwidth) + " " + converttostring(vfge.timestamp), false);
          string out_frequency = converttostring(vfge.frequency);

          out.append("f " + out_frequency.insert(3, ".") + " " + converttostring(vfge.afc) + " " + converttostring(config.squelch) + " " + converttostring(config.lowpass) + " " + converttostring(vfge.bandwidth * 1000) + " \t# " + vfge.serial + "\n");
        }

        for (i = 0; i < vfq.size(); i++) { 
          if ( (gettimestamp() - vfq[i].timestamp) > config.timer_holding) { //check timestamp and remove from list
            vfq.erase(vfq.begin() + i);
          }
          if ( (gettimestamp() - vfq[i].timestamp) > config.timer_serial) { //check serial and remove from list
            if (vfq[i].serial.length() < 8) {
              vfq.erase(vfq.begin() + i);
            }
          }
        }
      } else {
        debug("Error Frequency list", false);
      }

      string out_startfrequency = converttostring(config.startfrequency/1000);
      string out_stopfrequency = converttostring((config.startfrequency/1000)+2000);

      out.append("s " + out_startfrequency.insert(3, ".") + " " + out_stopfrequency.insert(3, ".") + " " + converttostring(config.steps) + " 6 3000\n");

      file_write(config.filename, out);

      out = "";
    }

    peaks.clear();

    sleep(config.timer_peaks);
  }
}


int main(int argc, char** argv) {
  debug("dxlAPRS_scanner - dxlAPRS extension for scanning frequencies", false);
  debug("Copyright (C) Jean-Michael Grobel (DO2JMG) <do2jmg@do2jmg.eu>", true);

  sleep(3);

  for (int i = 0; i<argc; i++) {
    if (strcmp(argv[i],"-v") == 0) {
      config.verbous = true;
    }
    if (strcmp(argv[i],"-p") == 0) { // port sdrtst -L <ip>:<port>
      if(i+1 < argc) {
        config.sdrtst_port = converttoint(argv[i+1]);
      } else {
        debug("Error : Port", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-u") == 0) { // port sondeudp -M <ip>:<port>
      if(i+1 < argc) {
        config.sondeudp_port = converttoint(argv[i+1]);
      } else {
        debug("Error : Port", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-s") == 0) {
      if(i+1 < argc) {
        config.steps = converttoint(argv[i+1]);
        if (config.steps < 1500) {config.steps = 1500; } // 1500hz is minimum
      } else {
        debug("Error : Steps", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-f") == 0) {
      if(i+1 < argc) {
        config.startfrequency = converttoint(argv[i+1]); // start frequency in hz
      } else {
        debug("Error : Startfrequency", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-o") == 0) { // channel file (sdrcfg.txt)
      if(i+1 < argc) {
        config.filename = argv[i+1];
      } else {
        debug("Error : Filename", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-t") == 0) { // timer for holding frequencies 
      if(i+1 < argc) {
        config.timer_holding = converttoint(argv[i+1]);
      } else {
        debug("Error : Holding timer", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-b") == 0) { // blacklist-file
      if(i+1 < argc) {
        config.blacklist = argv[i+1];
      } else {
        debug("Error : Blacklist", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-q") == 0) { // squelch for channel file
      if(i+1 < argc) {
        config.squelch = converttoint(argv[i+1]);
      } else {
        debug("Error : Squelch", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-n") == 0) { // Level
      if(i+1 < argc) {
        config.level = converttoint(argv[i+1]);
      } else {
        debug("Error : Level", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-w") == 0) { // Whitelist
      if(i+1 < argc) {
        config.whitelist = argv[i+1];
      } else {
        debug("Error : Whitelist", false);
        return 0;
      }
    } 

    if (strcmp(argv[i],"-tg") == 0) { // Tuner gain
      if(i+1 < argc) {
        config.tuner_gain = converttoint(argv[i+1]);
      } else {
        debug("Error : Tuner gain", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-tga") == 0) { // Tuner auto gain
      if(i+1 < argc) {
        config.tuner_auto_gain = converttoint(argv[i+1]);
      } else {
        debug("Error : Tuner auto gain", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-tgc") == 0) { // Tuner gain correction
      if(i+1 < argc) {
        config.tuner_gain_correction = converttoint(argv[i+1]);
      } else {
        debug("Error : Tuner gain correction", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-tp") == 0) { // Tuner ppm
      if(i+1 < argc) {
        config.tuner_ppm = converttoint(argv[i+1]);
      } else {
        debug("Error : Tuner ppm", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-ts") == 0) { // Tuner settings
      if(i+1 < argc) {
        config.tuner_settings = converttoint(argv[i+1]);
      } else {
        debug("Error : Tuner settings", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-l") == 0) { // Lowpass settings
      if(i+1 < argc) {
        config.lowpass = converttoint(argv[i+1]);
      } else {
        debug("Error : Lowpass settings", false);
        return 0;
      }
    } 
  }

  thread thread_receive_sdrtst (receive_sdrtst); // thread for receiving data via udp
  thread thread_receive_sondeudp(receive_sondeudp);
  thread thread_getpeaks (getpeaks); //thread for calculating frequencies       

  thread_receive_sdrtst.join();
  thread_receive_sondeudp .join();              
  thread_getpeaks.join();             
}
