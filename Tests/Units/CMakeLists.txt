
enable_testing()

set(INSTALL_GTEST OFF)
set(gtest_force_shared_crt ON)
find_package(GTest CONFIG REQUIRED)

if (NOT GTest_FOUND)
    message(WARNING "GTest not found, failed to build unit tests.")
    return()
endif ()

include(GoogleTest)

AddTestProgram(CoreTest.cpp "Bee::Core" "Core")
AddTestProgram(TupleTest.cpp "Bee::Container" "Container")

file(GLOB MathSources "Math/*.cpp")
AddTestBundle(MathTests "${MathSources}" "Bee::Math")

file(GLOB PlatformSources "Platform/*.cpp")
AddTestBundle(PlatformTests "${PlatformSources}" "Bee::Bee")

