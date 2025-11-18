@echo off
REM =======================================================
REM  BUILD ALL PROGRAMS (IM SERVER, CLIENT, LOAD BALANCER)
REM =======================================================

echo [*] Compiling IMServer...
C:\msys64\mingw64\bin\g++.exe -std=gnu++17 -O2 im_server.cpp -lws2_32 -o im_server.exe

echo [*] Compiling IMClient...
C:\msys64\mingw64\bin\g++.exe -std=gnu++17 -O2 im_client.cpp -lws2_32 -o im_client.exe

echo [*] Compiling Load Balancer...
C:\msys64\mingw64\bin\g++.exe -std=gnu++17 -O2 load_balancer.cpp -lws2_32 -o load_balancer.exe

echo.
echo [*] Build complete!
echo.
