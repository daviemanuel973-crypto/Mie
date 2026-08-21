#include <updateProtocol.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <cstdlib>

namespace
{
	std::array<int, 4> parseVersionNumbers(const std::string &version)
	{
		std::array<int, 4> result{};
		std::size_t position = (!version.empty() && (version[0] == 'v' || version[0] == 'V')) ? 1 : 0;
		for (std::size_t component = 0; component < result.size(); ++component)
		{
			while (position < version.size() && !std::isdigit(static_cast<unsigned char>(version[position])))
			{
				if (version[position] == '-' || version[position] == '+') { return result; }
				++position;
			}

			long value = 0;
			while (position < version.size() && std::isdigit(static_cast<unsigned char>(version[position])))
			{
				value = std::min<long>(INT_MAX, value * 10 + (version[position] - '0'));
				++position;
			}
			result[component] = static_cast<int>(value);
			if (position >= version.size() || version[position] == '-' || version[position] == '+') { break; }
			if (version[position] == '.') { ++position; }
		}
		return result;
	}

	bool hasPrereleaseSuffix(const std::string &version)
	{
		return version.find('-') != std::string::npos;
	}

	bool decodeJsonStringAt(const std::string &json, std::size_t quote,
		std::string &value, std::size_t &end)
	{
		if (quote >= json.size() || json[quote] != '"') { return false; }
		value.clear();
		for (std::size_t i = quote + 1; i < json.size(); ++i)
		{
			const char c = json[i];
			if (c == '"')
			{
				end = i + 1;
				return true;
			}
			if (c != '\\')
			{
				value.push_back(c);
				continue;
			}

			if (++i >= json.size()) { return false; }
			switch (json[i])
			{
				case '"': value.push_back('"'); break;
				case '\\': value.push_back('\\'); break;
				case '/': value.push_back('/'); break;
				case 'b': value.push_back('\b'); break;
				case 'f': value.push_back('\f'); break;
				case 'n': value.push_back('\n'); break;
				case 'r': value.push_back('\r'); break;
				case 't': value.push_back('\t'); break;
				default: return false; // URLs and asset names do not require \u escapes.
			}
		}
		return false;
	}

	bool findJsonStringValue(const std::string &json, const std::string &key,
		std::size_t start, std::string &value, std::size_t &end)
	{
		const std::string quotedKey = "\"" + key + "\"";
		auto keyPosition = json.find(quotedKey, start);
		if (keyPosition == std::string::npos) { return false; }
		auto colon = json.find(':', keyPosition + quotedKey.size());
		if (colon == std::string::npos) { return false; }
		auto quote = json.find('"', colon + 1);
		return quote != std::string::npos && decodeJsonStringAt(json, quote, value, end);
	}

	bool findAssetUrl(const std::string &json, const std::string &assetName, std::string &url)
	{
		std::size_t position = 0;
		while (position < json.size())
		{
			std::string name;
			std::size_t nameEnd = 0;
			if (!findJsonStringValue(json, "name", position, name, nameEnd)) { return false; }
			position = nameEnd;
			if (name != assetName) { continue; }

			std::size_t urlEnd = 0;
			if (!findJsonStringValue(json, "browser_download_url", nameEnd, url, urlEnd)) { return false; }
			return url.rfind("https://", 0) == 0;
		}
		return false;
	}
}

int compareSemanticVersions(const std::string &left, const std::string &right)
{
	const auto leftNumbers = parseVersionNumbers(left);
	const auto rightNumbers = parseVersionNumbers(right);
	if (leftNumbers < rightNumbers) { return -1; }
	if (leftNumbers > rightNumbers) { return 1; }

	const bool leftPrerelease = hasPrereleaseSuffix(left);
	const bool rightPrerelease = hasPrereleaseSuffix(right);
	if (leftPrerelease != rightPrerelease) { return leftPrerelease ? -1 : 1; }
	return 0;
}

bool parseWindowsReleaseInfo(const std::string &githubReleaseJson,
	const std::string &installerAssetName,
	WindowsReleaseInfo &release,
	std::string *errorMessage)
{
	if (errorMessage) { errorMessage->clear(); }
	release = {};
	std::size_t end = 0;
	if (!findJsonStringValue(githubReleaseJson, "tag_name", 0, release.version, end) ||
		release.version.empty())
	{
		if (errorMessage) { *errorMessage = "The release has no valid tag_name."; }
		return false;
	}

	if (!findAssetUrl(githubReleaseJson, installerAssetName, release.installerUrl))
	{
		if (errorMessage) { *errorMessage = "The Windows installer asset is missing."; }
		return false;
	}

	if (!findAssetUrl(githubReleaseJson, installerAssetName + ".sha256", release.checksumUrl))
	{
		if (errorMessage) { *errorMessage = "The installer checksum asset is missing."; }
		return false;
	}

	return true;
}

bool parseSha256Text(const std::string &text, std::string &lowercaseHash)
{
	lowercaseHash.clear();
	for (std::size_t i = 0; i + 64 <= text.size(); ++i)
	{
		bool valid = true;
		for (std::size_t j = 0; j < 64; ++j)
		{
			if (!std::isxdigit(static_cast<unsigned char>(text[i + j])))
			{
				valid = false;
				break;
			}
		}
		if (!valid) { continue; }
		if (i > 0 && std::isxdigit(static_cast<unsigned char>(text[i - 1]))) { continue; }
		if (i + 64 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 64]))) { continue; }

		lowercaseHash = text.substr(i, 64);
		std::transform(lowercaseHash.begin(), lowercaseHash.end(), lowercaseHash.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return true;
	}
	return false;
}
