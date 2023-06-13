# proxy-fmu

`proxy-fmu` is a tool-independent solution for running FMUs compatible with FMI for Co-simulation version 1.x & 2.x in distributed fashion.
`proxy-fmu` will automatically spawn a new process for each instance that is instantiated from an FMU.

By default, `proxy-fmu` targets `localhost`. For this behaviour zero configuration is required apart from the fact that the `proxyfmu` executable must be available.
It is also possible to make `proxy-fmu` run instances on a remote computer. For this to work, the `proxyfmu-booter` executable must be started on the remote computer and provided with a valid port.

In short, this solution allows you to:
* Import FMI 1.0 models in software that otherwise only supports FMI 2.0.
* Instantiate multiple instances of FMUs that only allows one instance per process.
* The ability to run the FMU on some remote resource
    * Which in turn allows FMUs to run on otherwise unsupported platforms.

The solution is written in C++ and the static library and bundled executables have no dependencies.

Currently, only Windows and Linux are supported, but there are no known obstacles for supporting the Darwin platform.

### How to build

`proxyfmu` is easiest built in conjunction with [vcpkg](https://vcpkg.io/en/index.html).

#### vcpkg (using manifest mode)

Call CMake with `-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake`


For an example on how to build the project using the command line, refer to the [CI setup](https://github.com/open-simulation-platform/proxy-fmu/blob/master/.github/workflows/build.yml).


### How is this project related to `FMU-proxy`?

This library serves similar purpose as [FMU-proxy v.0.6.2](https://github.com/NTNU-IHB/FMU-proxy/releases/tag/v0.6.2). 
However, it requires no JVM and automatically spawns new processes.
The current development of FMU-proxy is about wrapping existing FMUs into new FMUs with network capabilities, so they now serve different purposes. `proxy-fmu` is a library that apps can integrate with and FMU-proxy produces FMI compatible wrapper models.
