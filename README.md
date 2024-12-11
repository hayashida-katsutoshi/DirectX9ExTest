# DirectX9ExTest
## Build
|Environment|Version|
|---|---|
|IDE|Visual Studio 2022|
|Windows SDK|v7.1|
|Direct X SDK|August 2008|

## Debug Settings
### Visual Studio
  - DirectX9ExTest project Property Page > Debug
    |Option|Value|
    |---|---|
    |Working Directory|$(TargetDir)|

## Command Line Options
|Option|Value|Note|
|--primary|<width>x<height>|Specify Primary Display Resolution|
|--secondary|<width>x<height>|Specify Secondary Display Resolution|
|--msaa|none|Enable MSAA|

### e.g.
```
DirectX9ExTest.exe --primary 2160x3840 --secondary 1920x1080 --msaa
```
