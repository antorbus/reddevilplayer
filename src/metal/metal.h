#ifndef METAL_H
#define METAL_H

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info);

#endif