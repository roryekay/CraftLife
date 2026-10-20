param(
  [ValidateSet('halflife','halflife2')][string]$Game='halflife',
  [ValidatePattern('^[a-z0-9_]+$')][string]$Map='',
  [ValidateRange(0,9)][int]$Encounter=0,
  [switch]$WithoutRelay,
  [switch]$Diagnostics
)
$ErrorActionPreference='Stop'
$port=Split-Path $PSScriptRoot -Parent
$runtime=Join-Path $port $(if($Game -eq 'halflife'){'local/runtime-halflife'}else{'local/runtime-halflife2-v2'})
$exe=Join-Path $runtime $(if($Game -eq 'halflife'){'hl.exe'}else{'hl2.exe'})
$processName=if($Game -eq 'halflife'){'hl'}else{'hl2'}
if(Get-Process -Name hl,hl2 -ErrorAction SilentlyContinue){throw 'Close the running Half-Life game before starting another Minecraftify session.'}
$catalog=Get-Content -LiteralPath (Join-Path $port 'adapters/common/moments.json') -Raw | ConvertFrom-Json
if(!$Map){$Map=if($Game -eq 'halflife'){'c1a0'}else{'d1_trainstation_01'}}
if($Map -notin @($catalog.$Game | ForEach-Object map)){throw 'Map is not in the reviewed chapter/boss catalog.'}
if(!(Test-Path -LiteralPath $exe)){throw 'Run prepare-valve-runtime.ps1 first.'}
if(!$WithoutRelay){
  if(Get-NetUDPEndpoint -LocalPort $(if($Game -eq 'halflife'){25621}else{25622}) -ErrorAction SilentlyContinue){throw 'A relay/test already owns the local port.'}
  $nodeRecord=Join-Path $port 'local/node-runtime.txt'
  $node=if(Test-Path -LiteralPath $nodeRecord){(Get-Content -LiteralPath $nodeRecord -Raw).Trim()}else{''}
  if(!$node -or !(Test-Path -LiteralPath $node)){$node=(Get-Command node -ErrorAction Stop).Source}
  & $node -e "if(typeof WebSocket!=='function')process.exit(2)"
  if($LASTEXITCODE){throw 'This relay needs a Node runtime with built-in WebSocket support (Node 22 or newer).'}
}
$arguments="-game minecraftify -windowed -w 1280 -h 720 -novid -console -condebug +exec minecraftify.cfg +map $Map"
if($Game -eq 'halflife'){$arguments='-gl -nofbo '+$arguments}
if($Diagnostics){$arguments='-minecraftify-tests '+$arguments}
$process=Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $runtime -PassThru
$process.Id | Set-Content -LiteralPath (Join-Path $port "local/$Game-native.pid")
if(!$WithoutRelay){
  $relayArguments='"'+(Join-Path $port 'adapters/common/relay.mjs')+'" '+$Game+' --encounter='+$Encounter+' --host-pid='+$process.Id
  try{Start-Process -FilePath $node -ArgumentList $relayArguments -WorkingDirectory $port -WindowStyle Hidden -RedirectStandardOutput (Join-Path $port "local/$Game-relay.log") -RedirectStandardError (Join-Path $port "local/$Game-relay-errors.log") | Out-Null}
  catch{Stop-Process -Id $process.Id -ErrorAction SilentlyContinue;throw}
}
Write-Output "Started $Game, map $Map, PID $($process.Id). Keep either Minecraftify profile running; F8 offers both games."
