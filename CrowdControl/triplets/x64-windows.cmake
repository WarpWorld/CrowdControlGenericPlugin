set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
# Pin dependency builds to the VS2017-era toolset: MSVC 14.44 hits an internal
# compiler error (C1001 in <xstring>) building cpprestsdk. 14.x toolsets are
# ABI-compatible, so the main project can still build with the default toolset.
set(VCPKG_PLATFORM_TOOLSET_VERSION 14.16)
