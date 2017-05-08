#pragma once

//#if __cplusplus<201103L
//	#define OFXLIQUIDEVENT_USE_TR1
//#endif

#ifdef OFXCMS_USE_TR1
	#include <tr1/functional>
	#define FUNCTION tr1::function
#else
	#include <functional>
	#define FUNCTION std::function
#endif
