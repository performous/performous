concatenate()
{
    list="$1"
    element="$2"
    delimiter="\n"
    
    if [ -z "$list" ]
    then
        echo "$element"
    else
        echo -e "$list$delimiter$element"
    fi
}

removeEmptyLines()
{
    echo "$@" | sed '/^$/d'
}

isLinux()
{
    if [ "$OSTYPE" = "linux" ]
    then
        return 0
    fi
    
    return 1
}

isWindows()
{
    if [ "$OSTYPE" = "linux" ]
    then
        return 1
    fi
    
    return 0
}

onLinux()
{
    isLinux && $@
}

onWindows()
{
    isWindows && $@
}

runPSScript()
{
    enter "runPSScript"
    
    if isLinux 
    then
        "pwsh" $@
    else
        "powershell" "-noprofile" $@
    fi
}


