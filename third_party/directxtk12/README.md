# DirectXTK12

DirectXTK12 is built outside this repository and only the files required by
this project are vendored here.

Upstream source:
https://github.com/microsoft/DirectXTK12

Imported revision:
e656d54 2026-07-01 Bump github/codeql-action/upload-sarif from 4.36.0 to 4.36.2 (#420)

Build used:
DirectXTK_Desktop_2022_Win10.vcxproj

Configurations:
- Debug|x64 -> lib/Debug/DirectXTK12.lib
- Release|x64 -> lib/Release/DirectXTK12.lib

Headers are copied from the upstream Inc directory into include.
