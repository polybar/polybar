#!/usr/bin/env bash
read -p "Send SIGKILL to terminate processes? [Y/n] " -r choice

[[ "${choice^^}" == "Y" ]] || {
  echo "Aborting..."
  exit
}

[[ "${choice^^}" == "Y" ]] && {
  pgrep -f "(lemonbuddy_wrapper.sh|^lemonb(uddy|ar))" | xargs kill -9 >/dev/null 2>&1

  if [[ $? -eq 0 ]]; then
    echo "Kill signals successfully sent"
  else
    echo "Failed to send kill signal ($?)"
  fi
}
