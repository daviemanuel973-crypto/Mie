#include <updateProtocol.h>

#include <iostream>
#include <string>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed: " #condition " at line " << __LINE__ << "\n"; \
		return 1; \
	} } while (false)
}

int main()
{
	REQUIRE(compareSemanticVersions("v0.4.0", "0.3.0") > 0);
	REQUIRE(compareSemanticVersions("0.4.0", "v0.4.0") == 0);
	REQUIRE(compareSemanticVersions("0.4.0-beta.1", "0.4.0") < 0);
	REQUIRE(compareSemanticVersions("1.0.0", "0.99.99") > 0);
	REQUIRE(compareSemanticVersions("v0.9.3.1", "0.9.3") > 0);
	REQUIRE(compareSemanticVersions("0.9.3.1", "v0.9.3.1") == 0);
	REQUIRE(compareSemanticVersions("0.9.3.1-beta.1", "0.9.3.1") < 0);
	REQUIRE(compareSemanticVersions("0.9.4", "0.9.3.99") > 0);

	const std::string json = R"json({
		"tag_name":"v0.4.1",
		"assets":[
			{"name":"Mie-Survival-Windows-x64-Setup.exe","browser_download_url":"https://example.test/setup.exe"},
			{"name":"Mie-Survival-Windows-x64-Setup.exe.sha256","browser_download_url":"https://example.test/setup.exe.sha256"}
		]
	})json";

	WindowsReleaseInfo release;
	std::string error;
	REQUIRE(parseWindowsReleaseInfo(json, "Mie-Survival-Windows-x64-Setup.exe", release, &error));
	REQUIRE(release.version == "v0.4.1");
	REQUIRE(release.installerUrl == "https://example.test/setup.exe");
	REQUIRE(release.checksumUrl == "https://example.test/setup.exe.sha256");

	std::string hash;
	REQUIRE(parseSha256Text(
		"ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789  setup.exe\n", hash));
	REQUIRE(hash == "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
	REQUIRE(!parseSha256Text("not a hash", hash));

	return 0;
}
