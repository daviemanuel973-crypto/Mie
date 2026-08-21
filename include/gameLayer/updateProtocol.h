#pragma once

#include <string>

struct WindowsReleaseInfo
{
	std::string version;
	std::string installerUrl;
	std::string checksumUrl;
};

// Returns -1, 0 or 1. Three- and four-component release versions are supported;
// a leading 'v' and prerelease/build suffixes are allowed.
int compareSemanticVersions(const std::string &left, const std::string &right);

bool parseWindowsReleaseInfo(const std::string &githubReleaseJson,
	const std::string &installerAssetName,
	WindowsReleaseInfo &release,
	std::string *errorMessage = nullptr);

bool parseSha256Text(const std::string &text, std::string &lowercaseHash);
