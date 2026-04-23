# Bee 性能回归跑法（B12）
# 用法：
#   pwsh Scripts/run_bench.ps1              # 仅 Tensor.Bench
#   pwsh Scripts/run_bench.ps1 -IncludeCuda # 同时跑 CUDA.Bench
#   pwsh Scripts/run_bench.ps1 -Filter "BM_Cast.*"
#
# 约定：
# - 构建目录：build-bench（Release）
# - 产物：Benchmarks/baseline/{bench}_{shortSha}.json，该目录被 .gitignore 忽略
# - google-benchmark JSON 便于用 ConvertFrom-Json 做对比脚本

param(
    [switch] $IncludeCuda,
    [string] $Filter = ".*",
    [string] $BuildDir = "build-bench"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

# 拿 git short sha 作为输出标识
$sha = (git rev-parse --short HEAD).Trim()
if (-not $sha) { $sha = "local" }

$outDir = Join-Path $repoRoot "Benchmarks\baseline"
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

function Invoke-OneBench {
    param([string] $Target, [string] $ExePath)

    Write-Host ">>> 构建 $Target (Release)" -ForegroundColor Cyan
    cmake --build $BuildDir --config Release --target $Target | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "$Target 构建失败"
    }

    $outJson = Join-Path $outDir "${Target}_${sha}.json"
    Write-Host ">>> 运行 $Target -> $outJson" -ForegroundColor Cyan
    & $ExePath `
        --benchmark_filter=$Filter `
        --benchmark_format=json `
        --benchmark_out=$outJson
    if ($LASTEXITCODE -ne 0) {
        throw "$Target 运行失败"
    }
    Write-Host "    已写入 $outJson" -ForegroundColor Green
}

Invoke-OneBench -Target "Tensor.Bench" `
    -ExePath (Join-Path $repoRoot "$BuildDir\Benchmarks\Tensor\Tensor.Bench.exe")

if ($IncludeCuda) {
    $cudaExe = Join-Path $repoRoot "$BuildDir\Benchmarks\CUDA\CUDA.Bench.exe"
    if (-not (Test-Path $cudaExe)) {
        Write-Warning "CUDA.Bench 可执行文件不存在，跳过。"
    } else {
        Invoke-OneBench -Target "CUDA.Bench" -ExePath $cudaExe
    }
}

Write-Host ">>> 回归基准采集完成（$sha）" -ForegroundColor Green
