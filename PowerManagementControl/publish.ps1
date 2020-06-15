Write-Host "Creating publish output..."
dotnet publish -r linux-arm -c Release --self-contained
Write-Host "Deploying..."
scp -r ./bin\Release\netcoreapp3.1\linux-arm\publish pi@192.168.88.100:/home/pi/.pmc