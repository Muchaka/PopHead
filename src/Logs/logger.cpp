#include "logger.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

#if defined _MSC_VER
#pragma warning(disable:4996)
#endif

using PopHead::Logs::Logger;
using PopHead::Logs::Log;
using PopHead::Logs::ModuleID;
using PopHead::Logs::LogType;


Logger::Logger()
{
	openTheFile();
}

std::ostream & PopHead::Logs::operator<<(std::ostream & os, const LogType & dt)
{
	switch (dt) {
	case LogType::INFO:
		os << "INFO";
		break;
	case LogType::ERROR:
		os << "ERROR";
		break;
	case LogType::WARNING:
		os << "WARNING";
		break;
	}
	return os;
}

std::ostream& PopHead::Logs::operator<<(std::ostream & os, const ModuleID & dt)
{
	switch (dt)
	{
	case ModuleID::Base:
		os << "BASE";
		break;
	case ModuleID::Logs:
		os << "LOGS";
		break;
	case ModuleID::Music:
		os << "MUSIC";
		break;
	case ModuleID::Sound:
		os << "SOUND";
		break;
	case ModuleID::World:
		os << "WORLD";
		break;
	case ModuleID::Renderer:
		os << "RENDERER";
		break;
	case ModuleID::Physics:
		os << "PHYSICS";
		break;
	case ModuleID::States:
		os << "STATES";
		break;
	case ModuleID::Inputs:
		os << "INPUTS";
		break;
	case ModuleID::Resources:
		os << "RESOURCES";
		break;
	case ModuleID::None:
		os << "         ";
		break;
	}
	return os;
}

void Logger::openTheFile()
{
	std::time_t t = time(0);
	struct tm *now = localtime(&t);
	std::string fileName;

	if (now->tm_mday < 10 && now->tm_mon < 10) fileName += "0" + std::to_string(now->tm_mon + 1) + "." + "0" + std::to_string(now->tm_mday);
	if (now->tm_mday < 10 && now->tm_mon > 9) fileName += "0" + std::to_string(now->tm_mon + 1) + "." + std::to_string(now->tm_mday);
	if (now->tm_mon < 10 && now->tm_mon > 9) fileName += std::to_string(now->tm_mon + 1) + "." + "0" + std::to_string(now->tm_mday);
	if (now->tm_min < 10) fileName += "_" + std::to_string(now->tm_hour) + "-" + "0" + std::to_string(now->tm_min) + "-" + std::to_string(now->tm_sec);
	else fileName += "_" + std::to_string(now->tm_hour) + "-" + std::to_string(now->tm_min) + "-" + std::to_string(now->tm_sec);

	mLogFile.open("logs/log_" + fileName + ".txt", std::ofstream::out | std::ofstream::app);
}


void Logger::writeLog(const Log& log)
{
	if (mLogSettings.shouldThisLogBeWrittenIntoConsole(log))
		writeLogInConsole(log);

	if (mLogSettings.shouldThisLogBeWrittenIntoFile(log))
		saveLogsInFile(log);
}

void Logger::writeLogInConsole(const Log& log)

{
	std::cout  << "[  " << std::left << std::setw(7) << std::to_string(getTimeFromStartOfTheProgram().asSeconds()).erase(5, 4) << "s ]"
		<< " | " << std::setw(7) << std::left << log.type
		<< " | " << std::setw(9) << std::left << log.moduleID
		<< " | " << std::left << log.message << std::endl;
}

void Logger::saveLogsInFile(const Log& log)

{
	mLogFile << "[  " << std::left << std::setw(7) << std::to_string(getTimeFromStartOfTheProgram().asSeconds()).erase(5, 4) << "s ]"
		<< " | " << std::setw(7) << std::left << log.type
		<< " | " << std::setw(9) << std::left << log.moduleID
		<< " | " << std::left << log.message << std::endl
		<< std::flush;
}


sf::Time Logger::getTimeFromStartOfTheProgram()
{
	sf::Time elapsed = mTimeFromStartOfTheProgram.getElapsedTime();
	return elapsed;
}