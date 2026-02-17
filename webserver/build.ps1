# Build script for Windows using PowerShell
# Run this with: .\build.ps1

Write-Host "Building WebServer HTTP Components..." -ForegroundColor Green

# Check if g++ is available
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    Write-Host "Error: g++ not found. Please install MinGW or similar." -ForegroundColor Red
    exit 1
}

# Compile source files
Write-Host "Compiling source files..." -ForegroundColor Yellow

$sources = @(
    "srcs\HttpRequest.cpp",
    "srcs\HttpResponse.cpp",
    "srcs\StaticFileHandler.cpp",
    "srcs\UploadHandler.cpp",
    "tests\test_http.cpp"
)

$cxxflags = "-Wall -Wextra -std=c++98 -Iincludes"

# Create objs directory
if (-not (Test-Path "objs")) {
    New-Item -ItemType Directory -Path "objs" | Out-Null
}

# Clean old build
if (Test-Path "test_http.exe") {
    Remove-Item "test_http.exe"
}
Remove-Item "objs\*.o" -ErrorAction SilentlyContinue

# Compile
$cmd = "g++ $cxxflags -o test_http.exe " + ($sources -join " ")
Write-Host "Running: $cmd" -ForegroundColor Cyan

Invoke-Expression $cmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild successful!" -ForegroundColor Green
    Write-Host "Executable: test_http.exe" -ForegroundColor Green
    
    # Create directories if they don't exist
    if (-not (Test-Path "www")) {
        New-Item -ItemType Directory -Path "www" | Out-Null
        Write-Host "Created www/ directory" -ForegroundColor Yellow
    }
    
    if (-not (Test-Path "uploads")) {
        New-Item -ItemType Directory -Path "uploads" | Out-Null
        Write-Host "Created uploads/ directory" -ForegroundColor Yellow
    }
    
    Write-Host "`nTo run tests: .\test_http.exe" -ForegroundColor Cyan
    Write-Host "To run Python tests: python tests\test_parser.py" -ForegroundColor Cyan
} else {
    Write-Host "`nBuild failed!" -ForegroundColor Red
    exit 1
}
