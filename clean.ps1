Get-Content "./pwshbuild.conf" | foreach-object -begin {$h=@{}} -process { $k = [regex]::split($_,'=');
	if(($k[0].CompareTo("") -ne 0) -and ($k[0].StartsWith("[") -ne $True)) { $h.Add($k[0], $k[1])}}
Remove-Item $h.builddir -Recurse -Force 2> $null
Remove-Item cmaster_*.xlsx -Recurse