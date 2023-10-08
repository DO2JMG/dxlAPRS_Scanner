#ifndef	tools_H
#define	tools_H

using namespace std;

class tools
{
        public:
  
        void debug(std::string strData, bool newline);
        void file_write(const string strFilename, string strData);
        void file_write_append(const string strFilename, string strData);
        int gettimestamp();
        vector<std::string> splitString(const std::string& str);
};

#endif