set nocompatible

syntax enable

set tabstop=8           " The width of a TAB
set shiftwidth=8        " Indent size
set softtabstop=8       " Sets the number of columns for a TAB
set expandtab           " Expand TABs to spaces
set smarttab            " Make "tab" insert indents instead of tabs at the beginning of a line
set exrc
set secure

set number              " Show line numbers
set showcmd             " Show command in bottom bar
set nocursorline        " Highlight current line
set wildmenu
"set lazyredraw
set showmatch           " Higlight matching parenthesis

set incsearch           " search as characters are entered
set hlsearch            " highlight matches

" Replace tabs with spaces
map <F2> :retab <CR> :w <CR>

" Toggle whitespace visibility
nmap <F3> :set list!<CR>
set listchars=tab:>-,trail:-

" Toggle between header and implementation
map <F4> :e %:p:s,.h$,.X123X,:s,.cpp$,.h,:s,.X123X$,.cpp,<CR>

" Line extend: http://www.alexeyshmalko.com/2014/using-vim-as-c-cpp-ide/
set colorcolumn=120
highlight ColorColumn ctermbg=darkgray

" Text search
highlight Search cterm=NONE ctermfg=NONE ctermbg=darkgray

set path+=src
set path+=src/core
set path+=src/tasks
set path+=src/losses
set path+=src/tensor
set path+=src/solvers
set path+=src/learners
set path+=src/functions
set path+=apps
set path+=tests
set path+=deps/eigen3
set path+=deps/json/single_include

" Trim trailing whitespaces when saving
function! StripTrailingWhitespaces()
        let l = line(".")
        let c = col(".")
        %s/\s\+$//e
        call cursor(l, c)
endfunction
autocmd BufWritePre     * :call StripTrailingWhitespaces()
