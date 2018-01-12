# WebRTC video frame metadata example
This example shows how to pass real time metadata synchronized with video frames using [RTP Header Extensions](https://tools.ietf.org/html/rfc5285).

![webrtc-metadata](https://user-images.githubusercontent.com/9741323/34884551-0d8123c8-f78b-11e7-87a0-7c82f6fa160f.JPG)

## How to build

These steps will ensure your development environment is configured properly, and then they'll walk you through the process of building the code.

### Prerequisites 

+ Windows 10 Anniversary Update
+ [Visual Studio 2015 Update 3](https://www.visualstudio.com/en-us/news/releasenotes/vs2015-update3-vs)
+ [Windows 10 SDK - 10.0.14393.795](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive)

### Downloading WebRTC source code

> Note: Before running `webrtcSetup.ps1` script, please ensure PowerShell is set to [enable unrestricted script execution](https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_execution_policies?view=powershell-5.1&viewFallbackFrom=powershell-Microsoft.PowerShell.Core).

Open `.\config.ps1` and modify *WebRTCFolder*, which will store WebRTC source code, if needed.

Run `.\webrtcSetup.ps1` from the Windows PowerShell command line. This will download and configure the following:

+ [Chromium m58 release](https://chromium.googlesource.com/chromium/src/+/2b7c19d3)
+ [This patch](https://github.com/phongcao/webrtc-metadata-example/blob/master/metadata.patch), which adds metadata id to video frame

Note: For some reason, if you don't want to apply the metadata patch, run `.\webrtcSetupNoPatch.ps1` instead.

### Building WebRTC libraries

Run `.\webrtcBuild.ps1` from the Windows PowerShell command line. This will build 32bit and 64bit Debug, Release, Exes, Dlls and PDBs of WebRTC.

### The actual build

+ Open [the WebRTCMetadataExample solution](./WebRTCMetadataExample.sln) in Visual Studio
+ Build the solution (Build -> Build Solution) in the desired configuration (Build -> Configuration Manager -> Dropdowns at the top)
+ Done!

## Build output

After you've built the solution, you'll likely want to start one sample sender implementation, and one sample receiver implementation. These are applications demonstrate the synchronization between video frame and metadata.

> Note: you can click on and hold the window title bar, the numbers will freeze for a second so that you can read them.

To run locally, remember to start `peerconnection_server.exe`, the signaling server, before the sender and receiver apps.

## Metadata Id and data channel

Using video frame metadata id, you can easily send additional data via WebRTC data channel and synchronize between sender and receiver.

## Known issues

The example uses VP8 encoder/decoder but it's straightforward to support VP9 and H264 codecs.

## License

MIT
