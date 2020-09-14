# How to compile
As additional dependency ixwebsocket is needed. Since this is not directly copied into this project, you have to install it yourself. Easiest way to do so ist to use [vcpkg](https://github.com/microsoft/vcpkg).
Commands to run vcpkg and install ixwebsocket:
```
git clone git@github.com:microsoft/vcpkg.git
cd vcpks
.\bootstrap-vcpkg.bat
.\vcpkg.exe install ixwebsocket:x64-windows
```

Then you can simply open Visual Studio and everything should be set up to run.
The default path, where the dll is put into, is `C:\Program Files\Guild Wars 2\bin64\`, so you probably want to change that, depending on your needs.


## arcdps cannot load the generated dll
If you have the problem, that the dll is not loaded by arcdps, you can try to static compile the dll.
It has two ways to do so:
- with installing the static library
- with changing the x64-windows triplet

### install static library
This is the easier way and the official way to do it, but it didnt worked for me (VS didnt found the library files).
1. Uninstall ixwebsocket and its dependencies:
```
.\vcpkg.exe remove ixwebsocket:x64-windows
.\vcpkg.exe remove mbedtls:x64-windows
.\vcpkg.exe remove zlib:x64-windows
```
2. install the static dependencies:
```
.\vcpkg.exe install ixwebsocket:x64-windows-static
```
3. Done

### change behaviour of x64-windows triplet
This is the way i've done it, it is a hacky way to do it, but the only way i got it working:
1. Uninstall ixwebsocket and its dependencies:
```
.\vcpkg.exe remove ixwebsocket:x64-windows
.\vcpkg.exe remove mbedtls:x64-windows
.\vcpkg.exe remove zlib:x64-windows
```
2. Open the x64-windows triplet `\vcpkg\triplets\x64-windows.cmake`
3. Change the `VCPKG_LIBRARY_LINKAGE` Var to static. My file looks like this:
```
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
```
4. Install ixwebsocket again
```
.\vcpkg.exe install ixwebsocket:x64-windows
```
5. Done

# License
## arcdps-char-detector
This project is licenced under the MIT license. See [LICENSE](LICENSE)

## parson
This project uses parson as dependency. Parson is also licenced under the MIT license. Licence is added as header to the files: [parson.h](CurrentChar/parson.h), [parson.c](CurrentChar/parson.c)

## ImGui
This project uses ImGui as dependency. The currently used ImGui Version is 1.50. In [imgui](CurrentChar/imgui) is a clone of the Release Tag 1.50. ImGui is also licenced under the MIT License. 
[License file](CurrentChar/imgui/LICENSE) is also in the imgui directory.
