@echo off
rmdir /q /s single_dir_for_windows_kernel                                                                              1>nul 2>nul
mkdir single_dir_for_windows_kernel                                                                                    1>nul 2>nul

copy /y ..\..\src\lfds700_btree_addonly_unbalanced\*                     single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_freelist\*                                     single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_hash_addonly\*                                 single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_list_addonly_ordered_singlylinked\*            single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_list_addonly_singlylinked_unordered\*          single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_misc\*                                         single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_queue\*                                        single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_queue_bounded_singleconsumer_singleproducer\*  single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_ringbuffer\*                                   single_dir_for_windows_kernel\                1>nul 2>nul
copy /y ..\..\src\lfds700_stack\*                                        single_dir_for_windows_kernel\                1>nul 2>nul

copy /y ..\..\src\liblfds700_internal.h                                  single_dir_for_windows_kernel\                1>nul 2>nul
copy /y sources.static                                                   single_dir_for_windows_kernel\sources         1>nul 2>nul

echo Windows kernel static library build directory structure created.
echo (Note the effects of this batch file are idempotent).

