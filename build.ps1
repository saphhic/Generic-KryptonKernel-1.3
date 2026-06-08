param(
    [ValidateSet("auto", "llvm", "gcc")]
    [string]$Toolchain = "auto",
    [string]$ToolPrefix = "i686-elf-",
    [string]$BuildDir = "build",
    [string]$Image = "build\KryptonOS.img"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-Tool {
    param([string[]]$Names)

    foreach ($name in $Names) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }

    $llvmBin = "C:\Program Files\LLVM\bin"
    foreach ($name in $Names) {
        $path = Join-Path $llvmBin $name
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

function Invoke-Tool {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    Write-Host "  $FilePath $($Arguments -join ' ')"
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

function Get-Sectors {
    param([long]$Bytes)

    if ($Bytes -le 0) {
        return 1
    }
    return [int][Math]::Ceiling($Bytes / 512.0)
}

function Copy-Bytes {
    param(
        [byte[]]$Destination,
        [int]$Offset,
        [string]$Source
    )

    $bytes = [IO.File]::ReadAllBytes((Resolve-Path $Source))
    [Array]::Copy($bytes, 0, $Destination, $Offset, $bytes.Length)
}

function Get-Toolchain {
    if ($Toolchain -eq "gcc" -or $Toolchain -eq "auto") {
        $gcc = Resolve-Tool @("$($ToolPrefix)gcc")
        $gxx = Resolve-Tool @("$($ToolPrefix)g++")
        $ld = Resolve-Tool @("$($ToolPrefix)ld")
        $objcopy = Resolve-Tool @("$($ToolPrefix)objcopy")
        if ($gcc -and $gxx -and $ld -and $objcopy) {
            return @{
                Kind = "gcc"
                CC = $gcc
                CXX = $gxx
                LD = $ld
                OBJCOPY = $objcopy
            }
        }
    }

    if ($Toolchain -eq "llvm" -or $Toolchain -eq "auto") {
        $clang = Resolve-Tool @("clang.exe", "clang")
        $clangxx = Resolve-Tool @("clang++.exe", "clang++")
        $ld = Resolve-Tool @("ld.lld.exe", "ld.lld")
        $objcopy = Resolve-Tool @("llvm-objcopy.exe", "llvm-objcopy")
        if ($clang -and $clangxx -and $ld -and $objcopy) {
            return @{
                Kind = "llvm"
                CC = $clang
                CXX = $clangxx
                LD = $ld
                OBJCOPY = $objcopy
            }
        }
    }

    throw "No usable i386 ELF toolchain found. Install LLVM or an i686-elf GCC toolchain, then run .\build.ps1 again."
}

function New-ObjectPath {
    param([string]$Source)

    $safe = $Source -replace "[:\\/\.]", "_"
    return Join-Path $objDir "$safe.o"
}

$tools = Get-Toolchain
Write-Host "Using $($tools.Kind) toolchain."

$objDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $objDir | Out-Null
$imageDir = Split-Path $Image
if ($imageDir) {
    New-Item -ItemType Directory -Force -Path $imageDir | Out-Null
}

$commonCFlags = @(
    "-m32",
    "-ffreestanding",
    "-fno-pic",
    "-fno-pie",
    "-fno-stack-protector",
    "-fno-asynchronous-unwind-tables",
    "-fno-unwind-tables",
    "-DLAWAI_ENABLE_FRAMEBUFFER",
    "-IKryptonKernel"
)

$commonCxxFlags = @(
    "-std=gnu++17",
    "-fno-exceptions",
    "-fno-rtti",
    "-fno-threadsafe-statics"
) + $commonCFlags

if ($tools.Kind -eq "llvm") {
    $commonCFlags = @("--target=i386-unknown-elf") + $commonCFlags
    $commonCxxFlags = @("--target=i386-unknown-elf") + $commonCxxFlags
}

$stageAsmFlags = @("-m32", "-ffreestanding")
if ($tools.Kind -eq "llvm") {
    $stageAsmFlags = @("--target=i386-unknown-elf") + $stageAsmFlags
}

$kernelSources = @(
    "bios\kernel_entry.S",
    "bios\read_input.S",
    "KryptonKernel\main.cpp",
    "KryptonKernel\fbterminal.cpp", # Re-añadir fbterminal.cpp
    "KryptonKernel\startup.cpp",
    "KryptonKernel\drivers\vga\printf.cpp",
    "KryptonKernel\drivers\vga\keymap.cpp",
    "KryptonKernel\drivers\input\ps2_mouse.cpp",
    "KryptonKernel\drivers\frameb\framebufferc.cpp",
    "KryptonKernel\filesystem\vfs.cpp", # VFS es un sistema de archivos, no un terminal
    "KryptonKernel\hal\time\rtc.cpp",
    "KryptonKernel\programs\universalprint.cpp",
    "uslsys\index.cpp",
    "uslsys\ustartup.cpp",
    "uslsys\desktop.cpp",
    "KryptonKernel\hal\comparator.cpp",
    "KryptonKernel\hal\userf\user.cpp",
    "KryptonKernel\drivers\vga\vga.cpp",
    "KryptonKernel\hal\userf\kmalloc.cpp",
    "KryptonKernel\hal\reb_shtdw.cpp",
    "KryptonKernel\hal\runtime_support.cpp",
    "KryptonKernel\hal\time\wait_func.cpp",
    "KryptonKernel\hal\kernelpanic.cpp",
    "uslsys\ui_lib\ui_desktop.cpp",
    "uslsys\ui_lib\ui_window.cpp",
    "uslsys\ui_lib\ui_primitives.cpp",
    "uslsys\ui_lib\ui_font.cpp", # Sistema de fuentes para la UI
    "uslsys\ui_lib\ui_terminal_window.cpp", # Implementación de la ventana de terminal
    "uslsys\ui_lib\fb_externcmds.cpp" # Comandos externos para la terminal de ventana
)

$kernelObjects = @()
foreach ($source in $kernelSources) {
    $object = New-ObjectPath $source
    $kernelObjects += $object
    if ($source.EndsWith(".S")) {
        Invoke-Tool $tools.CC (@("-x", "assembler-with-cpp") + $commonCFlags + @("-c", $source, "-o", $object))
    } else {
        Invoke-Tool $tools.CXX ($commonCxxFlags + @("-c", $source, "-o", $object))
    }
}

$kernelElf = Join-Path $BuildDir "kernel.elf"
$kernelBin = Join-Path $BuildDir "kernel.bin"
Invoke-Tool $tools.LD (@("-m", "elf_i386", "-T", "bios\kernel.ld", "-o", $kernelElf) + $kernelObjects)
Invoke-Tool $tools.OBJCOPY @("-O", "binary", $kernelElf, $kernelBin)

$kernelSize = (Get-Item $kernelBin).Length
$kernelSectors = Get-Sectors $kernelSize

function Build-Stage2 {
    param([int]$KernelStartLba)

    $stage2Obj = Join-Path $objDir "stage2.o"
    $stage2Elf = Join-Path $BuildDir "stage2.elf"
    $stage2Bin = Join-Path $BuildDir "stage2.bin"
    Invoke-Tool $tools.CC (@(
        "-x", "assembler-with-cpp",
        "-DKERNEL_START_LBA=$KernelStartLba",
        "-DKERNEL_SECTORS=$kernelSectors",
        "-DKERNEL_FILE_SIZE=$kernelSize",
        "-c", "bios\stage2.S",
        "-o", $stage2Obj
    ) + $stageAsmFlags)
    Invoke-Tool $tools.LD @("-m", "elf_i386", "-T", "bios\stage2.ld", "-o", $stage2Elf, $stage2Obj)
    Invoke-Tool $tools.OBJCOPY @("-O", "binary", $stage2Elf, $stage2Bin)
    return $stage2Bin
}

$stage2Bin = Build-Stage2 2
$stage2Sectors = Get-Sectors ((Get-Item $stage2Bin).Length)
$kernelStartLba = 1 + $stage2Sectors
$stage2Bin = Build-Stage2 $kernelStartLba
$stage2Sectors = Get-Sectors ((Get-Item $stage2Bin).Length)
$kernelStartLba = 1 + $stage2Sectors

$stage1Obj = Join-Path $objDir "stage1.o"
$stage1Elf = Join-Path $BuildDir "stage1.elf"
$stage1Bin = Join-Path $BuildDir "stage1.bin"
Invoke-Tool $tools.CC (@(
    "-x", "assembler-with-cpp",
    "-DSTAGE2_START_LBA=1",
    "-DSTAGE2_SECTORS=$stage2Sectors",
    "-c", "bios\stage1.S",
    "-o", $stage1Obj
) + $stageAsmFlags)
Invoke-Tool $tools.LD @("-m", "elf_i386", "-T", "bios\stage1.ld", "-o", $stage1Elf, $stage1Obj)
Invoke-Tool $tools.OBJCOPY @("-O", "binary", $stage1Elf, $stage1Bin)

$stage1Size = (Get-Item $stage1Bin).Length
if ($stage1Size -ne 512) {
    throw "stage1 must be exactly 512 bytes, got $stage1Size"
}

$stage2Size = (Get-Item $stage2Bin).Length
$totalSectors = $kernelStartLba + $kernelSectors
$imageSectors = [Math]::Max(2880, $totalSectors)
$imageBytes = New-Object byte[] ($imageSectors * 512)

Copy-Bytes $imageBytes 0 $stage1Bin
Copy-Bytes $imageBytes 512 $stage2Bin
Copy-Bytes $imageBytes ($kernelStartLba * 512) $kernelBin

[IO.File]::WriteAllBytes((Join-Path (Get-Location) $Image), $imageBytes)

Write-Host ""
Write-Host "Image written: $Image"
Write-Host "stage2: $stage2Size bytes ($stage2Sectors sectors), LBA 1"
Write-Host "kernel: $kernelSize bytes ($kernelSectors sectors), LBA $kernelStartLba"
