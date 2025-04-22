#ifndef UTILS_H
#define UTILS_H

#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
// #include <AudioToolbox/AudioToolbox.h>

CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info);

#endif