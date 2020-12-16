
## Building

See below for notes on building the project for both Windows and Unix.


First, install [conan](https://conan.io/).

Then in order for conan to find `thrift` and `grpc`, you need to add a couple of remotes:

```bash
conan remote add helmesjo "https://api.bintray.com/conan/helmesjo/public-conan"
conan remote add inexorgame "https://api.bintray.com/conan/inexorgame/inexor-conan"
```

Finally, run `conan install`
```bash
conan install . -s build_type=Debug --install-folder=cmake-build-debug -o thrift=True|False -o grpc=True|False --build=missing
conan install . -s build_type=Release --install-folder=cmake-build-release -o thrift=True|False -o grpc=True|False --build=missing
```

On Linux you should add `-s compiler.libcxx=libstdc++11` to the command.


### FMI4cpp

```fmi4cpp``` is bundled as a git sub-module, so you'll need to ensure that you are also keeping the FMI4cpp sub-folder up-to-date.
To initialize the sub-module you can run:
```bash
git submodule update --init --recursive
```
To update it, run:
```bash
git submodule update --recursive
```
