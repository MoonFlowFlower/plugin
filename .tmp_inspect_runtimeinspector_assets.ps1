$ErrorActionPreference = 'Stop'
$Script = 'D:\project\game\ue\pluginmaker\plugins\runtimeinspector\.tmp_inspect_runtimeinspector_assets.py'
$UProject = 'D:\project\game\ue\pluginmaker\PluginMaker.uproject'
& 'D:\Software\Unreal\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $UProject -Unattended -NullRHI -NoSound -NoSplash -stdout -log ('-ExecutePythonScript=' + $Script)
