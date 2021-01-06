/*
 *
$ ./YOUR_COMPILER return_2.c # compile the source file shown above
$ ./gcc -m32 return_2.s -o return_2 # assemble it into an executable
$ ./return_2 # run the executable you just compiled
$ echo $? # check the return code; it should be 2
2
*/

/*
 * $ gcc -S -O3 -fno-asynchronous-unwind-tables return_2.c
$ cat return_2.s
    .section __TEXT,__text_startup,regular,pure_instructions
    .align 4
    .globl _main
_main:
    movl    $2, %eax
    ret
    .subsections_via_symbols
*/
int main()
{
    return 2;
}
