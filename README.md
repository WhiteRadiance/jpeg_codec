# jpeg_codec
JPEG图片的编解码
## CAUTION
This codec just implements the conversion between a JPEG file and a raw YUV file.

You'd better have some software to help you display a picture in YUV format.
## jpeg.h
Defines jpeg structure, quantization table, Huffman table, DCT matrix, NR_code, NR_values and some buffers.
## jpeg_std.cpp
Defines functions to encode a raw YUV picture into a JPEG.
## jpeg_dec.cpp
Defines functions to decode a JPEG into a raw YUV picture.
## main.cpp
Just an usage example.
