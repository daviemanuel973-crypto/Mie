#include <gameplay/fieldGuide.h>

namespace
{
	GuideProgress clientGuideProgress = {};
}

GuideProgress getClientGuideProgress()
{
	GuideProgress result = clientGuideProgress;
	result.sanitize();
	return result;
}

void setClientGuideProgress(GuideProgress progress)
{
	progress.sanitize();
	clientGuideProgress = progress;
}

void resetClientGuideProgress()
{
	clientGuideProgress = {};
}
