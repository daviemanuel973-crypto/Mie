#pragma once

enum class RuntimeSmokeFrameResult
{
    running,
    passed,
    failed,
};

bool runtimeSmokeRequested();
bool runtimeSmokeReusesExistingWorld();
bool beginRuntimeSmokeTest();
RuntimeSmokeFrameResult runtimeSmokeFramePassed(double frameSeconds);
int finishRuntimeSmokeTest(bool runtimePassed);
