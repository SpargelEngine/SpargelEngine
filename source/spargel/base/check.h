#pragma once

#include <spargel/base/panic.h>
#include <spargel/config.h>

#define spargel_check(cond) ((cond) ? (void)(0) : spargel_panic("Check `" #cond "` failed."))

#if SPARGEL_ENABLE_ASSERT
#define spargel_dcheck(cond) spargel_check(cond)
#else
#define spargel_dcheck(cond)
#endif
