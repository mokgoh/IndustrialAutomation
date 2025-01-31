// ==========================================================
// BMP Loader and Writer
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Markus Loibl (markus.loibl@epost.de)
// - Martin Weber (martweb@gmx.net)
// - Herv� Drolon (drolon@infonie.fr)
// - Michal Novotny (michal@etc.cz)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

#include "FreeImage.h"
#include "Utilities.h"

// ----------------------------------------------------------
//   Constants + headers
// ----------------------------------------------------------

static const BYTE RLE_COMMAND     = 0;
static const BYTE RLE_ENDOFLINE   = 0;
static const BYTE RLE_ENDOFBITMAP = 1;
static const BYTE RLE_DELTA       = 2;

#ifndef __MINGW32__		// prevents a bug in mingw32

static const BYTE BI_RGB          = 0;
static const BYTE BI_RLE8         = 1;
static const BYTE BI_RLE4         = 2;
static const BYTE BI_BITFIELDS    = 3;

#endif // __MINGW32__

// ----------------------------------------------------------

#ifdef WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

#ifndef __MINGW32__
typedef struct tagBITMAPCOREHEADER {
  DWORD   bcSize;
  WORD    bcWidth;
  WORD    bcHeight;
  WORD    bcPlanes;
  WORD    bcBitCnt;
} BITMAPCOREHEADER, *PBITMAPCOREHEADER; 
#endif //__MINGW32__

typedef struct tagBITMAPINFOOS2_1X_HEADER {
  DWORD  biSize;
  WORD   biWidth;
  WORD   biHeight; 
  WORD   biPlanes; 
  WORD   biBitCount;
} BITMAPINFOOS2_1X_HEADER, *PBITMAPINFOOS2_1X_HEADER; 

#ifndef __MINGW32__
typedef struct tagBITMAPFILEHEADER {
  WORD    bfType; 
  DWORD   bfSize;
  WORD    bfReserved1; 
  WORD    bfReserved2;
  DWORD   bfOffBits; 
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;
#endif //__MINGW32__

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Internal functions
// ==========================================================

#ifdef FREEIMAGE_BIGENDIAN
static void
SwapInfoHeader(BITMAPINFOHEADER *header) {
	SwapLong(&header->biSize);
	SwapLong((DWORD *)&header->biWidth);
	SwapLong((DWORD *)&header->biHeight);
	SwapShort(&header->biPlanes);
	SwapShort(&header->biBitCount);
	SwapLong(&header->biCompression);
	SwapLong(&header->biSizeImage);
	SwapLong((DWORD *)&header->biXPelsPerMeter);
	SwapLong((DWORD *)&header->biYPelsPerMeter);
	SwapLong(&header->biClrUsed);
	SwapLong(&header->biClrImportant);
}

static void
SwapCoreHeader(BITMAPCOREHEADER *header) {
	SwapLong(&header->bcSize);
	SwapShort(&header->bcWidth);
	SwapShort(&header->bcHeight);
	SwapShort(&header->bcPlanes);
	SwapShort(&header->bcBitCnt);
}

static void
SwapOS21XHeader(BITMAPINFOOS2_1X_HEADER *header) {
	SwapLong(&header->biSize);
	SwapShort(&header->biWidth);
	SwapShort(&header->biHeight);
	SwapShort(&header->biPlanes);
	SwapShort(&header->biBitCount);
}

static void
SwapFileHeader(BITMAPFILEHEADER *header) {
	SwapShort(&header->bfType);
  	SwapLong(&header->bfSize);
  	SwapShort(&header->bfReserved1);
  	SwapShort(&header->bfReserved2);
	SwapLong(&header->bfOffBits);
}
#endif

static FIBITMAP *
LoadWindowsBMP(FreeImageIO *io, fi_handle handle, int flags, unsigned bitmap_bits_offset) {
	FIBITMAP *dib = NULL;

	try {
		// load the info header

		BITMAPINFOHEADER bih;

		io->read_proc(&bih, sizeof(BITMAPINFOHEADER), 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
		SwapInfoHeader(&bih);
#endif

		// keep some general information about the bitmap

		int used_colors = bih.biClrUsed;
		int width       = bih.biWidth;
		int height      = bih.biHeight;
		int bit_count   = bih.biBitCount;
		int compression = bih.biCompression;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));

		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				if ((used_colors <= 0) || (used_colors > CalculateUsedPaletteEntries(bit_count)))
					used_colors = CalculateUsedPaletteEntries(bit_count);
				
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				dib = FreeImage_Allocate(width, height, bit_count);

				if (dib == NULL)
					throw "DIB allocation failed";

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;
				
				// load the palette

				io->read_proc(FreeImage_GetPalette(dib), used_colors * sizeof(RGBQUAD), 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				RGBQUAD *pal = FreeImage_GetPalette(dib);
				for(int i = 0; i < used_colors; i++) {
					INPLACESWAP(pal[i].rgbRed, pal[i].rgbBlue);
				}
#endif

				// seek to the actual pixel data.
				// this is needed because sometimes the palette is larger than the entries it contains predicts

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * sizeof(RGBQUAD))))
					io->seek_proc(handle, bitmap_bits_offset, SEEK_SET);
				
				// read the pixel data

				switch (compression) {
					case BI_RGB :
						if (height > 0) {
							io->read_proc((void *)FreeImage_GetBits(dib), height * pitch, 1, handle);
						} else {
							int tmp = abs(height);

							for (int c = 0; c < tmp; ++c) {
								io->read_proc((void *)FreeImage_GetScanLine(dib, tmp - c - 1), pitch, 1, handle);
							}
						}
						
						return dib;

					case BI_RLE4 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;
						int align = 0;
						BOOL low_nibble = FALSE;

						for (;;) {
							io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											low_nibble = FALSE;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x = 0;
											BYTE delta_y = 0;

											io->read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io->read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits     += delta_x / 2;
											scanline += delta_y;
											break;
										}

										default :
										{
											io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

											BYTE *sline = FreeImage_GetScanLine(dib, scanline);

											for (int i = 0; i < status_byte; i++) {
												if (low_nibble) {
													*(sline + bits) |= LOWNIBBLE(second_byte);

													if (i != status_byte - 1)
														io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

													bits++;
												} else {
													*(sline + bits) |= HINIBBLE(second_byte);
												}
												
												low_nibble = !low_nibble;
											}

											// align run length to even number of bytes 

											align = status_byte % 4;
											if((align == 1) || (align == 2)) {
												io->read_proc(&second_byte, sizeof(BYTE), 1, handle);
											}

											break;
										}
									}

									break;

								default :
								{
									BYTE *sline = FreeImage_GetScanLine(dib, scanline);

									io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										if (low_nibble) {
											*(sline + bits) |= LOWNIBBLE(second_byte);

											bits++;
										} else {
											*(sline + bits) |= HINIBBLE(second_byte);
										}				
										
										low_nibble = !low_nibble;
									}
								}

								break;
							}
						}

						return (FIBITMAP *)dib;
					}

					case BI_RLE8 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;

						for (;;) {
							io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x = 0;
											BYTE delta_y = 0;

											io->read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io->read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits     += delta_x;
											scanline += delta_y;

											break;
										}

										default :
											io->read_proc((void *)(FreeImage_GetScanLine(dib, scanline) + bits), sizeof(BYTE) * status_byte, 1, handle);
											
											// align run length to even number of bytes 

											if ((status_byte & 1) == 1)
												io->read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											bits += status_byte;													

											break;								
									};

									break;

								default :
									BYTE *sline = FreeImage_GetScanLine(dib, scanline);

									io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										*(sline + bits) = second_byte;

										bits++;					
									}

									break;
							};
						}

						break;
					}

					default :
						throw "compression type not supported";
				}

				break;
			}

			case 16 :
			{
				if (bih.biCompression == BI_BITFIELDS) {
					DWORD bitfields[3];

					io->read_proc(bitfields, 3 * sizeof(DWORD), 1, handle);

					dib = FreeImage_Allocate(width, height, bit_count, bitfields[0], bitfields[1], bitfields[2]);
				} else {
					dib = FreeImage_Allocate(width, height, bit_count, FI16_555_RED_MASK, FI16_555_GREEN_MASK, FI16_555_BLUE_MASK);
				}

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				io->read_proc(FreeImage_GetBits(dib), height * pitch, 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
					WORD *pixel = (WORD *)FreeImage_GetScanLine(dib, y);
					for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
						SwapShort(pixel);
						pixel++;
					}
				}
#endif

				return dib;
			}

			case 24 :
			case 32 :
			{
				if (bih.biCompression == BI_BITFIELDS) {
					DWORD bitfields[3];

					io->read_proc(bitfields, 3 * sizeof(DWORD), 1, handle);

					dib = FreeImage_Allocate(width, height, bit_count, bitfields[0], bitfields[1], bitfields[2]);
				} else {
					if( bit_count == 32 ) {
						dib = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
					} else {
						dib = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
					}
				}

				if (dib == NULL)
					throw "DIB allocation failed";

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				if (pInfoHeader->biClrUsed > 0) {
				    io->seek_proc(handle, pInfoHeader->biClrUsed * sizeof(RGBQUAD), SEEK_CUR);
				} else if ((bih.biCompression != BI_BITFIELDS) && (bitmap_bits_offset > sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))) {
					io->seek_proc(handle, bitmap_bits_offset, SEEK_CUR);
				}

				// read in the bitmap bits

				io->read_proc(FreeImage_GetBits(dib), height * pitch, 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
					BYTE *pixel = FreeImage_GetScanLine(dib, y);
					for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
						INPLACESWAP(pixel[0], pixel[2]);
						pixel += (bit_count>>3);
					}
				}
#endif

				// check if the bitmap contains transparency, if so enable it in the header

				FreeImage_SetTransparent(dib, (FreeImage_GetColorType(dib) == FIC_RGBALPHA));

				return dib;
			}
		}
	} catch(const char *message) {
		if(dib)
			FreeImage_Unload(dib);

		FreeImage_OutputMessageProc(s_format_id, message);
	}

	return NULL;
}

static FIBITMAP *
LoadOS22XBMP(FreeImageIO *io, fi_handle handle, int flags, unsigned bitmap_bits_offset) {
	FIBITMAP *dib = NULL;

	try {
		// load the info header

		BITMAPINFOHEADER bih;

		io->read_proc(&bih, sizeof(BITMAPINFOHEADER), 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
		SwapInfoHeader(&bih);
#endif

		// keep some general information about the bitmap

		int used_colors = bih.biClrUsed;
		int width       = bih.biWidth;
		int height      = bih.biHeight;
		int bit_count   = bih.biBitCount;
		int compression = bih.biCompression;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));
		
		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				if ((used_colors <= 0) || (used_colors > CalculateUsedPaletteEntries(bit_count)))
					used_colors = CalculateUsedPaletteEntries(bit_count);
					
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				dib = FreeImage_Allocate(width, height, bit_count);

				if (dib == NULL)
					throw "DIB allocation failed";

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;
				
				// load the palette

				io->seek_proc(handle, sizeof(BITMAPFILEHEADER) + bih.biSize, SEEK_SET);

				RGBQUAD *pal = FreeImage_GetPalette(dib);

				for (int count = 0; count < used_colors; count++) {
					FILE_BGR bgr;

					io->read_proc(&bgr, sizeof(FILE_BGR), 1, handle);
					
					pal[count].rgbRed = bgr.r;
					pal[count].rgbGreen = bgr.g;
					pal[count].rgbBlue = bgr.b;
				}

				// seek to the actual pixel data.
				// this is needed because sometimes the palette is larger than the entries it contains predicts

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * 3)))
					io->seek_proc(handle, bitmap_bits_offset, SEEK_SET);

				// read the pixel data

				switch (compression) {
					case BI_RGB :
						if (height > 0) {
							io->read_proc((void *)FreeImage_GetBits(dib), height * pitch, 1, handle);
						} else {
							for (int c = 0; c < abs(height); ++c) {
								io->read_proc((void *)FreeImage_GetScanLine(dib, height - c - 1), pitch, 1, handle);								
							}
						}
						
						return dib;

					case BI_RLE4 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;
						BOOL low_nibble = FALSE;

						for (;;) {
							io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											low_nibble = FALSE;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x;
											BYTE delta_y;

											io->read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io->read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits       += delta_x / 2;
											scanline   += delta_y;
											break;
										}

										default :
											io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

											BYTE *sline = FreeImage_GetScanLine(dib, scanline);

											for (int i = 0; i < status_byte; i++) {
												if (low_nibble) {
													*(sline + bits) |= LOWNIBBLE(second_byte);

													if (i != status_byte - 1)
														io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

													bits++;
												} else {
													*(sline + bits) |= HINIBBLE(second_byte);
												}
												
												low_nibble = !low_nibble;
											}

											if (((status_byte / 2) & 1 ) == 1)
												io->read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											break;
									};

									break;

								default :
								{
									BYTE *sline = FreeImage_GetScanLine(dib, scanline);

									io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										if (low_nibble) {
											*(sline + bits) |= LOWNIBBLE(second_byte);

											bits++;
										} else {
											*(sline + bits) |= HINIBBLE(second_byte);
										}				
										
										low_nibble = !low_nibble;
									}
								}

								break;
							};
						}

						break;
					}

					case BI_RLE8 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;

						for (;;) {
							io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

							switch (status_byte) {
								case RLE_COMMAND :
									io->read_proc(&status_byte, sizeof(BYTE), 1, handle);

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)dib;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x;
											BYTE delta_y;

											io->read_proc(&delta_x, sizeof(BYTE), 1, handle);
											io->read_proc(&delta_y, sizeof(BYTE), 1, handle);

											// apply them

											bits     += delta_x;
											scanline += delta_y;
											break;
										}

										default :
											io->read_proc((void *)(FreeImage_GetScanLine(dib, scanline) + bits), sizeof(BYTE) * status_byte, 1, handle);
											
											// align run length to even number of bytes 

											if ((status_byte & 1) == 1)
												io->read_proc(&second_byte, sizeof(BYTE), 1, handle);												

											bits += status_byte;

											break;								
									};

									break;

								default :
									BYTE *sline = FreeImage_GetScanLine(dib, scanline);

									io->read_proc(&second_byte, sizeof(BYTE), 1, handle);

									for (unsigned i = 0; i < status_byte; i++) {
										*(sline + bits) = second_byte;

										bits++;
									}

									break;
							};
						}

						break;
					}

					default :		
						throw "compression type not supported";
				}						

				break;
			}

			case 16 :
			{
				if (bih.biCompression == 3) {
					DWORD bitfields[3];

					io->read_proc(bitfields, 3 * sizeof(DWORD), 1, handle);

					dib = FreeImage_Allocate(width, height, bit_count, bitfields[0], bitfields[1], bitfields[2]);
				} else {
					dib = FreeImage_Allocate(width, height, bit_count, FI16_555_RED_MASK, FI16_555_GREEN_MASK, FI16_555_BLUE_MASK);
				}

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * 3)))
					io->seek_proc(handle, bitmap_bits_offset, SEEK_SET);

				io->read_proc(FreeImage_GetBits(dib), height * pitch, 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
					WORD *pixel = (WORD *)FreeImage_GetScanLine(dib, y);
					for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
						SwapShort(pixel);
						pixel++;
					}
				}
#endif
				return dib;
			}

			case 24 :
			case 32 :
			{
				if( bit_count == 32 ) {
					dib = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
				} else {
					dib = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
				}

				if (dib == NULL)
					throw "DIB allocation failed";
				
				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				if (bitmap_bits_offset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * 3)))
					io->seek_proc(handle, bitmap_bits_offset, SEEK_SET);
				
				// read in the bitmap bits

				io->read_proc(FreeImage_GetBits(dib), height * pitch, 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
					BYTE *pixel = FreeImage_GetScanLine(dib, y);
					for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
						INPLACESWAP(pixel[0], pixel[2]);
						pixel += (bit_count>>3);
					}
				}
#endif

				// check if the bitmap contains transparency, if so enable it in the header

				FreeImage_SetTransparent(dib, (FreeImage_GetColorType(dib) == FIC_RGBALPHA));

				return dib;
			}
		}
	} catch(const char *message) {
		if(dib)
			FreeImage_Unload(dib);

		FreeImage_OutputMessageProc(s_format_id, message);
	}

	return NULL;
}

static FIBITMAP *
LoadOS21XBMP(FreeImageIO *io, fi_handle handle, int flags, unsigned bitmap_bits_offset) {
	FIBITMAP *dib = NULL;

	try {
		BITMAPINFOOS2_1X_HEADER bios2_1x;

		io->read_proc(&bios2_1x, sizeof(BITMAPINFOOS2_1X_HEADER), 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
		SwapOS21XHeader(&bios2_1x);
#endif
		// keep some general information about the bitmap

		int used_colors = 0;
		int width       = bios2_1x.biWidth;
		int height      = bios2_1x.biHeight;
		int bit_count   = bios2_1x.biBitCount;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));
		
		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				used_colors = CalculateUsedPaletteEntries(bit_count);
				
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				dib = FreeImage_Allocate(width, height, bit_count);

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = 0;
				pInfoHeader->biYPelsPerMeter = 0;
				
				// load the palette

				RGBQUAD *pal = FreeImage_GetPalette(dib);

				for (int count = 0; count < used_colors; count++) {
					FILE_BGR bgr;

					io->read_proc(&bgr, sizeof(FILE_BGR), 1, handle);
					
					pal[count].rgbRed = bgr.r;
					pal[count].rgbGreen = bgr.g;
					pal[count].rgbBlue = bgr.b;
				}

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				io->seek_proc(handle, bitmap_bits_offset, SEEK_SET);
				
				// read the pixel data

				if (height > 0) {
					io->read_proc((void *)FreeImage_GetBits(dib), height * pitch, 1, handle);
				} else {
					for (int c = 0; c < abs(height); ++c) {
						io->read_proc((void *)FreeImage_GetScanLine(dib, height - c - 1), pitch, 1, handle);								
					}
				}
						
				return dib;
			}

			case 16 :
			{
				dib = FreeImage_Allocate(width, height, bit_count, FI16_555_RED_MASK, FI16_555_GREEN_MASK, FI16_555_BLUE_MASK);

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = 0;
				pInfoHeader->biYPelsPerMeter = 0;

				io->read_proc(FreeImage_GetBits(dib), height * pitch, 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
					WORD *pixel = (WORD *)FreeImage_GetScanLine(dib, y);
					for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
						SwapShort(pixel);
						pixel++;
					}
				}
#endif

				return dib;
			}

			case 24 :
			case 32 :
			{
				if( bit_count == 32 ) {
					dib = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
				} else {
					dib = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
				}

				if (dib == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(dib);
				pInfoHeader->biXPelsPerMeter = 0;
				pInfoHeader->biYPelsPerMeter = 0;

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				io->read_proc(FreeImage_GetBits(dib), height * pitch, 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
					BYTE *pixel = FreeImage_GetScanLine(dib, y);
					for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
						INPLACESWAP(pixel[0], pixel[2]);
						pixel += (bit_count>>3);
					}
				}
#endif

				// check if the bitmap contains transparency, if so enable it in the header

				FreeImage_SetTransparent(dib, (FreeImage_GetColorType(dib) == FIC_RGBALPHA));

				return dib;
			}
		}
	} catch(const char *message) {	
		if(dib)
			FreeImage_Unload(dib);

		FreeImage_OutputMessageProc(s_format_id, message);
	}

	return NULL;
}

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "BMP";
}

static const char * DLL_CALLCONV
Description() {
	return "Windows or OS/2 Bitmap";
}

static const char * DLL_CALLCONV
Extension() {
	return "bmp";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^BM";
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/bmp";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {
	BYTE bmp_signature1[] = { 0x42, 0x4D };
	BYTE bmp_signature2[] = { 0x42, 0x41 };
	BYTE signature[2] = { 0, 0 };

	io->read_proc(signature, 1, sizeof(bmp_signature1), handle);

	if (memcmp(bmp_signature1, signature, sizeof(bmp_signature1)) == 0)
		return TRUE;

	if (memcmp(bmp_signature2, signature, sizeof(bmp_signature2)) == 0)
		return TRUE;

	return FALSE;
}

static BOOL DLL_CALLCONV
SupportsExportDepth(int depth) {
	return (
			(depth == 1) ||
			(depth == 4) ||
			(depth == 8) ||
			(depth == 16) ||
			(depth == 24) ||
			(depth == 32)
		);
}

static BOOL DLL_CALLCONV 
SupportsExportType(FREE_IMAGE_TYPE type) {
	return (type == FIT_BITMAP) ? TRUE : FALSE;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImageIO *io, fi_handle handle, int page, int flags, void *data) {
	if (handle != NULL) {
		BITMAPFILEHEADER bitmapfileheader;
		DWORD type = 0;
		BYTE magic[2];

		// we use this offset value to make seemingly absolute seeks relative in the file
		
		long offset_in_file = io->tell_proc(handle);

		// read the magic

		io->read_proc(&magic, sizeof(magic), 1, handle);

		// compare the magic with the number we know

		// somebody put a comment here explaining the purpose of this loop
		while (memcmp(&magic, "BA", 2) == 0) {
			io->read_proc(&bitmapfileheader.bfSize, sizeof(DWORD), 1, handle);
			io->read_proc(&bitmapfileheader.bfReserved1, sizeof(WORD), 1, handle);
			io->read_proc(&bitmapfileheader.bfReserved2, sizeof(WORD), 1, handle);
			io->read_proc(&bitmapfileheader.bfOffBits, sizeof(DWORD), 1, handle);
			io->read_proc(&magic, sizeof(magic), 1, handle);
		}

		// read the fileheader

		io->seek_proc(handle, 0 - sizeof(magic), SEEK_CUR);
		io->read_proc(&bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, handle);
#ifdef FREEIMAGE_BIGENDIAN
		SwapFileHeader(&bitmapfileheader);
#endif

		// read the first byte of the infoheader

		io->read_proc(&type, sizeof(DWORD), 1, handle);
		io->seek_proc(handle, 0 - sizeof(DWORD), SEEK_CUR);
#ifdef FREEIMAGE_BIGENDIAN
		SwapLong(&type);
#endif

		// call the appropriate load function for the found bitmap type

		if (type == 40)
			return LoadWindowsBMP(io, handle, flags, offset_in_file + bitmapfileheader.bfOffBits);
		
		if (type == 12)
			return LoadOS21XBMP(io, handle, flags, offset_in_file + bitmapfileheader.bfOffBits);

		if (type <= 64)
			return LoadOS22XBMP(io, handle, flags, offset_in_file + bitmapfileheader.bfOffBits);

		FreeImage_OutputMessageProc(s_format_id, "unknown bmp subtype with id %d", type);
	}

	return NULL;
}

// ----------------------------------------------------------

static int
RLEEncodeLine(BYTE *target, BYTE *source, int size) {
	BYTE buffer[256];
	int buffer_size = 0;
	int target_pos = 0;

	for (int i = 0; i < size; ++i) {
		if ((i < size - 1) && (source[i] == source[i + 1])) {
			// find a solid block of same bytes

			int j = i + 1;

			while ((j < size - 1) && (source[j] == source[j + 1]))
				++j;

			// if the block is larger than 3 bytes, use it
			// else put the data into the larger pool

			if (((j - i) + 1) > 3) {
				// don't forget to write what we already have in the buffer

				switch(buffer_size) {
					case 0 :
						break;

					case RLE_DELTA :
						target[target_pos++] = 1;
						target[target_pos++] = buffer[0];
						target[target_pos++] = 1;
						target[target_pos++] = buffer[1];
						break;

					case RLE_ENDOFBITMAP :
						target[target_pos++] = buffer_size;
						target[target_pos++] = buffer[0];
						break;

					default :
						target[target_pos++] = RLE_COMMAND;
						target[target_pos++] = buffer_size;
						memcpy(target + target_pos, buffer, buffer_size);

						// prepare for next run
						
						target_pos += buffer_size;

						if ((buffer_size & 1) == 1)
							target_pos++;

						break;
				}

				// write the continuous data

				target[target_pos++] = (j - i) + 1;
				target[target_pos++] = source[i];

				buffer_size = 0;
			} else {
				for (int k = 0; k < (j - i) + 1; ++k) {
					buffer[buffer_size++] = source[i + k];

					if (buffer_size == 254) {
						// write what we have

						target[target_pos++] = RLE_COMMAND;
						target[target_pos++] = buffer_size;
						memcpy(target + target_pos, buffer, buffer_size);

						// prepare for next run

						target_pos += buffer_size;
						buffer_size = 0;
					}
				}
			}

			i = j;
		} else {
			buffer[buffer_size++] = source[i];
		}

		// write the buffer if it's full

		if (buffer_size == 254) {
			target[target_pos++] = RLE_COMMAND;
			target[target_pos++] = buffer_size;
			memcpy(target + target_pos, buffer, buffer_size);

			// prepare for next run

			target_pos += buffer_size;
			buffer_size = 0;
		}
	}

	// write the last bytes

	switch(buffer_size) {
		case 0 :
			break;

		case RLE_DELTA :
			target[target_pos++] = 1;
			target[target_pos++] = buffer[0];
			target[target_pos++] = 1;
			target[target_pos++] = buffer[1];
			break;

		case RLE_ENDOFBITMAP :
			target[target_pos++] = buffer_size;
			target[target_pos++] = buffer[0];
			break;

		default :
			target[target_pos++] = RLE_COMMAND;
			target[target_pos++] = buffer_size;
			memcpy(target + target_pos, buffer, buffer_size);

			// prepare for next run
			
			target_pos += buffer_size;

			if ((buffer_size & 1) == 1)
				target_pos++;

			break;			
	}

	// write the END_OF_LINE marker

	target[target_pos++] = RLE_COMMAND;
	target[target_pos++] = RLE_ENDOFLINE;

	// return the written size

	return target_pos;
}

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int page, int flags, void *data) {
	if ((dib != NULL) && (handle != NULL)) {
		// write the file header

		BITMAPFILEHEADER bitmapfileheader;
		bitmapfileheader.bfType = 0x4D42;
		bitmapfileheader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + FreeImage_GetHeight(dib) * FreeImage_GetPitch(dib);
		bitmapfileheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + FreeImage_GetColorsUsed(dib) * sizeof(RGBQUAD);
		bitmapfileheader.bfReserved1 = 0;
		bitmapfileheader.bfReserved2 = 0;

		// take care of the bit fields data of any

		bool bit_fields = (FreeImage_GetBPP(dib) == 16);

		if (bit_fields) {
			bitmapfileheader.bfSize += 3 * sizeof(DWORD);
			bitmapfileheader.bfOffBits += 3 * sizeof(DWORD);
		}

#ifdef FREEIMAGE_BIGENDIAN
		SwapFileHeader(&bitmapfileheader);
#endif
		if (io->write_proc(&bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, handle) != 1)
			return FALSE;		

		// update the bitmap info header

		BITMAPINFOHEADER bih = *FreeImage_GetInfoHeader(dib);

		if (bit_fields)
			bih.biCompression = BI_BITFIELDS;
		else if ((bih.biBitCount == 8) && (flags & BMP_SAVE_RLE))
			bih.biCompression = BI_RLE8;
		else
			bih.biCompression = BI_RGB;

		// write the bitmap info header

#ifdef FREEIMAGE_BIGENDIAN
		SwapInfoHeader(&bih);
#endif
		if (io->write_proc(&bih, sizeof(BITMAPINFOHEADER), 1, handle) != 1)
			return FALSE;

		// write the bit fields when we are dealing with a 16 bit BMP

		if (bit_fields) {
			DWORD d;

			d = FreeImage_GetRedMask(dib);

			if (io->write_proc(&d, sizeof(DWORD), 1, handle) != 1)
				return FALSE;

			d = FreeImage_GetGreenMask(dib);

			if (io->write_proc(&d, sizeof(DWORD), 1, handle) != 1)
				return FALSE;

			d = FreeImage_GetBlueMask(dib);

			if (io->write_proc(&d, sizeof(DWORD), 1, handle) != 1)
				return FALSE;
		}

		// write the palette

		if (FreeImage_GetPalette(dib) != NULL) {
			RGBQUAD *pal = FreeImage_GetPalette(dib);
			FILE_BGRA bgra;
			for( int i = 0; i < FreeImage_GetColorsUsed(dib); i++ ) {
				bgra.b = pal[i].rgbBlue;
				bgra.g = pal[i].rgbGreen;
				bgra.r = pal[i].rgbRed;
				bgra.a = pal[i].rgbReserved;
				if (io->write_proc(&bgra, sizeof(FILE_BGRA), 1, handle) != 1)
					return FALSE;
			}
		}

		// write the bitmap data... if RLE compression is enable, use it

		unsigned bpp = FreeImage_GetBPP(dib);
		if ((bpp == 8) && (flags & BMP_SAVE_RLE)) {
			BYTE *buffer = new BYTE[FreeImage_GetPitch(dib) * 2];

			for (DWORD i = 0; i < FreeImage_GetHeight(dib); ++i) {
				int size = RLEEncodeLine(buffer, FreeImage_GetScanLine(dib, i), FreeImage_GetLine(dib));

				if (io->write_proc(buffer, size, 1, handle) != 1) {
					delete [] buffer;
					return FALSE;
				}
			}

			buffer[0] = RLE_COMMAND;
			buffer[1] = RLE_ENDOFBITMAP;

			if (io->write_proc(buffer, 2, 1, handle) != 1) {
				delete [] buffer;
				return FALSE;
			}

			delete [] buffer;
#ifdef FREEIMAGE_BIGENDIAN
		} else if (bpp == 16) {
			WORD pixel;
			for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
				BYTE *line = FreeImage_GetScanLine(dib, y);
				for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
					pixel = ((WORD *)line)[x];
					SwapShort(&pixel);
					if (io->write_proc(&pixel, sizeof(WORD), 1, handle) != 1)
						return FALSE;
				}
			}
		} else if (bpp == 24) {
			FILE_BGR bgr;
			for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
				BYTE *line = FreeImage_GetScanLine(dib, y);
				for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
					RGBTRIPLE *triple = ((RGBTRIPLE *)line)+x;
					bgr.b = triple->rgbtBlue;
					bgr.g = triple->rgbtGreen;
					bgr.r = triple->rgbtRed;
					if (io->write_proc(&bgr, sizeof(FILE_BGR), 1, handle) != 1)
						return FALSE;
				}
			}
		} else if (bpp == 32) {
			FILE_BGRA bgra;
			for(int y = 0; y < FreeImage_GetHeight(dib); y++) {
				BYTE *line = FreeImage_GetScanLine(dib, y);
				for(int x = 0; x < FreeImage_GetWidth(dib); x++) {
					RGBQUAD *quad = ((RGBQUAD *)line)+x;
					bgra.b = quad->rgbBlue;
					bgra.g = quad->rgbGreen;
					bgra.r = quad->rgbRed;
					bgra.a = quad->rgbReserved;
					if (io->write_proc(&bgra, sizeof(FILE_BGRA), 1, handle) != 1)
						return FALSE;
				}
			}
#endif
		} else if (io->write_proc(FreeImage_GetBits(dib), FreeImage_GetHeight(dib) * FreeImage_GetPitch(dib), 1, handle) != 1) {
			return FALSE;
		}

		return TRUE;
	} else {
		return FALSE;
	}
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitBMP(Plugin *plugin, int format_id) {
	s_format_id = format_id;

	plugin->format_proc = Format;
	plugin->description_proc = Description;
	plugin->extension_proc = Extension;
	plugin->regexpr_proc = RegExpr;
	plugin->open_proc = NULL;
	plugin->close_proc = NULL;
	plugin->pagecount_proc = NULL;
	plugin->pagecapability_proc = NULL;
	plugin->load_proc = Load;
	plugin->save_proc = Save;
	plugin->validate_proc = Validate;
	plugin->mime_proc = MimeType;
	plugin->supports_export_bpp_proc = SupportsExportDepth;
	plugin->supports_export_type_proc = SupportsExportType;
	plugin->supports_icc_profiles_proc = NULL;	// not implemented yet;
}
