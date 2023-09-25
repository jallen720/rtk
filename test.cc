// #define USE_ORIGINAL

#include "ctk3/ctk3.h"

#ifdef USE_ORIGINAL
#include "rtk/tests/3d/main.h"
#include "rtk/tests/debug/main.h"
#else
#include "rtk/tests/3d/main_2.h"
#include "rtk/tests/debug/main_2.h"
#endif

sint32 main()
{
    // Test3D::Run();
    TestDebug::Run();
    return 0;
}
