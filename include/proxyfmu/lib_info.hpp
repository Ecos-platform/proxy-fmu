
#ifndef PROXYFMU_LIB_INFO_HPP
#define PROXYFMU_LIB_INFO_HPP

namespace proxyfmu
{

/// Software version
struct version
{
    int major = 0;
    int minor = 0;
    int patch = 0;
};

/// Returns the version of the proxyfmu library.
version library_version();

} // namespace proxyfmu

#endif // PROXYFMU_LIB_INFO_HPP
