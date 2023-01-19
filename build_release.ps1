Get-Content "./pwshbuild.conf" | foreach-object -begin {$h=@{}} -process { $k = [regex]::split($_,'=');
	if(($k[0].CompareTo("") -ne 0) -and ($k[0].StartsWith("[") -ne $True)) { $h.Add($k[0], $k[1])}}
if (!(Test-Path -Path $h.builddir)) { ./config_cmake.ps1 }
Remove-Item managekinetic.exe 2> $null
cmake . -DBUILDTYPE_RELEASE=TRUE -B $h.builddir
cmake --build $h.builddir --config Release
Copy-Item build\bin\Release\managekinetic.exe managekinetic.exe
