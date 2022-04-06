param([switch]$all, [switch]$clear)

$gitDir = git rev-parse --git-commom-dir

if (!(test-path $gitDir)) 
{
   $gitDir = git rev-parse --git-dir
}

$gitDir = (resolve-path ($gitDir)).Path

if (!$gitDir) {
    throw ".git - Directory not found"
}

$gitHooksDir = join-path $gitDir "hooks"

if (!(test-path $gitHooksDir)) {
    throw "$gitHooksDir not present"
}

if($clear) {
    write-host "cleaning old hooks dir: $githooksdir"
    Remove-Item (join-path $githooksdir "*") -force -recurse
}

write-host "providing files:"

if ($all) {
    Copy-Item $PSScriptRoot/optional/* $githooksdir -force -recurse -PassThru | format-table Fullname | out-host
}

Copy-Item $PSScriptRoot/necessary/* $githooksdir -force -recurse -PassThru | format-table Fullname | out-host

if($IsLinux) {
    chmod 755 (join-path $githooksdir "post-merge")
    chmod 755 (join-path $githooksdir "pre-commit")
    chmod 755 (join-path $githooksdir "precommit/check/*")
    chmod 755 (join-path $githooksdir "precommit/modify/*")
}
