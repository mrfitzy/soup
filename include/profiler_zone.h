#pragma once

#include <tracy/TracyC.h>

#define PROFILER_ZONE_BEGIN(ctx, name) TracyCZoneN(ctx, name, true)

#define PROFILER_ZONE_END(ctx, name) TracyCZoneEnd(ctx)
