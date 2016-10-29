" Swap between source/header
nnoremap <silent> <leader>af :call ft#cpphpp#Swap('edit')<cr>
nnoremap <silent> <leader>as :call ft#cpphpp#Swap('new')<cr>
nnoremap <silent> <leader>av :call ft#cpphpp#Swap('vnew')<cr>

" Code formatting using clang-format
set formatprg=/usr/bin/clang-format
nmap <f1> :ClangFormat<cr>
