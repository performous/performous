#!/bin/bash

clear_hooks=false

while [[ "$1" != "" ]]; do
	case $1 in
		--clear )
			clear_hooks=true
			;;
		* )
			echo "Unknown option: $1"
			exit 1
			;;
	esac
	shift
done

# Determine Git directory
git_dir=$(git rev-parse --git-common-dir 2>/dev/null || git rev-parse --git-dir 2>/dev/null)

if [[ -z "$git_dir" ]]; then
	echo ".git directory not found"
	exit 1
fi

# Resolve absolute path
git_dir=$(realpath "$git_dir")
git_hooks_dir="$git_dir/hooks"

if [[ ! -d "$git_hooks_dir" ]]; then
	echo "$git_hooks_dir not present"
	exit 1
fi

if [[ "$clear_hooks" == true ]]; then
	echo "Cleaning old hooks directory: $git_hooks_dir"
	find "$git_hooks_dir" -type f ! -name "*.sample" -exec rm -f {} +
	exit 0
fi

# Provide files
echo "Providing files:"
cp -r "$PWD/necessary/"* "$git_hooks_dir" && ls -l "$git_hooks_dir"

# Set permissions for Linux
if [[ "$(uname -s)" == "Linux" ]]; then
	chmod 755 "$git_hooks_dir/post-merge"
	chmod 755 "$git_hooks_dir/pre-commit"
	chmod -R 755 "$git_hooks_dir/precommit/check/"
fi

exit 0
