# bash completion for @APP@
_tawk_complete() {
    local cur prev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    case "$prev" in
        --config|--backup|--restore)  COMPREPLY=( $(compgen -f -- "$cur") ); return ;;
        --backend) COMPREPLY=( $(compgen -W "whatsmeow baileys" -- "$cur") ); return ;;
    esac
    if [ "$COMP_CWORD" -eq 1 ] && [[ "$cur" != -* ]]; then
        COMPREPLY=( $(compgen -W "send tail unread status-line" -- "$cur") )
        return
    fi
    case "${COMP_WORDS[1]}" in
        tail|unread) COMPREPLY=( $(compgen -W "--json" -- "$cur") ); return ;;
        status-line) COMPREPLY=( $(compgen -W "--format" -- "$cur") ); return ;;
        send) return ;;
    esac
    COMPREPLY=( $(compgen -W "--config --backend --debug --doctor --update --reinstall --encrypt --decrypt --change-passphrase --backup --restore --with-media --with-login --yes --version --help" -- "$cur") )
}
complete -F _tawk_complete @APP@
