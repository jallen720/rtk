#include "ctk3/ctk.h"
using namespace CTK;

#include "rtk/rtk.h"
using namespace RTK;

#include "rtk/tests/shared.h"
#include "rtk/tests/3d/main.h"
// #include "rtk/tests/debug/main.h"

sint32 main()
{
    Test3D::Run();
    // TestDebug::Run();
    return 0;
}
