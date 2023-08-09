We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format our code.

# Integration into editors

Integration into various editors (`vim`, `emacs`, `VSCode`, `CLion`) is on the
[clang-format project page](https://clang.llvm.org/docs/ClangFormat.html).

There is also an [eclipse plugin](https://github.com/wangzw/CppStyle).

# Integration into git

## Short version

See below for a more detailed explanation. In short: Add this to `~/.gitconfig`:
```
[clangFormat]
	binary = clang-format-12
	style = file
```
Now stage files for reformatting, then run `git clang-format`. It will reformat
only the code that is staged, and you can view reformatted code with `git
diff`. To add modified lines to your commit, stage them as well, then commit.

There is also a pre-commit hook that you can install.  To install it, copy it
`pre-commit-clang` to `.git/hooks/pre-commit`, and make it executable.

## Long version

In order to integrate `clang-format` into `git`, follow these steps:

1) Copy `pre-commit-clang` to `.git/hooks/pre-commit` and make it executable.

2) Install clang-format-12 (needs at least Ubuntu 20, remove anything else)
   ```bash
   $ sudo apt-get remove clang-format*
   $ sudo apt-get install clang-format-12
   ```

3) Configure `git` (set correct executable, set mode, provide more convenient alias)
   ```bash
   $ git config --global clangFormat.binary clang-format-12
   $ git config --global clangFormat.style file
   $ git config --global alias.clang-format clang-format-12
   ```

When this is done, you are set up. How to use:

4) When committing new code, add the code to commit into the staging (`git add
   -p` or `git add <file>`) _and stash the rest_ (e.g., `git stash -p`, this is
   quite important or reformatting might not work properly, or you don't know
   what has been reformatted)

5) Run git clang-format. The staged code will be reformatted. After this, you
   still have your changes in the staging area, and the formatted code appears
   as additional modification. Thus, your changes can be seen with
   ```bash
   $ git diff --staged
   ```
   while the modifications of clang-format with
   ```bash
   $ git diff
   ```
   So now, you can add all or parts of reformatted code to the staging area

6) Commit. It won't work if code is not properly formatted due to the
   pre-commit hook. Force committing with `git commit --no-verify`
