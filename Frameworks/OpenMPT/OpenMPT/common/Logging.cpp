/*
 * Logging.cpp
 * -----------
 * Purpose: General logging
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "Logging.h"
#include "mptFileIO.h"
#if defined(MODPLUG_TRACKER)
#include <atomic>
#endif
#include "version.h"

#include <iostream>

#include <cstdarg>
#include <cstring>

#include <stdarg.h>


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{
namespace log
{
	

#ifndef NO_LOGGING



#if !defined(MPT_LOG_GLOBAL_LEVEL_STATIC)
#if defined(MPT_LOG_GLOBAL_LEVEL)
int GlobalLogLevel = static_cast<int>(MPT_LOG_GLOBAL_LEVEL);
#else
int GlobalLogLevel = static_cast<int>(LogDebug);
#endif
#endif



#if defined(MODPLUG_TRACKER) && !defined(MPT_LOG_IS_DISABLED)

bool FileEnabled = false;
bool DebuggerEnabled = true;
bool ConsoleEnabled = false;

static char g_FacilitySolo[1024] = {0};
static char g_FacilityBlocked[1024] = {0};

void SetFacilities(const std::string &solo, const std::string &blocked)
{
	std::strcpy(g_FacilitySolo, solo.c_str());
	std::strcpy(g_FacilityBlocked, blocked.c_str());
}

bool IsFacilityActive(const char *facility)
{
	if(facility)
	{
		if(std::strlen(g_FacilitySolo) > 0)
		{
			if(std::strcmp(facility, g_FacilitySolo) != 0)
			{
				return false;
			}
		}
		if(std::strlen(g_FacilityBlocked) > 0)
		{
			if(std::strcmp(facility, g_FacilitySolo) == 0)
			{
				return false;
			}
		}
	}
	return true;
}

#endif


void Logger::SendLogMessage(const mpt::source_location &loc, LogLevel level, const char *facility, const mpt::ustring &text)
{
#ifdef MPT_LOG_IS_DISABLED
	MPT_UNREFERENCED_PARAMETER(loc);
	MPT_UNREFERENCED_PARAMETER(level);
	MPT_UNREFERENCED_PARAMETER(facility);
	MPT_UNREFERENCED_PARAMETER(text);
#else // !MPT_LOG_IS_DISABLED
	MPT_MAYBE_CONSTANT_IF(mpt::log::GlobalLogLevel < level)
	{
		return;
	}
	#if defined(MODPLUG_TRACKER)
		if(!IsFacilityActive(facility))
		{
			return;
		}
	#else // !MODPLUG_TRACKER
		MPT_UNREFERENCED_PARAMETER(facility);
	#endif // MODPLUG_TRACKER
	// remove eol if already present and add log level prefix
	const mpt::ustring message = LogLevelToString(level) + U_(": ") + mpt::String::RTrim(text, U_("\r\n"));
	const mpt::ustring file = mpt::ToUnicode(mpt::CharsetSource, loc.file_name() ? loc.file_name() : "");
	const mpt::ustring function = mpt::ToUnicode(mpt::CharsetSource, loc.function_name() ? loc.function_name() : "");
	const mpt::ustring line = mpt::ufmt::dec(loc.line());
	#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)
#if MPT_OS_WINDOWS
		static uint64 s_lastlogtime = 0;
		uint64 cur = mpt::Date::ANSI::Now();
		uint64 diff = cur/10000 - s_lastlogtime;
		s_lastlogtime = cur/10000;
#else
		uint64 cur = 0;
		uint64 diff = 0;
#endif
		if(mpt::log::FileEnabled)
		{
			static mpt::ofstream s_logfile;
			if(!s_logfile)
			{
				s_logfile.open(P_("mptrack.log"), std::ios::app);
			}
			if(s_logfile)
			{
				mpt::IO::WriteText(s_logfile, mpt::ToCharset(mpt::CharsetLogfile, mpt::format(U_("%1+%2 %3(%4): %5 [%6]\n"))
					( mpt::Date::ANSI::ToUString(cur)
					, mpt::ufmt::right(6, mpt::ufmt::dec(diff))
					, file
					, line
					, message
					, function
					)));
				mpt::IO::Flush(s_logfile);
			}
		}
		if(mpt::log::DebuggerEnabled)
		{
			OutputDebugStringW(mpt::ToWide(mpt::format(U_("%1(%2): +%3 %4 [%5]\n"))
				( file
				, line
				, mpt::ufmt::right(6, mpt::ufmt::dec(diff))
				, message
				, function
				)).c_str());
		}
		if(mpt::log::ConsoleEnabled)
		{
			static bool consoleInited = false;
			if(!consoleInited)
			{
				AllocConsole();
				consoleInited = true;
			}
			std::wstring consoletext = mpt::ToWide(message) + L"\r\n";
			DWORD dummy = 0;
			WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), consoletext.c_str(), mpt::saturate_cast<DWORD>(consoletext.length()), &dummy, NULL);
		}
	#elif defined(MODPLUG_TRACKER) && defined(MPT_BUILD_WINESUPPORT)
		std::clog
			<< "NativeSupport: "
			<< mpt::ToCharset(mpt::CharsetStdIO, file) << "(" << mpt::ToCharset(mpt::CharsetStdIO, line) << ")" << ": "
			<< mpt::ToCharset(mpt::CharsetStdIO, message)
			<< " [" << mpt::ToCharset(mpt::CharsetStdIO, function) << "]"
			<< std::endl;
	#else // !MODPLUG_TRACKER
		std::clog
			<< "libopenmpt: "
			<< mpt::ToCharset(mpt::CharsetStdIO, file) << "(" << mpt::ToCharset(mpt::CharsetStdIO, line) << ")" << ": "
			<< mpt::ToCharset(mpt::CharsetStdIO, message)
			<< " [" << mpt::ToCharset(mpt::CharsetStdIO, function) << "]"
			<< std::endl;
	#endif // MODPLUG_TRACKER
#endif // MPT_LOG_IS_DISABLED
}




#endif // !NO_LOGGING



#if defined(MODPLUG_TRACKER)

namespace Trace {

#if MPT_OS_WINDOWS

// Debugging functionality will use simple globals.

std::atomic<bool> g_Enabled = ATOMIC_VAR_INIT(false);

static bool g_Sealed = false;

struct Entry {
	uint32       Index;
	uint32       ThreadId;
	uint64       Timestamp;
	const char * Function;
	const char * File;
	int          Line;
	Direction    Direction;
};

static MPT_FORCEINLINE bool operator < (const Entry &a, const Entry &b) noexcept
{
/*
	return false
		|| (a.Timestamp < b.Timestamp)
		|| (a.ThreadID < b.ThreadID)
		|| (a.File < b.File)
		|| (a.Line < b.Line)
		|| (a.Function < b.Function)
		;
*/
	return false
		|| (a.Index < b.Index)
		;
}

static std::vector<mpt::log::Trace::Entry> Entries;

static std::atomic<uint32> NextIndex(0);

static uint32 ThreadIdGUI = 0;
static uint32 ThreadIdAudio = 0;
static uint32 ThreadIdNotify = 0;
static uint32 ThreadIdWatchdir = 0;

void Enable(std::size_t numEntries)
{
	if(g_Sealed)
	{
		return;
	}
	Entries.clear();
	Entries.resize(numEntries);
	NextIndex.store(0);
	g_Enabled = (numEntries > 0);
}

void Disable()
{
	if(g_Sealed)
	{
		return;
	}
	g_Enabled = false;
}

MPT_NOINLINE void Trace(const mpt::source_location & loc, Direction direction) noexcept
{
	// This will get called in realtime contexts and hot paths.
	// No blocking allowed here.
	const uint32 index = NextIndex.fetch_add(1);
	const std::size_t numEntries = Entries.size();
#if 1
	LARGE_INTEGER time;
	time.QuadPart = 0;
	QueryPerformanceCounter(&time);
	const uint64 timestamp = time.QuadPart;
#else
	FILETIME time = FILETIME();
	GetSystemTimeAsFileTime(&time);
	const uint64 timestamp = (static_cast<uint64>(time.dwHighDateTime) << 32) | (static_cast<uint64>(time.dwLowDateTime) << 0);
#endif
	const uint32 threadid = static_cast<uint32>(GetCurrentThreadId());
	mpt::log::Trace::Entry & entry = Entries[index % numEntries];
	entry.Index = index;
	entry.ThreadId = threadid;
	entry.Timestamp = timestamp;
	entry.Function = loc.function_name();
	entry.File = loc.file_name();
	entry.Line = loc.line();
	entry.Direction = direction;
}

void Seal()
{
	if(!g_Enabled)
	{
		return;
	}
	g_Enabled = false;
	g_Sealed = true;
	uint32 count = NextIndex.fetch_add(0);
	if(count < Entries.size())
	{
		Entries.resize(count);
	}
}

bool Dump(const mpt::PathString &filename)
{
	if(!g_Sealed)
	{
		return false;
	}

	LARGE_INTEGER qpcNow;
	qpcNow.QuadPart = 0;
	QueryPerformanceCounter(&qpcNow);
	uint64 ftNow = mpt::Date::ANSI::Now();

	// sort according to index in case of overflows
	std::stable_sort(Entries.begin(), Entries.end());

	mpt::ofstream f(filename);

	f << "Build: OpenMPT " << mpt::ToCharset(mpt::CharsetLogfile, Build::GetVersionStringExtended()) << std::endl;

	bool qpcValid = false;

	LARGE_INTEGER qpcFreq;
	qpcFreq.QuadPart = 0;
	QueryPerformanceFrequency(&qpcFreq);
	if(qpcFreq.QuadPart > 0)
	{
		qpcValid = true;
	}

	f << "Dump: " << mpt::ToCharset(mpt::CharsetLogfile, mpt::Date::ANSI::ToUString(ftNow)) << std::endl;
	f << "Captured events: " << Entries.size() << std::endl;
	if(qpcValid && (Entries.size() > 0))
	{
		double period = static_cast<double>(Entries[Entries.size() - 1].Timestamp - Entries[0].Timestamp) / static_cast<double>(qpcFreq.QuadPart);
		double eventsPerSecond = Entries.size() / period;
		f << "Period [s]: " << mpt::fmt::fix(period) << std::endl;
		f << "Events/second: " << mpt::fmt::fix(eventsPerSecond) << std::endl;
	}

	for(auto &entry : Entries)
	{
		if(!entry.Function) entry.Function = "";
		if(!entry.File) entry.File = "";
		std::string time;
		if(qpcValid)
		{
			time = mpt::ToCharset(mpt::CharsetLogfile, mpt::Date::ANSI::ToUString( ftNow - static_cast<int64>( static_cast<double>(qpcNow.QuadPart - entry.Timestamp) * (10000000.0 / static_cast<double>(qpcFreq.QuadPart) ) ) ) );
		} else
		{
			time = mpt::format("0x%1")(mpt::fmt::hex0<16>(entry.Timestamp));
		}
		f << time;
		if(entry.ThreadId == ThreadIdGUI)
		{
			f << " -----GUI ";
		} else if(entry.ThreadId == ThreadIdAudio)
		{
			f << " ---Audio ";
		} else if(entry.ThreadId == ThreadIdNotify)
		{
			f << " --Notify ";
		} else if(entry.ThreadId == ThreadIdWatchdir)
		{
			f << " WatchDir ";
		} else
		{
			f << " " << mpt::fmt::hex0<8>(entry.ThreadId) << " ";
		}
		f << (entry.Direction == mpt::log::Trace::Direction::Enter ? ">" : entry.Direction == mpt::log::Trace::Direction::Leave ? "<" : " ") << " ";
		f << entry.File << "(" << entry.Line << "): " << entry.Function;
		f << std::endl;
	}
	return true;
}

void SetThreadId(mpt::log::Trace::ThreadKind kind, uint32 id)
{
	if(id == 0)
	{
		return;
	}
	switch(kind)
	{
		case ThreadKindGUI:
			ThreadIdGUI = id;
			break;
		case ThreadKindAudio:
			ThreadIdAudio = id;
			break;
		case ThreadKindNotify:
			ThreadIdNotify = id;
			break;
		case ThreadKindWatchdir:
			ThreadIdWatchdir = id;
			break;
	}
}

uint32 GetThreadId(mpt::log::Trace::ThreadKind kind)
{
	uint32 result = 0;
	switch(kind)
	{
		case ThreadKindGUI:
			result = ThreadIdGUI;
			break;
		case ThreadKindAudio:
			result = ThreadIdAudio;
			break;
		case ThreadKindNotify:
			result = ThreadIdNotify;
			break;
		case ThreadKindWatchdir:
			result = ThreadIdWatchdir;
			break;
	}
	return result;
}

#endif // MPT_OS_WINDOWS

} // namespace Trace

#endif // MODPLUG_TRACKER


} // namespace log
} // namespace mpt


OPENMPT_NAMESPACE_END
