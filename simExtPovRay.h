#ifndef SIMEXTRAYTRACER_H
#define SIMEXTRAYTRACER_H

#include <simLib/simExp.h>

// The 3 required entry points of the plugin:
SIM_DLLEXPORT unsigned char simStart(void* reservedPointer,int reservedInt);
SIM_DLLEXPORT void simEnd();
SIM_DLLEXPORT void* simMessage(int message,int* auxiliaryData,void* customData,int* replyData);

SIM_DLLEXPORT void simPovRay(int message,void* data);

#endif // SIMEXTRAYTRACER_H
