module;

#include <HYPRE.h>
#include <HYPRE_sstruct_ls.h>
#include <HYPRE_sstruct_mv.h>
#include <TECIO.h>
#include <oneapi/tbb.h>
#include <oneapi/tbb/detail/_range_common.h>
#include <spdlog/spdlog.h>

#include <cstdarg>
#include <any>
#include <filesystem>
#include <fstream>
#include <print>
#include <iostream>

export module opflow;
export import ext.amgcl;

#define OPFLOW_INSIDE_MODULE

#include "OpFlow"