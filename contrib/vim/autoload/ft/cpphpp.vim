"
" Get the filename of the swap file
"
func! ft#cpphpp#GetFilename()
  let ext = expand('%:e')
  let root = expand('%:p:r')
  if (ext == 'cpp')
    return fnameescape(substitute(root, '\(src/.*/\)\?src/', '\1include/', '') . '.hpp')
  elseif (ext == 'hpp')
    return fnameescape(substitute(root, '\(include/.*/\)\?include/', '\1src/', '') . '.cpp')
  endif
endfunc

"
" Swap between source/header using given cmd
"
func! ft#cpphpp#Swap(cmd)
  execute a:cmd . ' ' . ft#cpphpp#GetFilename()
endfunc
