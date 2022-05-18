getTextFileExtensions()
{
    echo "c|cpp|cc|cxx|h|hh|ps1|psm1|bash|sh|svg|xml|md|html|css|js|json|txt|desktop"
}

getSourceFileExtensions()
{
    echo "c|cpp|cc|cxx|h|hh|ps1|psm1|bash|sh|js|"
}

getChangedFiles()
{
    extensions="$1"
    
    if [ -z "$extensions" ]
    then
        extensions=`getTextFileExtensions`
    fi
    
    if git rev-parse --verify HEAD >/dev/null 2>&1
    then
        against=HEAD
    else
        # Initial commit: diff against an empty tree object
        against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
    fi

    # We should check only added or modified files of these given types for line endings.
    changed_files=$(git diff-index --cached $against | grep -E '([MA]	.*\.('$extensions')$)' | cut -f 2)

    echo "$changed_files"
}

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


