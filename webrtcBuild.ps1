. .\config.ps1

Set-Location -Path ($fullPath + "\webrtc-checkout\src")

CMD /C 'ninja -C out/Win32/Debug'
CMD /C 'ninja -C out/x64/Debug'
CMD /C 'ninja -C out/Win32/Release'
CMD /C 'ninja -C out/x64/Release'

$outPath = New-Item -Path ($PSScriptRoot + "\webrtc") -ItemType directory -Force

Set-Location "third_party"
Get-ChildItem -Recurse -Filter "*.h" | ForEach-Object {
    $touchItem = New-Item (Join-Path -Path ($outPath.FullName + "\headers\third_party\") -ChildPath (Resolve-Path -Path $_.FullName -Relative))  -ItemType file -Force
    Copy-Item $_.FullName -Destination $touchItem.FullName -Force
}

Set-Location "../webrtc"
Get-ChildItem -Recurse -Filter "*.h" | ForEach-Object {
    $touchItem = New-Item (Join-Path -Path ($outPath.FullName + "\headers\webrtc\") -ChildPath (Resolve-Path -Path $_.FullName -Relative))  -ItemType file -Force
    Copy-Item $_.FullName -Destination $touchItem.FullName -Force
}

Set-Location "..\out\"
Get-ChildItem -Directory | ForEach-Object {
    Get-ChildItem -Path $_.FullName -Directory | ForEach-Object {
         $libPath = New-Item -Path ($outPath.FullName + "\" + (Resolve-Path -Path $_.FullName -Relative)) -ItemType Directory -Force

        Get-ChildItem -Path $_.FullName -Recurse -Filter "*.pdb" | Where-Object {
            $_.Name -notmatch ".*exe.*" -and $_.Name -notmatch ".*dll.*" } | ForEach-Object {
            $touchItem = New-Item -Path ($libPath.FullName + "\lib") -Name $_.Name  -ItemType file -Force
            Copy-Item $_.FullName -Destination $touchItem.FullName -Force
        }
		
        Get-ChildItem -Path $_.FullName -Recurse -Filter "*.lib" | Where-Object {
            $_.Name -match "common_video.lib|ffmpeg.dll.lib|webrtc.lib|boringssl_asm.lib|boringssl.dll.lib|field_trial_default.lib|metrics_default.lib|protobuf_full.lib|libyuv.lib" } | ForEach-Object {
            $touchItem = New-Item -Path ($libPath.FullName + "\lib") -Name $_.Name  -ItemType file -Force
            Copy-Item $_.FullName -Destination $touchItem.FullName -Force
        }
		
        Get-ChildItem -Path $_.FullName -Recurse -File | Where-Object {
            $_.Name -match "\.exe|\.exe\.pdb" } | ForEach-Object {
            $touchItem = New-Item -Path ($libPath.FullName + "\exe") -Name $_.Name -ItemType file -Force
            Copy-Item $_.FullName -Destination $touchItem.FullName -Force
        }
		
        Get-ChildItem -Path $_.FullName -Recurse -Filter "*.dll" | Where-Object {
            $_.Name -notmatch "api-ms-win.*|API-MS-WIN.*|vc.*"} | ForEach-Object {
            $touchItem = New-Item -Path ($libPath.FullName + "\dll") -Name $_.Name  -ItemType file -Force
            Copy-Item $_.FullName -Destination $touchItem.FullName -Force
        }
    }
}

Set-Location $PSScriptRoot
