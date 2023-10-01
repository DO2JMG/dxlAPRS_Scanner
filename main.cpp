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

using namespace std;

struct scanner_config {
  bool verbous = false;
  int port = 0; 
  int steps = 0;
  int startfrequency = 0;
  int countermaxnoise = 100;
  int counterminpeaks = 3;
  int squelch = 65; 
  int timer_peaks = 30; // refresh frequency list after 30 seconds
  int timer_holding = 120; // frequencies clear after 120 seconds
  string filename;
  string blacklist;
};

struct frequency_list {
  int frequency;
  int bandwidth;
  int timestamp;
};

scanner_config config;

vector<int> peaks;
vector<frequency_list> frequencies;
vector<int> blacklist;

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

int gettimestamp() {
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);

  strftime(buf, sizeof(buf), "%H%M%S", &tstruct);
  return stoi(buf);
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
    debug("socket creation error...", false);
    exit(-1);
  }

  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(config.port);
  serAddr.sin_addr.s_addr = INADDR_ANY;

  if ((bind(serSockDes, (struct sockaddr*)&serAddr, sizeof(serAddr))) < 0) {
    debug("binding error...", false);
    close(serSockDes);
    exit(-1);
  }

  cliAddrLen = sizeof(cliAddr);

  if (config.verbous) debug("Start frequency " + to_string(config.startfrequency) + "hz", false);
  if (config.verbous) debug("Steps : " + to_string(config.steps) + "hz", true);


  while (true) {
    int readStatus = recvfrom(serSockDes, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliAddr, &cliAddrLen);

    if (buffer[0] == 'S' && buffer[1] == 'D' && buffer[2] == 'R' && buffer[3] == 'L' && readStatus > 0) {
      debug("------------- Receiving data from port " + to_string(config.port), false);

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

      debug("Calculateing average", false);

      int avg_all = 0;
      int avg_min = 1024;
      int avg_max = 0;

      int db_noise_counter = 0;
      int db_summary = 0;

      for (auto i = db.begin(); i != db.end(); ++i) {
        if (*i < avg_min) { avg_min = *i; }
        if (*i > avg_max) { avg_max = *i; }

        if (db_noise_counter <= config.countermaxnoise) {
          db_summary += *i;
          db_noise_counter++;
        } else {
          db_avg.push_back(db_summary / config.countermaxnoise);
          if (config.verbous) debug("db noise block : " + to_string(db_summary / config.countermaxnoise), false);

          db_summary = 0;
          db_noise_counter = 0;
        }
      }

      for (auto i = db_avg.begin(); i != db_avg.end(); ++i) {
        avg_all += *i;
      }

      avg_all = (avg_all / db_avg.size());

      if (config.verbous) debug("db avg : " + to_string(avg_all), false);
      if (config.verbous) debug("db min : " + to_string(avg_min), false);
      if (config.verbous) debug("db max : " + to_string(avg_max), false);

      // detecting peaks
      debug("Detecting peaks", false);

      for (long unsigned i = 0; i < (db.size()); i++) { // add peaks to the list
        if (db[i] > (avg_all + ((avg_all - avg_min)) -5)) {
          int iFrequency = (i)*config.steps + (config.startfrequency);
         
          iFrequency = iFrequency/1000.0;  // hz in khz
          iFrequency = (iFrequency > (floor(iFrequency)+0.5f)) ? ceil(iFrequency) : floor(iFrequency);

          if (frequencyisonlist(iFrequency) == false) { // frequency is not on the list
            peaks.push_back(iFrequency);
            if (config.verbous) debug("detected peak at " + to_string(iFrequency) + "hz db = " + to_string(db[i]), false);
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

  double lowfrequency, highfrequency;
  double frequency;
  int peaks_index_block;
  int frequency_count;

  while (true) {

    sort(std::begin(peaks), std::end(peaks)); // sort peaks
    int i = 0;

    if (peaks.size() > 0) {
      debug("------------- peaks...", false);

      frequency_count = peaks.size();

      i = 0;
      peaks_index_block = 0;   
       while ((i < frequency_count) && (frequencies.size() < 200)) {
        lowfrequency = peaks[peaks_index_block];
        if (config.verbous) debug("smallest frequency " + to_string((int)lowfrequency), false);

        while((peaks_index_block < frequency_count-1) && ((peaks[peaks_index_block+1] - peaks[peaks_index_block]) < 5.0)) {
          peaks_index_block++;
        }

        if (config.verbous) debug("greatest frequency "+to_string(peaks[peaks_index_block])+", at index "+to_string(peaks_index_block),false);

        if ((peaks_index_block - i) >= config.counterminpeaks)  // 
        {
          if (config.verbous) debug("strong signals detected with more or equal " + to_string(config.counterminpeaks + 1) + " nearby frequency",false);

          frequency_list new_frequency;

          highfrequency = peaks[peaks_index_block];
          frequency = (highfrequency - lowfrequency);

          new_frequency.bandwidth = frequency;

          frequency = frequency / 2;
          frequency = (frequency > (floor(frequency)+0.5f)) ? ceil(frequency) : floor(frequency);

          double rounded_frequency = std::round(((lowfrequency + frequency)/1000) * 100.0) / 100.0; 

          bool ison = false;  

          for (i = 0; i < frequencies.size(); i++) { 
            if ((rounded_frequency*1000) == frequencies[i].frequency) { //check whether the frequency is on the list
              frequencies[i].timestamp = gettimestamp();
              ison = true;
            }
          }

          for (i = 0; i < blacklist.size(); i++) { 
            if ((rounded_frequency*1000) == blacklist[i]) { //check whether the frequency is on the blacklist
              ison = true;
            }
          }
      
          if (ison == false) {
            new_frequency.frequency = (rounded_frequency*1000); // add frequency to the list
            new_frequency.timestamp = gettimestamp();
            frequencies.push_back(new_frequency);
          }
         
          //if (config.verbous) debug("take center frequency " + to_string(frequency_list_new[frequency_count_new-1]), false);
        }
        peaks_index_block++;
        i = peaks_index_block;
        if (config.verbous) debug("go ahead on index " + to_string(i), false);
      }
      debug(to_string(frequencies.size()) + " frequencies saved in the list", false);
      string out = "created with dxlAPRS_scanner\n\n";

      for (auto& element : frequencies) { // creating the frequencieslist for dxlAPRS
        debug(to_string(element.frequency) + " " + to_string(element.bandwidth) + " " + to_string(element.timestamp), false);
        string out_frequency = to_string(element.frequency);
        string out_timestamp = to_string(element.timestamp);
        out_timestamp = out_timestamp.insert(2, ":");
        out_timestamp = out_timestamp.insert(5, ":");

        out += "f " + out_frequency.insert(3, ".") + " 5 " + to_string(config.squelch) + " 0 " + to_string(element.bandwidth * 1000) + " # Timestamp " + out_timestamp + "\n";
        
      }

      for (i = 0; i < frequencies.size(); i++) { 
        if ( (gettimestamp() - frequencies[i].timestamp) > config.timer_holding) { //check timestamp and remove from list
          frequencies.erase(frequencies.begin() + i);
        }
      }

      string out_startfrequency = to_string(config.startfrequency/1000);
      string out_stopfrequency = to_string((config.startfrequency/1000)+2000);

      out += "s " + out_startfrequency.insert(3, ".") + " " + out_stopfrequency.insert(3, ".") + " " + to_string(config.steps) + " 6 3000\n";

      ofstream outputfile(config.filename);
      outputfile << out;
      outputfile.close();
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
    if (strcmp(argv[i],"-p") == 0) {
      if(i+1 < argc) {
        config.port = stoi(argv[i+1]);
      } else {
        debug("Error : Port", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-s") == 0) {
      if(i+1 < argc) {
        config.steps = stoi(argv[i+1]);
      } else {
        debug("Error : Steps", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-f") == 0) {
      if(i+1 < argc) {
        config.startfrequency = stoi(argv[i+1]);
      } else {
        debug("Error : Startfrequency", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-o") == 0) {
      if(i+1 < argc) {
        config.filename = argv[i+1];
      } else {
        debug("Error : Filename", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-t") == 0) {
      if(i+1 < argc) {
        config.timer_holding = stoi(argv[i+1]);
      } else {
        debug("Error : Holding timer", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-b") == 0) {
      if(i+1 < argc) {
        config.blacklist = argv[i+1];
      } else {
        debug("Error : Blacklist", false);
        return 0;
      }
    } 
    if (strcmp(argv[i],"-q") == 0) {
      if(i+1 < argc) {
        config.squelch = stoi(argv[i+1]);
      } else {
        debug("Error : Squelch", false);
        return 0;
      }
    } 
  }


    fstream fblacklist;

    fblacklist.open(config.blacklist,ios::in);
    if (fblacklist.is_open()) { 
      string line;
      while(getline(fblacklist, line)) {
        if (line.length() > 1) {
          blacklist.push_back(stoi(line));
          debug(line, false);
        }
      }
    }
  


  thread thread_receivedata (receivedata); // thread for receiving data via udp
  thread thread_getpeaks (getpeaks); //thread for calculating frequencies       

  thread_receivedata.join();                
  thread_getpeaks.join();             
}