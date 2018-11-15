# buddhabrot-amp
buddhabrot-amp is a [buddhabrot](https://en.wikipedia.org/wiki/Buddhabrot) generator written in C++. It utilizes [C++ AMP](https://en.wikipedia.org/wiki/C%2B%2B_AMP) for the buddhabrot calculation, [Direct3D11](https://docs.microsoft.com/en-us/windows/desktop/direct3d11/atoc-dx-graphics-direct3d-11)/[DXGI](https://docs.microsoft.com/en-us/windows/desktop/api/_direct3ddxgi/) for presentation to the screen & [WIC](https://docs.microsoft.com/en-us/windows/desktop/wic/-wic-about-windows-imaging-codec) for writing PNGs.

## How to run
- Clone this repo
- Open solution using Visual Studio (build & tested with Visual Studio 2017)
- Build & run via Visual Studio

## Main components
### `BuddhabrotGenerator`
This class represents the core logic of generating the buddhabrot. It uses C++ AMP to find complex numbers which escape the [Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set) & mark their path on a "canvas" up until they're considered to have escaped. The canvas that is used to record the paths of these escaping points make up the buddhabrot. We color a point on this canvas brighter/darker based on how many paths hit/did not hit this particular cell/point.

### `BuddhabrotPresenter`
This class simply takes three canvases of equal dimensions for each color (red, green & blue) , puts the three color channels into one texture & finally samples this texture into a DXGI swapchain to be displayed on the screen.

### `write_png_from_arrays`
This function is similar in it's logic to the `BuddhabrotPresenter` in that it takes 3 canvases, combines them into one image & writes that image out to disk as a PNG file.

## External dependencies used
- [C++ AMP](https://en.wikipedia.org/wiki/C%2B%2B_AMP)
- [Direct3D11](https://docs.microsoft.com/en-us/windows/desktop/direct3d11/atoc-dx-graphics-direct3d-11)
- [DXGI](https://docs.microsoft.com/en-us/windows/desktop/api/_direct3ddxgi/)
- [WIC](https://docs.microsoft.com/en-us/windows/desktop/wic/-wic-about-windows-imaging-codec)
- [TinyMT](http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/TINYMT/index.html)
- [args](https://github.com/Taywee/args)

## Sample image produced
![sample](docs/images/image-amp.png)