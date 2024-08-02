# mcufont-converter-web
TTF font to C code converter for the embedded [`mcufont`](https://github.com/mcufont) font library. The project is based on mcufont encoder https://github.com/mcufont/mcufont/blob/master/encoder/main.cc and WebAssembly.

## Clone the project
```
git clone --recurse-submodules https://github.com/HexRx/mcufont-converter-web.git
```

## How to build a WebAssembly part
Use Emscripten toolchain with docker
```
docker run --rm -v $(pwd):/src -u $(id -u):$(id -g) -w /src/build emscripten/emsdk emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
docker run --rm -v $(pwd):/src -u $(id -u):$(id -g) -w /src/build emscripten/emsdk emmake make
```

## Description of Emscripten parameters
`STACK_SIZE=131072` - requires for recursion function `fill_tree_suffixes()`.
`ALLOW_MEMORY_GROWTH=1` - handling fonts with a large number of glyphs.

## Preact frontend part
-   `npm run dev` - Starts a dev server at http://localhost:5173/
-   `npm run build` - Builds for production, emitting to `dist/`
-   `npm run preview` - Starts a server at http://localhost:4173/ to test production build locally

## License
This project is licensed under the MIT License - see the [LICENSE](/LICENSE) file for details.
