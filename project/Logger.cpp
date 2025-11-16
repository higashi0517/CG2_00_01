#include "Logger.h"
#include <iostream>
#include <Windows.h>

namespace Logger {

	void Log(const std::string& message) {

		std::cout << message << std::endl;

		OutputDebugStringA(message.c_str());
	}
}