#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>

namespace
{
	std::filesystem::path crashDirectory()
	{
		return std::filesystem::path("crashLogs");
	}

	std::string timestampForFile()
	{
		SYSTEMTIME time{};
		GetLocalTime(&time);
		std::ostringstream stream;
		stream << std::setfill('0')
			<< std::setw(4) << time.wYear
			<< std::setw(2) << time.wMonth
			<< std::setw(2) << time.wDay << '-'
			<< std::setw(2) << time.wHour
			<< std::setw(2) << time.wMinute
			<< std::setw(2) << time.wSecond;
		return stream.str();
	}

	void writeTextReport(const std::filesystem::path &path,
		const char *reason, EXCEPTION_POINTERS *exceptionInfo)
	{
		std::ofstream report(path, std::ios::out | std::ios::trunc);
		if (!report.is_open()) { return; }

		report << "Mie crash report\n";
#ifdef MIE_VERSION
		report << "version: " << MIE_VERSION << "\n";
#endif
		report << "reason: " << (reason ? reason : "unknown") << "\n";
		report << "process_id: " << GetCurrentProcessId() << "\n";
		report << "thread_id: " << GetCurrentThreadId() << "\n";
		if (exceptionInfo && exceptionInfo->ExceptionRecord)
		{
			report << "exception_code: 0x" << std::hex << std::uppercase
				<< exceptionInfo->ExceptionRecord->ExceptionCode << "\n";
			report << "exception_address: 0x" << reinterpret_cast<std::uintptr_t>(
				exceptionInfo->ExceptionRecord->ExceptionAddress) << "\n";
			report << "exception_flags: 0x" << exceptionInfo->ExceptionRecord->ExceptionFlags << "\n";
		}
	}

	void writeMiniDump(const std::filesystem::path &path, EXCEPTION_POINTERS *exceptionInfo)
	{
		HMODULE dbgHelp = LoadLibraryW(L"DbgHelp.dll");
		if (!dbgHelp) { return; }

		using MiniDumpWriteDumpFn = BOOL (WINAPI *)(HANDLE, DWORD, HANDLE,
			MINIDUMP_TYPE, const PMINIDUMP_EXCEPTION_INFORMATION,
			const PMINIDUMP_USER_STREAM_INFORMATION,
			const PMINIDUMP_CALLBACK_INFORMATION);
		auto miniDumpWriteDump = reinterpret_cast<MiniDumpWriteDumpFn>(
			GetProcAddress(dbgHelp, "MiniDumpWriteDump"));
		if (!miniDumpWriteDump)
		{
			FreeLibrary(dbgHelp);
			return;
		}

		HANDLE file = CreateFileW(path.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ,
			nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION dumpInfo{};
			dumpInfo.ThreadId = GetCurrentThreadId();
			dumpInfo.ExceptionPointers = exceptionInfo;
			dumpInfo.ClientPointers = FALSE;
			miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
				MiniDumpNormal, exceptionInfo ? &dumpInfo : nullptr, nullptr, nullptr);
			CloseHandle(file);
		}
		FreeLibrary(dbgHelp);
	}

	void emitCrashArtifacts(const char *reason, EXCEPTION_POINTERS *exceptionInfo)
	{
		std::error_code error;
		std::filesystem::create_directories(crashDirectory(), error);
		if (error) { return; }

		const std::string stamp = timestampForFile();
		const auto base = crashDirectory() / ("MieCrash-" + stamp);
		writeTextReport(base.string() + ".txt", reason, exceptionInfo);
		writeMiniDump(base.string() + ".dmp", exceptionInfo);
	}

	LONG WINAPI mieUnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo)
	{
		emitCrashArtifacts("unhandled SEH exception", exceptionInfo);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	void mieTerminateHandler()
	{
		emitCrashArtifacts("std::terminate", nullptr);
		std::abort();
	}

	struct CrashDiagnosticsInstaller
	{
		CrashDiagnosticsInstaller()
		{
			SetUnhandledExceptionFilter(mieUnhandledExceptionFilter);
			std::set_terminate(mieTerminateHandler);
		}
	};

	CrashDiagnosticsInstaller installCrashDiagnostics;
}
#endif
