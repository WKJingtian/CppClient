#include <iostream>
#include <format>
#include <thread>
#include <mutex>
#include <string>
#include <filesystem>
#include <codecvt>
#include <locale>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <stdio.h>

#include <functional>
#include <type_traits>
#include <assert.h>

#include <vector>
#include <queue>

#define PY_SSIZE_T_CLEAN
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include "Net/NetPack.h"
#include "Net/NetPackHandler.h"
#include "Net/Player.h"
#include "Audio/AudioCenter.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#pragma comment(lib, "Ws2_32.lib")

#define AUDIO_OUTPUT_BUFLEN 500