#!/usr/bin/env bash

[[ $# -eq 1 ]] || [[ $# -eq 2 ]] || {
  echo "Usage: $0 <config_file>"; exit 1
}

function main
{
  local readfrom="$1" ; shift
  local writeto="${readfrom}.patched"
  local reverse='false'

  [[ -e $readfrom ]] || {
    echo "Invalid file specified..."; exit 1
  }

  [[ $# -gt 0 ]] && [[ $1 == "reverse" ]] && {
    reverse='true'
    writeto="${readfrom}.reversed"
  }

  if $reverse; then
    sed -r ':SUBSTITUTE_COLON_IN_PARAM s/^([^=]+)[-]([^=]+)+(\s?=.*)/\1:\2\3/g ; t SUBSTITUTE_COLON_IN_PARAM' "$readfrom" |\
      sed -r ':SUBSTITUTE_COLON_IN_VALUE_1 s/(.*\s?=\s?)(.*)<([^ ]*)-([^ ]*)>/\1\2<\3:\4>/g ; t SUBSTITUTE_COLON_IN_VALUE_1' |\
      sed -r ':SUBSTITUTE_COLON_IN_VALUE_2 s/(.*\s?=\s?)(.*)\$\{([^ ]*)-([^ ]*)\}/\1\2\$\{\3:\4\}/g ; t SUBSTITUTE_COLON_IN_VALUE_2' |\
      sed -r 's/^(;\s?|#\s?)(vim)[-](ft=dosini)/\1\2:\3/' | tee "$writeto" >/dev/null
  else
    sed -r ':SUBSTITUTE_COLON_IN_PARAM s/^([^=]+)[:]([^=]+)+(\s?=.*)/\1-\2\3/g ; t SUBSTITUTE_COLON_IN_PARAM' "$readfrom" |\
      sed -r ':SUBSTITUTE_COLON_IN_VALUE_1 s/(.*\s?=\s?)(.*)<([^ ]*):([^ ]*)>/\1\2<\3-\4>/g ; t SUBSTITUTE_COLON_IN_VALUE_1' |\
      sed -r ':SUBSTITUTE_COLON_IN_VALUE_2 s/(.*\s?=\s?)(.*)\$\{([^ ]*):([^ ]*)\}/\1\2\$\{\3-\4\}/g ; t SUBSTITUTE_COLON_IN_VALUE_2' |\
      sed -r 's/^(;\s?|#\s?)(vim)[-](ft=dosini)/\1\2:\3/' | tee "$writeto" >/dev/null
  fi

  ret=$?

  if [[ $ret -gt 1 ]]; then
    echo -e "\033[31mFailed to patch config file... #$ret\033[0m"; exit $ret
  else
    echo -e "\033[32mConfig file was Successfully patched (Saved at: ${writeto})\033[0m"; exit 0
  fi
}

main "$@"
