@echo off
echo ========================================
echo Saaya AI - Git Setup and Push Script
echo ========================================
echo.

cd /d "%~dp0"

echo Step 1: Configuring Git...
git config user.email "saaya@example.com"
git config user.name "Saaya Developer"
echo Done!
echo.

echo Step 2: Adding all files...
git add .
echo Done!
echo.

echo Step 3: Creating initial commit...
git commit -m "Initial commit: Saaya AI - Offline Android AI Chat App with Llama 3.1 8B support"
echo Done!
echo.

echo Step 4: Adding remote repository...
git remote remove origin 2>nul
git remote add origin https://github.com/04162214027/saaya1.git
echo Done!
echo.

echo Step 5: Renaming branch to main...
git branch -M main
echo Done!
echo.

echo Step 6: Pushing to GitHub...
git push -u origin main
echo.

echo ========================================
echo Git setup complete!
echo ========================================
pause
