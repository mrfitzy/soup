Requires zig 0.16.0.

Launch the emulator:
```
zig build run
```

Launch integrated debugger:
```
zig build debug
```

Launch with [tracy](https://github.com/wolfpld/tracy/) profiling:
```
zig build -Doptimize=ReleaseFast -Dprofile run
```
