#include "utility.h"

double GetTimeStampSecond() {
 std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
 auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
 std::time_t timestamp = tmp.count();
 //std::time_t timestamp = std::chrono::system_clock::to_time_t(tp);
 return timestamp / 1000.0;
}

double GetMod(double s, double a) {
  return s - std::floor(s / a) * a;
}