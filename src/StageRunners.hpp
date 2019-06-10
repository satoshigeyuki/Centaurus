#pragma once

#include "CodeGenEM64T.hpp"
#include "Stage1Runner.hpp"
#if RECURSIVE
#include "Stage2Runner.hpp"
#include "Stage3Runner.hpp"
#else
#include "Stage2RunnerNonRecursive.hpp"
#include "Stage3RunnerNonRecursive.hpp"
#endif
