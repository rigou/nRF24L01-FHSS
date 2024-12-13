#pragma once

// Oscilloscope output on SCOPE_GPIO is handled by these macros
// Do not define SCOPE_GPIO if you don't need it
#ifdef SCOPE_GPIO
#define setupScope()	{pinMode(SCOPE_GPIO, OUTPUT); dbprintf("Scope on gpio %d\n", SCOPE_GPIO);}
#define writeScope(x)	digitalWrite(SCOPE_GPIO, (x))
#else
#define setupScope(...)
#define writeScope(...)
#endif

// all debug output is handled by these macros, Serial is never called directly for tracing
// DEBUG_ON : 0=debug off, 1=output to serial, 2=output to serial and optionally bluetooth with dbtprintln()
#ifndef DEBUG_ON
#define DEBUG_ON 0
#endif
#if DEBUG_ON
#define dbprint(...)   Serial.print(__VA_ARGS__)
#define dbprintln(...) Serial.println(__VA_ARGS__)
#define dbprintf(...)  Serial.printf(__VA_ARGS__)
#if DEBUG_ON == 1
// redirect bluetooth output to serial
#define dbtprintln(...) Serial.println(__VA_ARGS__)
#define dbtprintf(...)  Serial.printf(__VA_ARGS__)
#elif DEBUG_ON == 2
#define dbtprintln(...) bt_Writeln(__VA_ARGS__)
#define dbtprintf(...)  bt_Printf(__VA_ARGS__)
#endif
#else
#define dbprint(...)
#define dbprintln(...)
#define dbprintf(...)
#define dbtprintln(...)
#define dbtprintf(...)
#endif

// all trace output is handled by these macros, Serial is never called directly for tracing
// TRACE_ON : 0=trace off, 1=output to serial, 2=output to serial and optionally bluetooth with trbtprintln()
#ifndef TRACE_ON
#define TRACE_ON 0
#endif
#if TRACE_ON
#define trprint(...)   Serial.print(__VA_ARGS__)
#define trprintln(...) Serial.println(__VA_ARGS__)
#define trprintf(...)  Serial.printf(__VA_ARGS__)
#if TRACE_ON == 1
// redirect bluetooth output to serial
#define trbtprintln(...) Serial.println(__VA_ARGS__)
#define trbtprintf(...)  Serial.printf(__VA_ARGS__)
#elif TRACE_ON == 2
#define trbtprintln(...) bt_Writeln(__VA_ARGS__)
#define trbtprintf(...)  bt_Printf(__VA_ARGS__)
#endif
#else
#define trprint(...)
#define trprintln(...)
#define trprintf(...)
#define trbtprintln(...)
#define trbtprintf(...)
#endif

