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
|---|---|---|
|--primary|\<width\>x\<height\>|<b>main display</b><br>Specify Primary Display Resolution<br>default(no option): current resolution|
|--secondary|\<width\>x\<height\>|Specify Secondary Display Resolution<br>default(no option): current resolution|
|--msaa|none|Enable MSAA<br>default(no option): disabled|
|--rot|none|Rotate scrolling image and position.<br>Tearing check for landscape mode resolution.|

## Command Line Usage Example
  - Primary: 2160x3840, Secondary: 1920x1080, MSAA enabled
    ```
    DirectX9ExTest.exe --primary 2160x3840 --secondary 1920x1080 --msaa
    ```

  - Tearing check for landscape mode resolution
    ```
    DirectX9ExTest.exe --primary 3840x2160 --secondary 1920x1080 --msaa --rot
    ```

  - Tearing check for portrait mode resolution
    ```
    DirectX9ExTest.exe --primary 2160x3840 --secondary 1080x1920 --msaa
    ```

