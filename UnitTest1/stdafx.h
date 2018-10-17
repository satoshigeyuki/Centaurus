// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

// CppUnitTest のヘッダー
#include "CppUnitTest.h"

#if defined(CENTAURUS_BUILD_WINDOWS)
// TODO: プログラムに必要な追加ヘッダーをここで参照してください
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif
