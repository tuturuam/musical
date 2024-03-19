# Musical

<video src='https://github.com/tuturuam/musical/assets/31025785/e634790a-6299-4e7e-b43d-7b3ae04366c5' width=180/>
</video>





An early prototype music visualizer with transparency made with raylib in C.

Enable a low-pass filter by pressing "f".

Enable a slow-down by pressing "k".

[output3.webm](https://github.com/tuturuam/musical/assets/31025785/f3e2cd97-fd93-40c5-86ae-e223123a3c10)


## Building 

Make sure you have raylib installed. And that you edit main.c to a screen size that actually fits your screen.

### Linux
Run 
```bash
./build.sh
```

### Windows
Follow [Working on Windows](https://github.com/raysan5/raylib/wiki/Working-on-Windows#manual-setup-with-w64devkit) until step 4.

Run
```bash
gcc -Wall -Wextra -o musical.exe main.c -lraylib -lopengl32-lgdi32 -lwinmm -DPLATFORM_DESKTOP
./musical.exe
```

