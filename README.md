# atrac3_decoder
A reference decoder for the Minidisc ATRAC3 codec

Currently, it only supports LP2.

The usage is as follows:
- Connect a NetMD Minidisc player to Web Minidisc Pro
- Download an ATRAC3 LP2 file from the player and save it as a `.wav` file
- Use this decoder to render the ATRAC3 `.wav` file to a standard PCM `.wav` file.

To build, simply run `make`, and this will generate two binaries: `decoder`
and `test` (for unit tests). Running `decoder` will display the help options.

This has been built and run on MacOS with `clang++`, but no other compilers or systems so far.
It uses C++11 for very wide compatibility.
