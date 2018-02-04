#ifndef LoggerSD_hpp
#define LoggerSD_hpp

#include <SD.h>

class LoggerSD {
public:
  bool start();
  void stop();
  void print(const char[]);
  void println(const char[]);
  void println(int, int = DEC);
  void print(int, int = DEC);

protected:
  File dataFile;
  bool fileAvailableToWrite = false;
};

#endif
