#pragma once

// This file is meant to only have utilities for debugging the application

#include <fstream>
#include <locale>
#include <codecvt>

constexpr char logLocation[] = "C:\\Facultate\\CSSO\\Week6\\log";

std::string ConvertU16String(const char16_t* str) {
	static std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
	return converter.to_bytes(str);
}

void LogString(const char* str, bool clear = false) {
	int mode = clear ? (std::ios::out | std::ios::trunc) : (std::ios::out | std::ios::app);
	std::ofstream fout(logLocation, mode);
	fout << str << "\n";
	fout.flush();
}

void LogString(const char16_t* str, bool clear = false) {
	int mode = clear ? (std::ios::out | std::ios::trunc) : (std::ios::out | std::ios::app);
	std::ofstream fout(logLocation, mode);
	fout << ConvertU16String(str) << "\n";
	fout.flush();
}
