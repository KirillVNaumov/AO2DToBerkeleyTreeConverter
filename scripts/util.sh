#!/usr/bin/bash


info() { echo -e "\033[0;32m[i] $*\033[0m"; }
warn() { echo -e "\033[0;38:5:166m[w] $*\033[0m"; }
error() { echo -e "\033[0;31m[e] $*\033[0m" >&2; }

check_exit() {
    if [ "$1" -ne 0 ]; then error "$2" && exit "$1"; fi
}

check_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then error "Command '$1' not found, crashing out." && exit 127; fi
}

pstdout() {
    while IFS= read -r line; do
        echo -e "\033[0;32m[i] $line\033[0m"
    done
}
pstderr() {
    while IFS= read -r line; do
        echo -e "\033[0;31m[e] $line\033[0m" >&2
    done
}
pretty() {
    "$@" 1> >(pstdout) 2> >(pstderr)
    return $?
}

prompt() {
    local answer
    local question
    if [ -n "$2" ]; then
        echo -ne "\033[0;38:5:166m[w] $1 \033[0m" >&2
        question="$2"
    else
        question="$1"
    fi

    while true; do
        read -r -p $'\033[35m'"$question [y/n]: "$'\033[0m' answer
        answer="${answer,,}" # convert to lowercase
        case "$answer" in
            y|yes) echo "y"; return ;;
            n|no) echo "n"; return ;;
            *) echo -ne "\033[0;35mInvalid answer '$answer', retrying: \033[0m" >&2 ;;
        esac
    done
}