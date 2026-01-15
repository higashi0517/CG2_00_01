#include "Logger.h"
#include <iostream>
#include <Windows.h>

namespace Logger {

	void Log(const std::string& message) {

		OutputDebugStringA(message.c_str());
	}
}