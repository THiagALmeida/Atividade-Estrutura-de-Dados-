@echo off
rem Compila com o MSVC (Visual Studio Build Tools). Uso: build.bat  ou  build.bat asan
setlocal
cd /d "%~dp0"
where cl >nul 2>nul && goto compilar
for %%r in ("%ProgramFiles%" "%ProgramFiles(x86)%") do (
  for /d %%v in ("%%~r\Microsoft Visual Studio\*") do (
    for /d %%e in ("%%v\*") do (
      if exist "%%e\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%e\VC\Auxiliary\Build\vcvars64.bat"
    )
  )
)
if not defined VCVARS (echo Visual Studio Build Tools nao encontrado. & exit /b 1)
call "%VCVARS%" >nul
:compilar
if not exist bin mkdir bin
rem /O2: benchmark com otimizacao igual para as duas estruturas
set FLAGS=/nologo /O2 /W4 /WX /utf-8 /std:c11 /D_CRT_SECURE_NO_WARNINGS /Fo:bin\
if "%1"=="asan" set FLAGS=/nologo /Od /W4 /WX /utf-8 /std:c11 /D_CRT_SECURE_NO_WARNINGS /Fo:bin\ /fsanitize=address /Zi
set SRC=src\dados.c src\grafo_matriz.c src\grafo_lista.c src\bench.c src\analise.c
cl %FLAGS% %SRC% src\main.c /Fe:bin\rede.exe || exit /b 1
cl %FLAGS% %SRC% tests\testes.c /Fe:bin\testes.exe || exit /b 1
if "%1"=="asan" copy /y "%VCToolsInstallDir%bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" bin\ >nul
echo Compilado: bin\rede.exe e bin\testes.exe
