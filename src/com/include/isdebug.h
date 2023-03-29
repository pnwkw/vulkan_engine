#ifndef DISPLAY_ISDEBUG_H
#define DISPLAY_ISDEBUG_H


namespace com {
#ifdef NDEBUG
	constexpr bool isDebug = false;
#else
	constexpr bool isDebug = true;
#endif
};

#endif //DISPLAY_ISDEBUG_H
