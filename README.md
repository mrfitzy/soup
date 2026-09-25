Requires zig 0.16.0.

Launch the emulator:
```
zig build run
```

Launch integrated debugger:
```
zig build debug
```

<img width="1315" height="763" alt="screenshot of integrated debugger" src="https://github.com/user-attachments/assets/1ee859a9-9f96-4919-8d52-5f9bcaf4dd8b" />


Launch emulator with [tracy](https://github.com/wolfpld/tracy/) profiling:
```
zig build -Doptimize=ReleaseFast -Dprofile run
```
