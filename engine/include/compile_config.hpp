#pragma once

#define P7_LOGS_PATH L" /P7.Dir=logs/game/"

#ifdef NDEBUG
	#define P7_SINK L" /P7.Sink=FileTxt"
#else
	#define P7_SINK L" /P7.Sink=Console"
#endif

#ifndef APP_VERSION
	#define MAJOR_VERSION 0
	#define MINOR_VERSION 0
	#define PATCH_VERSION 0
#endif

#define ASSETS_HASH_SPACE 0