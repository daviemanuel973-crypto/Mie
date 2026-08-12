#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <bcrypt.h>
#include <winhttp.h>

#include <updateProtocol.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#ifndef MIE_VERSION
#define MIE_VERSION "0.4.0"
#endif
#define MIE_WIDEN_INNER(value) L##value
#define MIE_WIDEN(value) MIE_WIDEN_INNER(value)

namespace
{
	constexpr const wchar_t *releaseApiHost = L"api.github.com";
	constexpr const wchar_t *releaseApiPath = L"/repos/daviemanuel973-crypto/Mie/releases/latest";
	constexpr const char *installerAssetName = "Mie-Survival-Windows-x64-Setup.exe";
	constexpr std::uint64_t maxInstallerBytes = 2ull * 1024ull * 1024ull * 1024ull;
	constexpr auto updateCheckInterval = std::chrono::hours(6);

	std::wstring executablePath()
	{
		std::vector<wchar_t> buffer(512);
		for (;;)
		{
			const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (length == 0) { return {}; }
			if (length < buffer.size() - 1) { return std::wstring(buffer.data(), length); }
			buffer.resize(buffer.size() * 2);
		}
	}

	std::wstring utf8ToWide(const std::string &text)
	{
		if (text.empty()) { return {}; }
		const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (size <= 0) { return {}; }
		std::wstring result(size, L'\0');
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			text.data(), static_cast<int>(text.size()), result.data(), size);
		return result;
	}

	void appendLog(const std::filesystem::path &appDirectory, const std::string &message)
	{
		std::error_code error;
		std::filesystem::create_directories(appDirectory / "updates", error);
		std::ofstream log(appDirectory / "updates" / "updater.log", std::ios::app);
		if (log) { log << message << '\n'; }
	}

	std::wstring quoteArgument(const std::wstring &argument)
	{
		std::wstring result = L"\"";
		unsigned int backslashes = 0;
		for (wchar_t c : argument)
		{
			if (c == L'\\')
			{
				++backslashes;
				continue;
			}
			if (c == L'"')
			{
				result.append(backslashes * 2 + 1, L'\\');
				result.push_back(L'"');
				backslashes = 0;
				continue;
			}
			result.append(backslashes, L'\\');
			backslashes = 0;
			result.push_back(c);
		}
		result.append(backslashes * 2, L'\\');
		result.push_back(L'"');
		return result;
	}

	bool startProcess(const std::filesystem::path &executable, const std::wstring &arguments,
		const std::filesystem::path &workingDirectory, bool wait, DWORD *exitCode = nullptr)
	{
		std::wstring commandLine = quoteArgument(executable.wstring());
		if (!arguments.empty()) { commandLine += L" " + arguments; }
		std::vector<wchar_t> writableCommand(commandLine.begin(), commandLine.end());
		writableCommand.push_back(L'\0');

		STARTUPINFOW startup{};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process{};
		const BOOL created = CreateProcessW(executable.c_str(), writableCommand.data(), nullptr, nullptr,
			FALSE, 0, nullptr, workingDirectory.c_str(), &startup, &process);
		if (!created) { return false; }

		if (wait)
		{
			WaitForSingleObject(process.hProcess, INFINITE);
			DWORD code = 1;
			GetExitCodeProcess(process.hProcess, &code);
			if (exitCode) { *exitCode = code; }
		}
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		return true;
	}

	bool queryStatusOk(HINTERNET request)
	{
		DWORD status = 0;
		DWORD size = sizeof(status);
		return WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) && status == 200;
	}

	bool openRequestForUrl(HINTERNET session, const std::wstring &url,
		HINTERNET &connection, HINTERNET &request)
	{
		URL_COMPONENTS parts{};
		parts.dwStructSize = sizeof(parts);
		parts.dwSchemeLength = static_cast<DWORD>(-1);
		parts.dwHostNameLength = static_cast<DWORD>(-1);
		parts.dwUrlPathLength = static_cast<DWORD>(-1);
		parts.dwExtraInfoLength = static_cast<DWORD>(-1);
		if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts)) { return false; }
		if (parts.nScheme != INTERNET_SCHEME_HTTPS) { return false; }

		const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
		std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
		if (parts.dwExtraInfoLength) { path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength); }
		connection = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
		if (!connection) { return false; }
		request = WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
		if (!request) { WinHttpCloseHandle(connection); connection = nullptr; return false; }

		DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
		WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
		const wchar_t *headers = L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
		if (!WinHttpAddRequestHeaders(request, headers, static_cast<DWORD>(-1),
			WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE) ||
			!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
				WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request, nullptr))
		{
			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connection);
			request = nullptr;
			connection = nullptr;
			return false;
		}
		return queryStatusOk(request);
	}

	bool readHttpResponse(HINTERNET request, std::string &data, std::uint64_t maximumBytes)
	{
		data.clear();
		std::vector<char> buffer(64 * 1024);
		for (;;)
		{
			DWORD received = 0;
			if (!WinHttpReadData(request, buffer.data(), static_cast<DWORD>(buffer.size()), &received))
			{
				return false;
			}
			if (received == 0) { return true; }
			if (data.size() + received > maximumBytes) { return false; }
			data.append(buffer.data(), received);
		}
	}

	bool httpGetText(const std::wstring &url, std::string &data, std::uint64_t maximumBytes)
	{
		HINTERNET session = WinHttpOpen(L"MieSurvivalUpdater/" MIE_WIDEN(MIE_VERSION),
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!session) { return false; }
		WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);

		HINTERNET connection = nullptr;
		HINTERNET request = nullptr;
		const bool opened = openRequestForUrl(session, url, connection, request);
		const bool read = opened && readHttpResponse(request, data, maximumBytes);
		if (request) { WinHttpCloseHandle(request); }
		if (connection) { WinHttpCloseHandle(connection); }
		WinHttpCloseHandle(session);
		return read;
	}

	bool downloadFile(const std::wstring &url, const std::filesystem::path &destination,
		std::uint64_t maximumBytes)
	{
		HINTERNET session = WinHttpOpen(L"MieSurvivalUpdater/" MIE_WIDEN(MIE_VERSION),
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!session) { return false; }
		WinHttpSetTimeouts(session, 5000, 5000, 15000, 30000);
		HINTERNET connection = nullptr;
		HINTERNET request = nullptr;
		if (!openRequestForUrl(session, url, connection, request))
		{
			if (request) { WinHttpCloseHandle(request); }
			if (connection) { WinHttpCloseHandle(connection); }
			WinHttpCloseHandle(session);
			return false;
		}

		std::ofstream output(destination, std::ios::binary | std::ios::trunc);
		std::vector<char> buffer(128 * 1024);
		std::uint64_t total = 0;
		bool success = static_cast<bool>(output);
		while (success)
		{
			DWORD received = 0;
			if (!WinHttpReadData(request, buffer.data(), static_cast<DWORD>(buffer.size()), &received))
			{
				success = false;
				break;
			}
			if (received == 0) { break; }
			total += received;
			if (total > maximumBytes) { success = false; break; }
			output.write(buffer.data(), received);
			success = static_cast<bool>(output);
		}
		output.close();
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);
		if (!success || total == 0)
		{
			std::error_code error;
			std::filesystem::remove(destination, error);
			return false;
		}
		return true;
	}

	bool sha256File(const std::filesystem::path &path, std::string &hashText)
	{
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		BCRYPT_HASH_HANDLE hash = nullptr;
		DWORD objectLength = 0;
		DWORD hashLength = 0;
		DWORD readLength = 0;
		if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
			BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
				reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &readLength, 0) < 0 ||
			BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
				reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &readLength, 0) < 0)
		{
			if (algorithm) { BCryptCloseAlgorithmProvider(algorithm, 0); }
			return false;
		}

		std::vector<unsigned char> object(objectLength);
		std::vector<unsigned char> digest(hashLength);
		bool success = BCryptCreateHash(algorithm, &hash, object.data(), objectLength,
			nullptr, 0, 0) >= 0;
		std::ifstream input(path, std::ios::binary);
		std::vector<unsigned char> buffer(1024 * 1024);
		while (success && input)
		{
			input.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
			const auto count = input.gcount();
			if (count > 0 && BCryptHashData(hash, buffer.data(), static_cast<ULONG>(count), 0) < 0)
			{
				success = false;
			}
		}
		success = success && input.eof() && BCryptFinishHash(hash, digest.data(), hashLength, 0) >= 0;
		if (hash) { BCryptDestroyHash(hash); }
		BCryptCloseAlgorithmProvider(algorithm, 0);
		if (!success) { return false; }

		std::ostringstream result;
		result << std::hex << std::setfill('0');
		for (unsigned char byte : digest) { result << std::setw(2) << static_cast<unsigned int>(byte); }
		hashText = result.str();
		return true;
	}

	bool shouldCheckForUpdates(const std::filesystem::path &updatesDirectory)
	{
		std::ifstream input(updatesDirectory / "last-check.txt");
		std::int64_t last = 0;
		input >> last;
		const auto now = std::chrono::system_clock::now();
		const auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		return last <= 0 || nowSeconds - last >=
			std::chrono::duration_cast<std::chrono::seconds>(updateCheckInterval).count();
	}

	void recordUpdateCheck(const std::filesystem::path &updatesDirectory)
	{
		const auto now = std::chrono::system_clock::now();
		const auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		std::ofstream output(updatesDirectory / "last-check.txt", std::ios::trunc);
		if (output) { output << nowSeconds; }
	}

	bool stageLatestUpdate(const std::filesystem::path &appDirectory)
	{
		const auto updatesDirectory = appDirectory / "updates";
		std::error_code filesystemError;
		std::filesystem::create_directories(updatesDirectory, filesystemError);
		if (filesystemError || !shouldCheckForUpdates(updatesDirectory)) { return false; }
		recordUpdateCheck(updatesDirectory);

		std::string releaseJson;
		const std::wstring apiUrl = std::wstring(L"https://") + releaseApiHost + releaseApiPath;
		if (!httpGetText(apiUrl, releaseJson, 2 * 1024 * 1024))
		{
			appendLog(appDirectory, "Update check failed: GitHub release metadata was unavailable.");
			return false;
		}

		WindowsReleaseInfo release;
		std::string parseError;
		if (!parseWindowsReleaseInfo(releaseJson, installerAssetName, release, &parseError))
		{
			appendLog(appDirectory, "Update check failed: " + parseError);
			return false;
		}
		if (compareSemanticVersions(release.version, MIE_VERSION) <= 0) { return false; }

		std::string checksumText;
		std::string expectedHash;
		if (!httpGetText(utf8ToWide(release.checksumUrl), checksumText, 64 * 1024) ||
			!parseSha256Text(checksumText, expectedHash))
		{
			appendLog(appDirectory, "Update check failed: release checksum was unavailable or invalid.");
			return false;
		}

		const auto partPath = updatesDirectory / "Mie-Survival-Windows-x64-Setup.exe.part";
		const auto installerPath = updatesDirectory / installerAssetName;
		std::filesystem::remove(partPath, filesystemError);
		if (!downloadFile(utf8ToWide(release.installerUrl), partPath, maxInstallerBytes))
		{
			appendLog(appDirectory, "Update download failed for " + release.version + ".");
			return false;
		}

		std::string actualHash;
		if (!sha256File(partPath, actualHash) || actualHash != expectedHash)
		{
			std::filesystem::remove(partPath, filesystemError);
			appendLog(appDirectory, "Update rejected: SHA-256 mismatch for " + release.version + ".");
			return false;
		}

		std::filesystem::remove(installerPath, filesystemError);
		std::filesystem::rename(partPath, installerPath, filesystemError);
		if (filesystemError) { return false; }

		const auto pendingTemporary = updatesDirectory / "pending.txt.tmp";
		const auto pending = updatesDirectory / "pending.txt";
		{
			std::ofstream output(pendingTemporary, std::ios::trunc);
			if (!output) { return false; }
			output << release.version << '\n' << expectedHash << '\n';
		}
		std::filesystem::remove(pending, filesystemError);
		std::filesystem::rename(pendingTemporary, pending, filesystemError);
		if (filesystemError) { return false; }
		appendLog(appDirectory, "Update " + release.version + " staged for the next launch.");
		return true;
	}

	bool applyPendingUpdate(const std::filesystem::path &appDirectory)
	{
		const auto updatesDirectory = appDirectory / "updates";
		const auto pending = updatesDirectory / "pending.txt";
		const auto installer = updatesDirectory / installerAssetName;
		std::ifstream input(pending);
		std::string version;
		std::string expectedHash;
		std::getline(input, version);
		std::getline(input, expectedHash);
		std::string normalizedExpectedHash;
		std::string actualHash;
		if (!input || !parseSha256Text(expectedHash, normalizedExpectedHash) ||
			!sha256File(installer, actualHash) || actualHash != normalizedExpectedHash)
		{
			appendLog(appDirectory, "Pending update was rejected because its checksum is invalid.");
			std::error_code error;
			std::filesystem::remove(pending, error);
			std::filesystem::remove(installer, error);
			return false;
		}

		const std::wstring arguments = L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /DIR=" +
			quoteArgument(appDirectory.wstring());
		DWORD exitCode = 1;
		if (!startProcess(installer, arguments, appDirectory, true, &exitCode) || exitCode != 0)
		{
			appendLog(appDirectory, "Installer failed while applying " + version + ".");
			return false;
		}

		std::error_code error;
		std::filesystem::remove(pending, error);
		std::filesystem::remove(installer, error);
		return true;
	}

	bool handOffPendingUpdate(const std::filesystem::path &appDirectory)
	{
		if (!std::filesystem::exists(appDirectory / "updates" / "pending.txt")) { return false; }

		wchar_t temporaryDirectory[MAX_PATH] = {};
		if (!GetTempPathW(MAX_PATH, temporaryDirectory)) { return false; }
		const auto temporaryUpdater = std::filesystem::path(temporaryDirectory) /
			(L"MieUpdater-" + std::to_wstring(GetCurrentProcessId()) + L".exe");
		if (!CopyFileW(executablePath().c_str(), temporaryUpdater.c_str(), FALSE)) { return false; }
		return startProcess(temporaryUpdater, L"--apply " + quoteArgument(appDirectory.wstring()),
			temporaryUpdater.parent_path(), false);
	}

	int runLauncher(const std::filesystem::path &appDirectory)
	{
		if (handOffPendingUpdate(appDirectory)) { return 0; }
		if (!startProcess(appDirectory / "Mie.exe", L"", appDirectory, false))
		{
			MessageBoxW(nullptr, L"Mie.exe could not be started. Reinstall Mie Survival and try again.",
				L"Mie Survival", MB_OK | MB_ICONERROR);
			return 1;
		}

		// The GUI is already open while this console-free launcher checks and
		// downloads in the background. A verified update is applied next launch.
		stageLatestUpdate(appDirectory);
		return 0;
	}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int)
{
	const auto self = std::filesystem::path(executablePath());
	if (self.empty()) { return 1; }
	if (commandLine && std::wstring(commandLine) == L"--self-test")
	{
		return compareSemanticVersions("0.4.0", "0.3.0") > 0 ? 0 : 1;
	}
	if (commandLine && std::wstring(commandLine).rfind(L"--apply", 0) == 0)
	{
		std::wstring appArgument = commandLine + 7;
		while (!appArgument.empty() && std::iswspace(appArgument.front())) { appArgument.erase(appArgument.begin()); }
		if (appArgument.size() >= 2 && appArgument.front() == L'"' && appArgument.back() == L'"')
		{
			appArgument = appArgument.substr(1, appArgument.size() - 2);
		}
		const std::filesystem::path appDirectory(appArgument);
		const bool updateApplied = applyPendingUpdate(appDirectory);
		if (updateApplied)
		{
			startProcess(appDirectory / "MieLauncher.exe", L"", appDirectory, false);
		}
		else
		{
			// Keep a valid failed update for the next user-initiated launch, but
			// start the current game directly so failure cannot create an automatic
			// launcher -> updater -> launcher retry loop.
			startProcess(appDirectory / "Mie.exe", L"", appDirectory, false);
		}
		MoveFileExW(self.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
		return 0;
	}
	return runLauncher(self.parent_path());
}
