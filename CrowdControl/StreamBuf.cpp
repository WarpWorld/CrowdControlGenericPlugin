#include "Streambuf.hpp" 

HANDLE Streambuf::hConsole = nullptr;
std::queue<std::string> Streambuf::queue;
Streambuf::LogType Streambuf::currentLogType = Streambuf::LogType::Log;
