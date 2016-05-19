#!/usr/bin/env bash
#
# Creates a summary of active torrents
#

main() {
  local rtorrent_session_dir=${1:-"${HOME}/.cache/rtorrent"} ; shift
  local max_count=${1:-3} ; shift
  local cap=${1:-40} ; shift

  local file target_dir chunks_wanted chunks_done chunks_total i

  for file in $(find "$rtorrent_session_dir" -name '*.rtorrent' | sed -nr 's/^(.*)\.rtorrent$/\0/p'); do
    target_dir=$(sed -nr 's/.*directory[0-9]+:(.*)7:hashing.*/\1/p' "$file")

    state=$(egrep -ro "statei([0-9]+)e13" "$file")
    state=${state##*i}
    state=${state%%e*}

    chunks_done=$(egrep -ro "chunks_donei([0-9]+)e13" "$file")
    chunks_wanted=$(egrep -ro "chunks_wantedi([0-9]+)e8" "$file")
    chunks_done=${chunks_done##*i}
    chunks_done=${chunks_done%%e*}
    chunks_wanted=${chunks_wanted##*i}
    chunks_wanted=${chunks_wanted%%e*}
    chunks_total=$(( chunks_done + chunks_wanted ))

    if (( $(sed -nr 's/.*statei([0-9]+)e13.*/\1/p' "$file") )); then
      [[ "$chunks_total" == "$chunks_wanted" ]] && [[ $chunks_done -eq 0 ]] && continue;

      num_files=$(( num_files + 1 ))
      label=$(echo "$target_dir" | sed -nr 's/\//\n/gp' | tail -1)

      if [[ ${#label} -gt $cap ]]; then
        label=${label:0:$cap}
        label="${label% *}..."
      fi

      echo "${label}:${chunks_total:-0}:${chunks_done:-0}:${chunks_wanted:-0}"

      i=$(( i + 1 ))

      [[ $i -ge $max_count ]] && break
    fi
  done
}

main "$@"
