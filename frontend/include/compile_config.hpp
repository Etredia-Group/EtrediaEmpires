#pragma once

#ifdef NDEBUG
	#define P7_SINK L"/P7.Sink=FileTxt"
#else
	#define P7_SINK L"/P7.Sink=Console"
#endif

#ifndef APP_VERSION
	#define MAJOR_VERSION 0
	#define MINOR_VERSION 0
	#define PATCH_VERSION 0
#endif