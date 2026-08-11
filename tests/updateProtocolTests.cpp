#include <updateProtocol.h>

#include <cassert>
#include <string>

int main()
{
	assert(compareSemanticVersions("v0.4.0", "0.3.0") > 0);
	assert(compareSemanticVersions("0.4.0", "v0.4.0") == 0);
	assert(compareSemanticVersions("0.4.0-beta.1", "0.4.0") < 0);
	assert(compareSemanticVersions("1.0.0", "0.99.99") > 0);

	const std::string json = R"json({
		"tag_name":"v0.4.1",
		"assets":[
			{"name":"Mie-Survival-Windows-x64-Setup.exe","browser_download_url":"https://example.test/setup.exe"},
			{"name":"Mie-Survival-Windows-x64-Setup.exe.sha256","browser_download_url":"https://example.test/setup.exe.sha256"}
		]
	})json";

	WindowsReleaseInfo release;
	std::string error;
	assert(parseWindowsReleaseInfo(json, "Mie-Survival-Windows-x64-Setup.exe", release, &error));
	assert(release.version == "v0.4.1");
	assert(release.installerUrl == "https://example.test/setup.exe");
	assert(release.checksumUrl == "https://example.test/setup.exe.sha256");

	std::string hash;
	assert(parseSha256Text(
		"ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789  setup.exe\n", hash));
	assert(hash == "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
	assert(!parseSha256Text("not a hash", hash));

	return 0;
}
