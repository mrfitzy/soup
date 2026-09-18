#pragma once

#ifndef TRACY_ENABLE

#define PROFILER_ZONE_BEGIN(ctx, name)
#define PROFILER_ZONE_END(ctx, name)

#else

#include <tracy/TracyC.h>
#define PROFILER_ZONE_BEGIN(ctx, name) TracyCZoneN(ctx, name, true)
#define PROFILER_ZONE_END(ctx, name) TracyCZoneEnd(ctx)

#endif // TRACY_ENABLE
