// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

// TODO: プログラムに必要な追加ヘッダーをここで参照してください
