#ifndef _JPEG_H
#define _JPEG_H

#include <vector>

using namespace std;

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;


typedef struct s_DQT {
	u16		label = 0xFFDB;			//Define Quantization Table
	u16		dqtlen = 67;			//0x43 = 2+1+64=67
	u8		dqtacc_id = 0x00;		//acc=[7:4] 0:8bit, 1:16bit, id=[3:0] #0-3 (up to 4 tables)
	u8		QTable[64] = { 0 };		//init QTable[128] if acc=1 e.g. 16bit
}DQT;


typedef struct s_DHT_DC {
	u16		label = 0xFFC4;			//Define DC Huffman Table
	u16		dhtlen = 31;			//0x1F = 2+1+16+12
	u8		hufftype_id = 0x00;		//type=[7:4] 0:DC, 1:AC, id=[3:0] #0-1 (DC/AC id increases respectively)
	u8		DC_NRcodes[16] = { 0 };	//how many codes in each code_length (1-16)
	u8		DC_Values[12] = { 0 };	//Huffman tree num of node (不一定固定为12字节)
}DHT_DC;
typedef struct s_DHT_AC {
	u16		label = 0xFFC4;			//Define AC Huffman Table
	u16		dhtlen = 181;			//0xB5 = 2+1+16+162
	u8		hufftype_id = 0x10;
	u8		AC_NRcodes[16] = { 0 };
	u8		AC_Values[162] = { 0 };	//不一定固定为162字节
}DHT_AC;



/*
 * JPEG使用大端模式,下方多字节数据全部需要倒置fwrite
 * 
 * 下面结构体忽略了Exif结构或一些罕见的标识符:
 *		各标识符段之间无论出现多少0xFF都是合法的(至少要有一个)
 *	   >以下补充的标签后都有2字节指示段的长度:
 *		(APP1	0xE1		Exif struct seg			相机信息，作者版权信息等)
 *		APPn:	0xE1-0xEF	Applications			其他应用数据块(n=1,2,3,...,15)
 *		COM:	0xFE		Comments				注释块
 *		DNL:	0xDC		Def Number of Lines		影响SOF0，不过通常不支持，忽略其内容
 *		DRI:	0xDD		Def Restart Interval	DC部分的差分编码复位间隔，共6字节，前4字节为0xFFDD0004，后2字节表示n个MCU会出现RSTm标识符
 *		DAC:	0xCC		Def Arithmetic Table	算术编码表，因为版权法律问题，不能生产算术编码的JPEG图片，忽略其内容
 *     >以下补充的标签仅仅是个标签，其后不存在属于该段的任何内容，也没有段的长度信息:
 *		(SOI)(EOI)
 *		TEM:	0x01		未知意义				未知意义，直接掠过
 *		RSTm:	0xD0-0xD7	#m reset label			每n(DRI中指定)个MCU后的用于sync(同步)的复位标签(m=0,1,2,...,7)，该标签超过7后从0继续标记
 *     >其实还有一些极其罕见的标识符，为防止疏漏，编程时应该判断当前标签是否为常用标签，否则忽略其内容，而不应该直接判断它是不是罕见标签:
 *		SOF1-15:0xC1-0xCF	<不存在SOF4/8/12, 0xC4=DHT, 0xC8=JPG, 0xCC=DAC>
 *		JPG:	0xC8		保留/解码故障
 *		DHP:	0xDE
 *		EXP:	0xDF
 *		JPG0:	0xF0
 *		JPG13:	0xFD
 */
struct s_JPEG_header {
	u16		SOI = 0xFFD8;			//Start Of Image

	u16		APP0 = 0xFFE0;			//Application 0
	u16		app0len = 16;			//APP0's len, =16 for usual JPEG e.g. no thumbnail
	char	app0id[5] = "JFIF";		//"JFIF\0" = 0x4A46494600
	u16		jfifver = 0x0101;		//JFIF's version: 0x0101(v1.1) or 0x0102(v1.2)
	u8		xyunit = 0;				//x/y density's unit, 0:no unit, 1:dot/inch, 2:dot/cm
	u16		xden = 0x0001;			//x density
	u16		yden = 0x0001;			//y density
	u8		thumbh = 0;				//thumbnail horizon(width)
	u8		thumbv = 0;				//thumbnail vertical(height)

	DQT		DQT0;					//DQT for Luminance
	DQT		DQT1;					//DQT for Chrominance

	u16		SOF0 = 0xFFC0;			//Start Of Frame
	u16		sof0len = 0x0011;		//len = 17 for 24bit TrueColor
	u8		sof0acc = 0x08;			//acc = 0 (8 bit/sample), optional: 0x08(almost), 0x12, 0x16
	u16		imheight = 0;			//image height/pixel
	u16		imwidth = 0;			//image width/pixel
	u8		clrcomponent = 3;		//color components=3, e.g. JFIF只使用YCbCr(3),Exif这里也是3
	u8		clrY_id = 0x01;			//Y_component_id=1
	u8		clrY_sample = 0x11;			//Y_Hor_sam_factor=1, Y_Ver_sam_factor=1
	u8		clrY_QTable = 0x00;		//Y -> QTable #0
	u8		clrU_id = 0x02;			//U_component_id=2
	u8		clrU_sample = 0x11;			//
	u8		clrU_QTable = 0x01;		//U -> QTable #1
	u8		clrV_id = 0x03;			//V_component_id=3
	u8		clrV_sample = 0x11;			//
	u8		clrV_QTable = 0x01;		//V -> QTable #1
	/*
	u8		clrK_id = 0x04;			//JFIF不支持印刷工业的Cyan-Magenta-Yellow-blacK的CMYK四色空间
	u8		clrK_sample = 0x11;
	u8		clrK_QTable = 0x01;
	*/
	DHT_DC	DHT_DC0;				//DHT for Luminance
	DHT_AC	DHT_AC0;
	DHT_DC	DHT_DC1;				//DHT for Chrominance
	DHT_AC	DHT_AC1;

	u16		SOS = 0xFFDA;			//Start Of Scan
	u16		soslen = 12;			//0x0C
	u8		component = 3;			//Should equal to color component
	u16		Y_id_dht = 0x0100;		//Y: id=1, use #0 DC and #0 AC
	u16		U_id_dht = 0x0211;		//U: id=1, use #1 DC and #1 AC
	u16		V_id_dht = 0x0311;		//V: id=1, use #1 DC and #1 AC
	//always fit
	u8		SpectrumS = 0x00;		//spectrum start
	u8		SpectrumE = 0x3F;		//spectrum end
	u8		SpectrumC = 0x00;		//spectrum choose

	//Here is filled with Compressed-Image-Data

	u16		EOI = 0xFFD9;			//End Of Image
	//init
	s_JPEG_header() { app0id[0] = 'J'; app0id[1] = 'F'; app0id[2] = 'I'; app0id[3] = 'F'; app0id[4] = 0; }
};


typedef	struct s_BitString {
	int code;
	int	len;

	s_BitString() { code = 0; len = 0; }
}BitString;
typedef struct s_BitString HuffType;


static const double DCT[8][8] = {
	{0.35355,	0.35355,	0.35355,	0.35355,	0.35355,	0.35355,	0.35355,	0.35355},
	{0.49039,	0.41573,	0.27779,	0.09756,	-0.09756,	-0.27779,	-0.41573,	-0.49039},
	{0.46194,	0.19134,	-0.19134,	-0.46194,	-0.46194,	-0.19134,	0.19134,	0.46194},
	{0.41573,	-0.09755,	-0.49039,	-0.27779,	0.27779,	0.49039,	0.09755,	-0.41573},
	{0.35355,	-0.35355,	-0.35355,	0.35355,	0.35355,	-0.35355,	-0.35355,	0.35355},
	{0.27779,	-0.49039,	0.09755,	0.41573,	-0.41573,	-0.09755,	0.49039,	-0.27779},
	{0.19134,	-0.46194,	0.46194,	-0.19134,	-0.19134,	0.46194,	-0.46194,	0.19134},
	{0.09755,	-0.27779,	0.41573,	-0.49039,	0.49039,	-0.41573,	0.27779,	-0.09754}
};

static const double DCT_T[8][8] = {
	{0.35355,	0.49039,	0.46194,	0.41573,	0.35355,	0.27779,	0.19134,	0.09755},
	{0.35355,	0.41573,	0.19134,	-0.09755,	-0.35355,	-0.49039,	-0.46194,	-0.27779},
	{0.35355,	0.27779,	-0.19134,	-0.49039,	-0.35355,	0.09755,	0.46194,	0.41573},
	{0.35355,	0.09755,	-0.46194,	-0.27779,	0.35355,	0.41573,	-0.19134,	-0.49039},
	{0.35355,	-0.09755,	-0.46194,	0.27779,	0.35355,	-0.41573,	-0.19134,	0.49039},
	{0.35355,	-0.27779,	-0.19134,	0.49039,	-0.35355,	-0.09755,	0.46194,	-0.41573},
	{0.35355,	-0.41573,	0.19134,	0.09755,	-0.35355,	0.49039,	-0.46194,	0.27779},
	{0.35355,	-0.49039,	0.46194,	-0.41573,	0.35355,	-0.27779,	0.19134,	-0.09754}
};


static const double DCT16[16][16] = {
	{0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000,  0.250000},
	{0.351851,  0.338330,  0.311806,  0.273300,  0.224292,  0.166664,  0.102631,  0.034654, -0.034654, -0.102631, -0.166664, -0.224292, -0.273300, -0.311806, -0.338330, -0.351851},
	{0.346760,	0.293969,  0.196424,  0.068975, -0.068975, -0.196424, -0.293969, -0.346760, -0.346760, -0.293969, -0.196424, -0.068975,  0.068975,  0.196424,  0.293969,  0.346760},
	{0.338330,  0.224292,  0.034654, -0.166664, -0.311806, -0.351851, -0.273300, -0.102631,  0.102631,  0.273300,  0.351851,  0.311806,  0.166664, -0.034654, -0.224292, -0.338330},
	{0.326641,  0.135300, -0.135299, -0.326641, -0.326641, -0.135299,  0.135299,  0.326641,  0.326641,  0.135299, -0.135299, -0.326641, -0.326641, -0.135299,  0.135299,  0.326641},
	{0.311806,  0.034654, -0.273300, -0.338330, -0.102631,  0.224292,  0.351851,  0.166664, -0.166664, -0.351851, -0.224292,  0.102631,  0.338330,  0.273300, -0.034654, -0.311806},
	{0.293969, -0.068975, -0.346760, -0.196424,  0.196424,  0.346760,  0.068975, -0.293969, -0.293969,  0.068975,  0.346760,  0.196424, -0.196424, -0.346760, -0.068975,  0.293969},
	{0.273300, -0.166664, -0.338330,  0.034654,  0.351851,  0.102631, -0.311806, -0.224292,  0.224292,  0.311806, -0.102631, -0.351851, -0.034654,  0.338330,  0.166664, -0.273300},
	{0.250000, -0.250000, -0.250000,  0.250000,  0.250000, -0.250000, -0.250000,  0.250000,  0.250000, -0.250000, -0.250000,  0.250000,  0.250000, -0.250000, -0.250000,  0.250000},
	{0.224292, -0.311806, -0.102631,  0.351851, -0.034654, -0.338330,  0.166664,  0.273300, -0.273300, -0.166664,  0.338330,  0.034654, -0.351851,  0.102631,  0.311806, -0.224292},
	{0.196424, -0.346760,  0.068975,  0.293969, -0.293969, -0.068975,  0.346760, -0.196424, -0.196424,  0.346760, -0.068975, -0.293969,  0.293969,  0.068975, -0.346760,  0.196424},
	{0.166664, -0.351851,  0.224292,  0.102631, -0.338330,  0.273300,  0.034654, -0.311806,  0.311806, -0.034654, -0.273300,  0.338330, -0.102631, -0.224292,  0.351851, -0.166664},
	{0.135299, -0.326641,  0.326641, -0.135299, -0.135299,  0.326641, -0.326641,  0.135299,  0.135299, -0.326641,  0.326641, -0.135299, -0.135299,  0.326641, -0.326641,  0.135299},
	{0.102631, -0.273300,  0.351851, -0.311806,  0.166664,  0.034654, -0.224292,  0.338330, -0.338330,  0.224292, -0.034654, -0.166664,  0.311806, -0.351851,  0.273300, -0.102631},
	{0.068975, -0.196424,  0.293969, -0.346760,  0.346760, -0.293969,  0.196424, -0.068975, -0.068975,  0.196424, -0.293969,  0.346760, -0.346760,  0.293969, -0.196424,  0.068975},
	{0.034654, -0.102631,  0.166664, -0.224292,  0.273300, -0.311806,  0.338330, -0.351851,  0.351851, -0.338330,  0.311806, -0.273300,  0.224292, -0.166664,  0.102631, -0.034654}
};

static const double DCT16_T[16][16] = {
	{0.250000,  0.351851,  0.346760,  0.338330,  0.326641,  0.311806,  0.293969,  0.273300,  0.250000,  0.224292,  0.196424,  0.166664,  0.135299,  0.102631,  0.068975,  0.034654},
	{0.250000,  0.338330,  0.293969,  0.224292,  0.135299,  0.034654, -0.068975, -0.166664, -0.250000, -0.311806, -0.346760, -0.351851, -0.326641, -0.273300, -0.196424, -0.102631},
	{0.250000,  0.311806,  0.196424,  0.034654, -0.135299, -0.273300, -0.346760, -0.338330, -0.250000, -0.102631,  0.068975,  0.224292,  0.326641,  0.351851,  0.293969,  0.166664},
	{0.250000,  0.273300,  0.068975, -0.166664, -0.326641, -0.338330, -0.196424,  0.034654,  0.250000,  0.351851,  0.293969,  0.102631, -0.135299, -0.311806, -0.346760, -0.224292},
	{0.250000,  0.224292, -0.068975, -0.311806, -0.326641, -0.102631,  0.196424,  0.351851,  0.250000, -0.034654, -0.293969, -0.338330, -0.135299,  0.166664,  0.346760,  0.273300},
	{0.250000,  0.166664, -0.196424, -0.351851, -0.135299,  0.224292,  0.346760,  0.102631, -0.250000, -0.338330, -0.068975,  0.273300,  0.326641,  0.034654, -0.293969, -0.311806},
	{0.250000,  0.102631, -0.293969, -0.273300,  0.135299,  0.351851,  0.068975, -0.311806, -0.250000,  0.166664,  0.346760,  0.034654, -0.326641, -0.224292,  0.196424,  0.338330},
	{0.250000,  0.034654, -0.346760, -0.102631,  0.326641,  0.166664, -0.293969, -0.224292,  0.250000,  0.273300, -0.196424, -0.311806,  0.135299,  0.338330, -0.068975, -0.351851},
	{0.250000, -0.034654, -0.346760,  0.102631,  0.326641, -0.166664, -0.293969,  0.224292,  0.250000, -0.273300, -0.196424,  0.311806,  0.135299, -0.338330, -0.068975,  0.351851},
	{0.250000, -0.102631, -0.293969,  0.273300,  0.135299, -0.351851,  0.068975,  0.311806, -0.250000, -0.166664,  0.346760, -0.034654, -0.326641,  0.224292,  0.196424, -0.338330},
	{0.250000, -0.166664, -0.196424,  0.351851, -0.135299, -0.224292,  0.346760, -0.102631, -0.250000,  0.338330, -0.068975, -0.273300,  0.326641, -0.034654, -0.293969,  0.311806},
	{0.250000, -0.224292, -0.068975,  0.311806, -0.326641,  0.102631,  0.196424, -0.351851,  0.250000,  0.034654, -0.293969,  0.338330, -0.135299, -0.166664,  0.346760, -0.273300},
	{0.250000, -0.273300,  0.068975,  0.166664, -0.326641,  0.338330, -0.196424, -0.034654,  0.250000, -0.351851,  0.293969, -0.102631, -0.135299,  0.311806, -0.346760,  0.224292},
	{0.250000, -0.311806,  0.196424, -0.034654, -0.135299,  0.273300, -0.346760,  0.338330, -0.250000,  0.102631,  0.068975, -0.224292,  0.326641, -0.351851,  0.293969, -0.166664},
	{0.250000, -0.338330,  0.293969, -0.224292,  0.135299, -0.034654, -0.068975,  0.166664, -0.250000,  0.311806, -0.346760,  0.351851, -0.326641,  0.273300, -0.196424,  0.102631},
	{0.250000, -0.351851,  0.346760, -0.338330,  0.326641, -0.311806,  0.293969, -0.273300,  0.250000, -0.224292,  0.196424, -0.166664,  0.135299, -0.102631,  0.068975, -0.034654}
};


//Q-Table in *.jpeg file has been ZigZaged
/*static unsigned char Q_Y[64] = {
	16, 11, 12, 14, 12, 10, 16, 14,
	13, 14, 18, 17, 16, 19, 24, 40,
	26, 24, 22, 22, 24, 49, 35, 37,
	29, 40, 58, 51, 61, 60, 57, 51,
	56, 55, 64, 72, 92, 78, 64, 68,
	87, 69, 55, 56, 80, 109,81, 87,
	95, 98, 103,104,103,62, 77,113,
	121,112,100,120,92, 101,103,99
};*/
static unsigned char Q_Y[64] = {
	 5,  3,  4,  4,  4,  3,  5,  4,
	 4,  4,  5,  5,  5,  6,  7, 12,
	 8,  7,  7,  7,  7, 15, 12, 12,
	 9, 12, 17, 15, 18, 18, 17, 15,
	17, 17, 19, 22, 28, 23, 19, 20,
	26, 21, 17, 17, 24, 33, 24, 26,
	29, 29, 29, 31, 31, 31, 19, 23,
	34, 36, 34, 30, 36, 30, 31, 30
};


/*static unsigned char Q_UV[64] = {
	17, 18, 18, 24, 21, 24, 47, 26,
	26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 66, 99 ,99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};*/
static unsigned char Q_UV[64] = {
	 5,  5,  5,  7,  6,  7, 14,  8,
	 8, 14, 30, 20, 17, 20, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30,
};


static const unsigned char RLE_ZigZag[64][2] = {
	{0, 0},{0, 1},{1, 0},{2, 0},{1, 1},{0, 2},{0, 3},{1, 2},
	{2, 1},{3, 0},{4, 0},{3, 1},{2, 2},{1, 3},{0, 4},{0, 5},
	{1, 4},{2, 3},{3, 2},{4, 1},{5, 0},{6, 0},{5, 1},{4, 2},
	{3, 3},{2, 4},{1, 5},{0, 6},{0, 7},{1, 6},{2, 5},{3, 4},
	{4, 3},{5, 2},{6, 1},{7, 0},{7, 1},{6, 2},{5, 3},{4, 4},
	{3, 5},{2, 6},{1, 7},{2, 7},{3, 6},{4, 5},{5, 4},{6, 3},
	{7, 2},{7, 3},{6, 4},{5, 5},{4, 6},{3, 7},{4, 7},{5, 6},
	{6, 5},{7, 4},{7, 5},{6, 6},{5, 7},{6, 7},{7, 6},{7, 7}
};


static const unsigned char Standard_Luminance_DC_NRCodes[16] =	{ 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char Standard_Luminance_DC_Values[12] =	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };


static const unsigned char Standard_Chrominance_DC_NRCodes[16] ={ 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const unsigned char Standard_Chrominance_DC_Values[12] =	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };


static const unsigned char Standard_Luminance_AC_NRCodes[16] =	{ 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7D };
static const unsigned char Standard_Luminance_AC_Values[162] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};


static const unsigned char Standard_Chrominance_AC_NRCodes[16] =	{ 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
static const unsigned char Standard_Chrominance_AC_Values[162] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};



static HuffType	HuffTbl_Y_DC[12];				//注意非标准的DC表很有可能大于12(这里仅用于标准表)
static HuffType	HuffTbl_Y_AC[256];
static HuffType	HuffTbl_UV_DC[12];
static HuffType	HuffTbl_UV_AC[256];
static u8		fit_Q_Y[64] = { 0 };			//注意本程序中Q_Y,Q_U都是已经ZigZag的,所以量子化时应该把Q表进行反ZigZag操作再使用
static u8		fit_Q_UV[64] = { 0 };
static u8		Q_Y_wr[64] = { 0 };				//专门往jpeg文件写入的QTable,和标准Table元素位置完全对应,仅仅是它乘以某个系数
static u8		Q_UV_wr[64] = { 0 };
static u8		initHuff_flag = 0;
static double	init_quality_444 = 0.0;
static double	init_quality_411 = 0.0;


static u16	max_first_Lumi_DC[17] = { 0 }, max_first_Lumi_AC[17] = { 0 }, max_first_Chromi_DC[17] = { 0 }, max_first_Chromi_AC[17] = { 0 };
static u8	HuffTbl_len_Y_DC[17][130] = { 0 }, HuffTbl_len_Y_AC[17][130] = { 0 }, HuffTbl_len_UV_DC[17][130] = { 0 }, HuffTbl_len_UV_AC[17][130] = { 0 };




//func in JPEG_std.cpp
int		round_double(double val);
void	show_2dmatrix(int m[][8]);
void	show_2dmatrix(double m[][8]);

void	matrix_multiply(double res[][8], const double ma[][8], const double mb[][8], int odr = 8);
void	Forward_DCT(double res[][8], const double block[][8]);
void	Inverse_DCT(double res[][8], const double block[][8]);
void	Quantize_down_process(int res[][8], const double input[][8], const u8* Qm);
void	RLE_process(vector<int>* pvrle, const int afterQ[][8], int* prev_DC);
void	Bit_Coding_process(vector<int>* pvrle, vector<int>* pvbitcode);
void	Build_Huffman_Table(const u8* nr_codes, const u8* std_table, HuffType* huffman_table);
int		Huffman_encoding_process(vector<int>* pvbitcode, const HuffType* HTDC, const HuffType* HTAC, BitString* outputBitString);
void	write_BitString_Sequence(BitString* huff_outBitString, int BitString_count, int* newByte, int* newBytePos, FILE* fp);
void	Write_JPEG_444(FILE* fp, const int width, const int height, const u8* yuv_Y, const u8* yuv_U, const u8* yuv_V, double quality = 1.0);
void	Write_JPEG_420(FILE* fp, const int width, const int height, const u8* yuv_Y, const u8* yuv_U, const u8* yuv_V, double quality = 1.0);

u32*	BigEnd32(u32* ptr);
u16*	BigEnd16(u16* ptr);
short	clamp(short val, short bot, short top);



//func in JPEG_dec.cpp
//but Inv-DCT() is in JPEG_std.h
void	Build_Huffman_Table(const u8* nr_codes, const u8* std_table, HuffType* huffman_table, u16* max_in_ThisLen, u8 huff_len_table[][130]);
void	rd_restore_HuffBit_RLE(vector<int>* pvrle, int* pnewByte, int* pnewBytePos, int* prev_DC, const u16* max_first_DC, const u16* max_first_AC, \
		u8 huff_lentbl_DC[][130], u8 huff_lentbl_AC[][130], HuffType* rd_hufftbl_DC, HuffType* rd_hufftbl_AC, FILE* fp);
void	rd_restore_InvQuantize(double afterInvQ[][8], vector<int>* pvrle, const u8* Qm);

void	pre_rd_DHT_info(FILE* fp, u16* pLumi_DC_Values_len, u16* pLumi_AC_Values_len, u16* pChromi_DC_Values_len, u16* pChromi_AC_Values_len);
void	pre_rd_SOF0_info(FILE* fp, u8* pY_sample, u16* imw, u16* imh);
void	Dec_JPEG_to_YUV(FILE* fp, u8* yuv_Y, u8* yuv_U, u8* yuv_V, bool toYUV420);


#endif
