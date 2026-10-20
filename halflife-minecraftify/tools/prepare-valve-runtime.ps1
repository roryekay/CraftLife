param([ValidateSet('halflife','halflife2')][string]$Game='halflife',[ValidatePattern('^[a-z0-9-]+$')][string]$RuntimeName='')
$ErrorActionPreference='Stop'
$port=Split-Path $PSScriptRoot -Parent
$workspace=Split-Path $port -Parent
$steam='C:/Program Files (x86)/Steam/steamapps/common'
$original=Join-Path $steam $(if($Game -eq 'halflife'){'Half-Life'}else{'Half-Life 2'})
if(!$RuntimeName){$RuntimeName=if($Game -eq 'halflife'){'runtime-halflife'}else{'runtime-halflife2-v2'}}
$runtime=Join-Path $port "local/$RuntimeName"
New-Item -ItemType Directory -Path $runtime -Force | Out-Null
$nodeCommand=Get-Command node -ErrorAction SilentlyContinue
if($nodeCommand){$nodeCommand.Source | Set-Content -LiteralPath (Join-Path $port 'local/node-runtime.txt') -Encoding utf8}
# Independent executable/runtime directory. Owned assets are mounted by junction, never packaged.
Get-ChildItem -LiteralPath $original -File | Where-Object Extension -In '.exe','.dll','.asi' | ForEach-Object {
  $target=Join-Path $runtime $_.Name
  if(!(Test-Path -LiteralPath $target)){Copy-Item -LiteralPath $_.FullName -Destination $target}
}
foreach($folder in $(if($Game -eq 'halflife'){@('valve','valve_hd','platform')}else{@('hl2','platform','steam_input')})){
  $target=Join-Path $runtime $folder
  if(!(Test-Path -LiteralPath $target)){New-Item -ItemType Junction -Path $target -Target (Join-Path $original $folder) | Out-Null}
}
$mod=Join-Path $runtime 'minecraftify'
New-Item -ItemType Directory -Path $mod,(Join-Path $mod 'cfg'),(Join-Path $mod 'bin') -Force | Out-Null
if($Game -eq 'halflife'){
  $unusedProxy=Join-Path $runtime 'opengl32.dll'
  $reviewedProxy=Join-Path $port 'local/reshade-runtime/ReShade32.dll'
  if((Test-Path -LiteralPath $unusedProxy) -and !(Get-Item -LiteralPath $unusedProxy).LinkType -and (Get-FileHash -LiteralPath $unusedProxy).Hash -eq (Get-FileHash -LiteralPath $reviewedProxy).Hash){Remove-Item -LiteralPath $unusedProxy}
  New-Item -ItemType Directory -Path (Join-Path $mod 'dlls'),(Join-Path $mod 'cl_dlls') -Force | Out-Null
  Copy-Item -LiteralPath (Join-Path $port 'local/hl1-build/Release/hl.dll') -Destination (Join-Path $mod 'dlls/hl.dll')
  Copy-Item -LiteralPath (Join-Path $port 'local/hl1-build/Release/client.dll') -Destination (Join-Path $mod 'cl_dlls/client.dll')
  @'
game "Minecraftify Anything - Half-Life"
gamedll "dlls/hl.dll"
fallback_dir "valve"
type "singleplayer_only"
cldll "1"
version "0.2.0-dev"
'@ | Set-Content -LiteralPath (Join-Path $mod 'liblist.gam') -Encoding ascii
  '70' | Set-Content -LiteralPath (Join-Path $runtime 'steam_appid.txt') -Encoding ascii
  $dllName='opengl32.dll'
}else{
  $engineBin=Join-Path $runtime 'bin'
  if((Test-Path -LiteralPath $engineBin) -and (Get-Item -LiteralPath $engineBin).LinkType){throw 'Engine bin must be an isolated ordinary directory. Choose a fresh RuntimeName.'}
  New-Item -ItemType Directory -Path $engineBin -Force | Out-Null
  Get-ChildItem -LiteralPath (Join-Path $original 'bin') -File | ForEach-Object {if(!(Test-Path -LiteralPath (Join-Path $engineBin $_.Name))){Copy-Item -LiteralPath $_.FullName -Destination $engineBin}}
  $assets=($original.Replace('\','/'))
  @"
"GameInfo"
{
  game "Minecraftify Anything - Half-Life 2"
  type singleplayer_only
  FileSystem
  {
    SteamAppId 220
    SearchPaths
    {
      game+mod "|gameinfo_path|."
      mod+mod_write+default_write_path "|gameinfo_path|."
      game "$assets/hl2/hl2_sound_vo_english.vpk"
      game "$assets/hl2/hl2_pak.vpk"
      game "$assets/hl2/hl2_textures.vpk"
      game "$assets/hl2/hl2_sound_misc.vpk"
      game "$assets/hl2/hl2_misc.vpk"
      game "$assets/hl2"
      gamebin "$assets/hl2/bin"
      platform "$assets/platform/platform_misc.vpk"
      platform "$assets/platform"
    }
  }
}
"@ | Set-Content -LiteralPath (Join-Path $mod 'gameinfo.txt') -Encoding ascii
  New-Item -ItemType Directory -Path (Join-Path $mod 'addons') -Force | Out-Null
  Copy-Item -LiteralPath (Join-Path $workspace 'halflife2-minecraftify/local/native-build/Release/MinecraftifyHL2.dll') -Destination (Join-Path $mod 'addons/MinecraftifyHL2.dll')
  $plugin=(Join-Path $mod 'addons/MinecraftifyHL2').Replace('\','/')
  "`"Plugin`" { `"file`" `"$plugin`" }" | Set-Content -LiteralPath (Join-Path $mod 'addons/minecraftify.vdf') -Encoding ascii
  '220' | Set-Content -LiteralPath (Join-Path $runtime 'steam_appid.txt') -Encoding ascii
  $dllName='d3d9.dll'
}
if($Game -eq 'halflife2'){
Copy-Item -LiteralPath (Join-Path $port 'local/reshade-runtime/ReShade32.dll') -Destination (Join-Path $runtime $dllName)
Copy-Item -LiteralPath (Join-Path $port 'local/renderer32/Release/MinecraftifyRenderer.addon32') -Destination $runtime
New-Item -ItemType Directory -Path (Join-Path $runtime 'reshade-shaders/Shaders') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $port 'shaders/Minecraftify.fx') -Destination (Join-Path $runtime 'reshade-shaders/Shaders')
Copy-Item -LiteralPath (Join-Path $port 'third_party/ReShade.fxh') -Destination (Join-Path $runtime 'reshade-shaders/Shaders')
if($Game -eq 'halflife2'){
  Copy-Item -LiteralPath (Join-Path $port 'local/reshade-runtime/ReShade32.dll') -Destination (Join-Path $runtime 'bin/d3d9.dll')
  Copy-Item -LiteralPath (Join-Path $port 'local/renderer32/Release/MinecraftifyRenderer.addon32') -Destination (Join-Path $runtime 'bin')
}
@'
[GENERAL]
EffectSearchPaths=.\reshade-shaders\Shaders\**
TextureSearchPaths=.\reshade-shaders\Textures\**
PresetPath=.\MinecraftifyPreset.ini
[INPUT]
KeyOverlay=36,0,0,0
[OVERLAY]
TutorialProgress=4
'@ | Set-Content -LiteralPath (Join-Path $runtime 'ReShade.ini') -Encoding ascii
@'
Techniques=Minecraftify@Minecraftify.fx
TechniqueSorting=Minecraftify@Minecraftify.fx
[Minecraftify.fx]
DepthGate=0
'@ | Set-Content -LiteralPath (Join-Path $runtime 'MinecraftifyPreset.ini') -Encoding ascii
}
$cfg=@'
// Isolated Minecraftify controls. Native movement/use remain available.
bind "i" ""
bind "f5" ""
bind "f8" ""
bind "f6" ""
bind "f9" ""
bind "f10" ""
bind "PGUP" ""
bind "PGDN" ""
bind "CTRL" ""
bind "1" ""
bind "2" ""
bind "3" ""
bind "4" ""
bind "5" ""
bind "6" ""
bind "7" ""
bind "8" ""
bind "9" ""
bind "q" ""
bind "f" ""
bind "MOUSE1" ""
bind "MOUSE2" ""
bind "e" "+use"
bind "l" "impulse 100"
bind "w" "+forward"
bind "s" "+back"
bind "a" "+moveleft"
bind "d" "+moveright"
bind "SPACE" "+jump"
bind "SHIFT" "+duck"
bind "`" "toggleconsole"
fps_max 60
sv_cheats 1
'@
if($Game -eq 'halflife'){$cfg+="`nm_rawinput 1`ncl_lw 0`nsv_maxspeed 640`ncl_forwardspeed 640`ncl_backspeed 640`ncl_sidespeed 640`n";$cfg | Set-Content -LiteralPath (Join-Path $mod 'minecraftify.cfg') -Encoding ascii}
else{$cfg+="`nr_drawviewmodel 0`n";$cfg | Set-Content -LiteralPath (Join-Path $mod 'cfg/minecraftify.cfg') -Encoding ascii}
if($Game -eq 'halflife2'){
  Copy-Item -LiteralPath (Join-Path $runtime 'ReShade.ini') -Destination (Join-Path $runtime 'bin/ReShade.ini')
  Copy-Item -LiteralPath (Join-Path $runtime 'MinecraftifyPreset.ini') -Destination (Join-Path $runtime 'bin/MinecraftifyPreset.ini')
  New-Item -ItemType Directory -Path (Join-Path $runtime 'bin/reshade-shaders/Shaders') -Force | Out-Null
  Copy-Item -LiteralPath (Join-Path $port 'shaders/Minecraftify.fx'),(Join-Path $port 'third_party/ReShade.fxh') -Destination (Join-Path $runtime 'bin/reshade-shaders/Shaders')
}
Write-Output $runtime
