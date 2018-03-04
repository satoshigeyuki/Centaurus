#pragma once

#include <Windows.h>
#include <stdio.h>

namespace Centaurus
{
#define PROGRAM_UUID L"{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME L"Centaurus.exe"

class IPCMaster
{
    HANDLE m_pipe, m_semaphore;
public:
    IPCMaster()
    {
        wchar_t buf[MAX_PATH];

        _snwprintf(buf, MAX_PATH, LR"(\\.\pipe\%s%s[%u]Master)", PROGRAM_UUID, PROGRAM_NAME, GetCurrentProcessId());

        m_pipe = CreateNamedPipeW(buf, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 4096, 4096, 1000, NULL);

        _snwprintf(buf, MAX_PATH, LR"()");

        m_semaphore = CreateSemaphore(NULL, 0, 256, );
    }
    virtual ~IPCMaster()
    {

    }
};
}