#include "Logger.h"
#include "StringUtility.h"
#include <iostream>
#include <Windows.h>

namespace Logger {

	void Log(const std::string& message) {

		OutputDebugStringW(StringUtility::ConvertString(message).c_str());
	}
}