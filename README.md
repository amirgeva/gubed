# gubed - Instrumenting debugger for wren

![Screen shot](https://github.com/amirgeva/gubed/shot.png)

This is a rudimentary debugger application for wren scripts.
Instead of using modifications to the VM, or using VM hooks/traps for debugging, it sacrifices runtime performance in exchange for implementation simplicity.

It uses instrumentation, which means that the wren script is modified when loaded, and lines that call the debugger are added.  These lines allow the debugger to step over the code, and also pass the value of local variables, for examination.

The current version is a preliminary alpha version tested on very few code examples, so it will probably require more work and getting script code examples that demonstrate failure points in order to improve it.  The instrumented code is currently only lines within methods. 
