@echo off
cd /d "%~dp0"
".venv\Scripts\python.exe" teclado\server.py --open
pause
