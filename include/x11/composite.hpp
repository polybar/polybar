#pragma once

#include "config.hpp"

#if not WITH_XCOMPOSITE
#error "X Composite extension is disabled..."
#endif

#include <xcb/composite.h>
#include <xpp/proto/composite.hpp>
