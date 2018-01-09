[CmdletBinding()]
    [OutputType([PSCustomObject])]
    Param
    (
        #Root Path to Install WebRTC Source/Build
        [Parameter(Mandatory=$false,
                   Position=0)]
        $WebRTCFolder = "C:\webrtc-source\"
    )

function Get-ScriptDirectory
{
  $Invocation = (Get-Variable MyInvocation -Scope 1).Value
  Split-Path $Invocation.MyCommand.Path
}

Import-Module BitsTransfer
Add-Type -AssemblyName System.IO.Compression.FileSystem

if((Test-Path ($WebRTCFolder)) -eq $false) {
    New-Item -Path $WebRTCFolder -ItemType Directory -Force 
}

$fullPath = (Resolve-Path -Path $WebRTCFolder).path
