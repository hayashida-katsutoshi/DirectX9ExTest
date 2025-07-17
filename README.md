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
|--tertiary|\<width\>x\<height\>|Specify Tertiary Display Resolution<br>default(no option): current resolution|
|--msaa|none|Enable MSAA<br>default(no option): disabled|
|--bbcount|0-30(D3DPRESENT_BACK_BUFFERS_MAX_EX)|The value of D3DPRESENT_PARAMETERS::BackBufferCount.<br>default(no option): 2|
|--reboot|\<time\>|Specify reboot interval(sec.)|
|--nowait|none|Call D3D9Ex::Present function immediately.|

## Keyboard Control
|Key|Function|
|---|---|
|Left Arrow|Remove one graphic object|
|Right Arrow|Add one graphic object|
|Up Arrow|Add ten graphic object|
|Down Arrow|Remove ten graphic object|

## Command Line Usage Example
  - Primary: 2160x3840, Secondary: 1920x1080, MSAA enabled
    ```
    DirectX9ExTest.exe --primary 2160x3840 --secondary 1920x1080 --msaa
    ```

  - Change the value of BackBufferCount
    ```
    DirectX9ExTest.exe --bbcount 1
    ```
