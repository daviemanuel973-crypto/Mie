#pragma once

enum class RuntimeSmokeFrameResult
{
    running,
    passed,
    failed,
};

bool runtimeSmokeRequested();
bool beginRuntimeSmokeTest();
RuntimeSmokeFrameResult runtimeSmokeFramePassed();
int finishRuntimeSmokeTest(bool runtimePassed);
