#include <stdio.h>
#include <stdlib.h>		//malloc, free

#include <iostream>
#include <vector>

#include "JPEG.h"

using namespace std;


int round_double(double val)
{
	double res = (val > 0.0) ? (val + 0.5) : (val - 0.5);
	return (int)res;
}


void show_2dmatrix(int m[][8])
{
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			cout << m[i][j] << "  ";
		}
		cout << endl;
	}
	cout << endl;
}


void show_2dmatrix(double m[][8])
{
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			cout << m[i][j] << "  ";
		}
		cout << endl;
	}
	cout << endl;
}


void matrix_multiply(double res[][8], const double ma[][8], const double mb[][8], int odr)
{
	if (odr != 8) { cout << "*** order != 8 ***" << endl; return; }
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				res[i][j] += ma[i][t] * mb[t][j];
			}
		}
	}
}


void Forward_DCT(double res[][8], const double block[][8])
{
	double temp[8][8] = { 0.0 };
	/*matrix_multiply(temp, DCT, block);
	matrix_multiply(res, temp, DCT_T);*/
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				temp[i][j] += DCT[i][t] * block[t][j];
			}
		}
	}
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				res[i][j] += temp[i][t] * DCT_T[t][j];
			}
		}
	}
}


void Inverse_DCT(double res[][8], const double block[][8])
{
	double temp[8][8] = { 0.0 };
	/*matrix_multiply(temp, DCT_T, block);
	matrix_multiply(res, temp, DCT);*/
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				temp[i][j] += DCT_T[i][t] * block[t][j];
			}
		}
	}
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			for (int t = 0; t < 8; t++) {
				res[i][j] += temp[i][t] * DCT[t][j];
			}
		}
	}
}


void Quantize_down_process(int res[][8], const double input[][8], const u8* Qm)//量子化
{
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			res[i][j] = round_double(input[i][j] / Qm[i * 8 + j]);
		}
	}
}


void RLE_process(vector<int>* pvrle, const int afterQ[][8], int* prev_DC)	//RunLengthEncoding,行程编码
{
	int raw[64] = { 0 };
	for (int p = 0; p < 64; p++) {		//ZigZag RLE
		raw[p] = afterQ[RLE_ZigZag[p][0]][RLE_ZigZag[p][1]];
	}
	int prev = *prev_DC;
	*prev_DC = raw[0];					//update
	raw[0] -= prev;						//Diff-code-DC
	pvrle->push_back(0);				//上来立马把DC的值压进vector,防止DC差分为0时和EOB分不清
	pvrle->push_back(raw[0]);

	int idx = 1, i = 1;
	int zeros = 0;
	while (idx != 64)
	{
		zeros = 0;
		while (raw[idx] == 0)
		{
			for (i = idx; i < 64; i++) {
				if (raw[i] != 0)
					break;
			}
			if (i == 64) {				//EOB -> (0,0)
				pvrle->push_back(0);
				pvrle->push_back(0);
				return;
			}
			zeros++;
			if(zeros == 16) {
				pvrle->push_back(15);
				pvrle->push_back(0);
				zeros = 0;
			}
			idx++;
		}
		pvrle->push_back(zeros);
		pvrle->push_back(raw[idx]);
		idx++;
	}
}


void Bit_Coding_process(vector<int>* pvrle, vector<int>* pvbitcode)
{
	unsigned char	code_len = 0;
	int				bit_not = 0;
	for (int i = 0; i < (int)pvrle->size() - 1; i += 2) {
		int val = pvrle->at((size_t)i + 1);
		int aval = abs(val);
		if (aval >= 16384) {
			code_len = 15;
			bit_not = 32767;
		}
		else if (aval >= 8192) {
			code_len = 14;
			bit_not = 16383;
		}
		else if (aval >= 4096) {
			code_len = 13;
			bit_not = 8191;
		}
		else if (aval >= 2048) {
			code_len = 12;
			bit_not = 4095;
		}
		else if (aval >= 1024) {
			code_len = 11;
			bit_not = 2047;
		}
		else if (aval >= 512) {
			code_len = 10;
			bit_not = 1023;
		}
		else if (aval >= 256) {
			code_len = 9;
			bit_not = 511;
		}
		else if (aval >= 128) {
			code_len = 8;
			bit_not = 255;
		}
		else if (aval >= 64) {
			code_len = 7;
			bit_not = 127;
		}
		else if (aval >= 32) {
			code_len = 6;
			bit_not = 63;
		}
		else if (aval >= 16) {
			code_len = 5;
			bit_not = 31;
		}
		else if (aval >= 8) {
			code_len = 4;
			bit_not = 15;
		}
		else if (aval >= 4) {
			code_len = 3;
			bit_not = 7;
		}
		else if (aval >= 2) {
			code_len = 2;
			bit_not = 3;
		}
		else if (aval == 1) {
			code_len = 1;
			bit_not = 1;
		}
		else if (aval == 0) {
			code_len = 0;
			bit_not = 0;
		}
		else;

		if (val == 0) {
			pvbitcode->push_back((pvrle->at(i) << 4) + code_len);
		}
		else {
			if (val > 0) {
				pvbitcode->push_back((pvrle->at(i) << 4) + code_len);
				pvbitcode->push_back(val);
			}
			else {
				pvbitcode->push_back((pvrle->at(i) << 4) + code_len);
				pvbitcode->push_back(bit_not & (~aval));
			}
		}
	}
}


void Build_Huffman_Table(const u8* nr_codes, const u8* std_table, HuffType* huffman_table)
{
	unsigned char pos_in_table = 0;
	unsigned short code_value = 0;

	for (int k = 1; k <= 16; k++)
	{
		for (int j = 1; j <= nr_codes[k - 1]; j++)
		{
			huffman_table[std_table[pos_in_table]].code = code_value;
			huffman_table[std_table[pos_in_table]].len = k;
			pos_in_table++;
			code_value++;
		}
		code_value <<= 1;
	}
}


int Huffman_encoding_process(vector<int>* pvbitcode, const HuffType* HTDC, const HuffType* HTAC, BitString* outputBitString)
{
	int x = 1;
	if (pvbitcode->at(0) == 0)							//DC
		outputBitString[0] = HTDC[pvbitcode->at(0)];
	else {
		outputBitString[0] = HTDC[pvbitcode->at(0)];
		outputBitString[1].code = pvbitcode->at(1);
		outputBitString[1].len = pvbitcode->at(0) & 0x0F;
		x = 2;
	}

	for (int i = x; i < pvbitcode->size(); i+=2) {
		if (pvbitcode->at(i) == 0xF0) {
			outputBitString[i] = HTAC[0xF0];
			i -= 1;
		}
		else if (pvbitcode->at(i) == 0x00) {
			outputBitString[i] = HTAC[0x00];
			break;
		}
		else {
			outputBitString[i] = HTAC[pvbitcode->at(i)];//AC
			outputBitString[i + 1].code = pvbitcode->at((size_t)i + 1);
			outputBitString[i + 1].len = pvbitcode->at(i) & 0x0F;
		}
	}
	return (u8)pvbitcode->size();
}


void write_BitString_Sequence(BitString* huff_outBitString, int BitString_count, int* newByte, int* newBytePos, FILE* fp)
{
	//outer-func default newByte = 0, newBytePos = 7
	static unsigned short mask[16] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768 };
	for (int i = 0; i < BitString_count; i++) {
		int value = huff_outBitString[i].code;
		int posval = huff_outBitString[i].len - 1;
		while (posval >= 0)
		{
			if ((value & mask[posval]) != 0)
				*newByte |= mask[*newBytePos];
			posval--;
			(*newBytePos)--;
			if (*newBytePos < 0) {

				unsigned char nB = *newByte;
				fwrite(&nB, 1, 1, fp);
				if (nB == 0xFF) {				//Handle special case
					nB = 0x00;					//0xFF in JPEG is likely a header label
					fwrite(&nB, 1, 1, fp);		//add 0x00 to inform it is a normal data
				}

				//Re-initialize
				*newBytePos = 7;
				*newByte = 0;

			}
		}
	}
}



//4Byte小端转换为大端
u32* BigEnd32(u32* ptr)
{
	u32* ptemp = ptr;
	u8	b1 = (u8)(*ptemp);
	u8	b2 = (u8)(*ptemp >> 8);
	u8	b3 = (u8)(*ptemp >> 16);
	u8	b4 = (u8)(*ptemp >> 24);
	*ptemp = (u32)b4 + ((u32)b3 << 8) + ((u32)b2 << 16) + ((u32)b1 << 24);
	return ptemp;
}


//2Byte小端转换为大端
u16* BigEnd16(u16* ptr)
{
	u16* ptemp = ptr;
	u8	b1 = (u8)(*ptemp);
	u8	b2 = (u8)(*ptemp >> 8);
	*ptemp = (u16)b2 + ((u16)b1 << 8);
	return ptemp;
}


//上下端钳位
short clamp(short val, short bot, short top)
{
	if (val >= top)		val = top;
	else if (val <= bot)	val = bot;
	else;
	return val;
}



/*
 * intro:	根据需要可以改写文件头或者采样因子
 * param:	quality	可取[0.3 , 20.0], 取1.0时使用标准量化表, 注意quality约大图像质量越好
 * note:	本函数没有使用fclose()来关闭文件
 *			yuv_Y,yuv_U,yuv_V必须是yuv444格式的数组,即YUV三个分量应该同等大小
 *			写入到JPEG中后依然是444格式,即该函数实现 yuv444 --> jpeg(444)
 */
void Write_JPEG_444(FILE* fp, const int width, const int height, const u8* yuv_Y, const u8* yuv_U, const u8* yuv_V, double quality)
{
	struct s_JPEG_header Jheader;
	if (quality < 0.3)		quality = 0.3;
	if (quality > 20.0)		quality = 20.0;
	
	//init
	if (initHuff_flag == 0) {
		Build_Huffman_Table(Standard_Luminance_DC_NRCodes, Standard_Luminance_DC_Values, HuffTbl_Y_DC);
		Build_Huffman_Table(Standard_Luminance_AC_NRCodes, Standard_Luminance_AC_Values, HuffTbl_Y_AC);
		Build_Huffman_Table(Standard_Chrominance_DC_NRCodes, Standard_Chrominance_DC_Values, HuffTbl_UV_DC);
		Build_Huffman_Table(Standard_Chrominance_AC_NRCodes, Standard_Chrominance_AC_Values, HuffTbl_UV_AC);
		initHuff_flag = 1;												//label the flag
	}

	//quality
	if (quality != init_quality_444) {									//Inverse-ZigZag Quantize Table to fit the quality
		for (int m = 0; m < 64; m++) {
			int aa = (int)((double)Q_Y[m] / quality + 0.5);
			if (aa <= 0)		aa = 1;
			if (aa > 0xFF)		aa = 0xFF;
			fit_Q_Y[RLE_ZigZag[m][0] * 8 + RLE_ZigZag[m][1]] = (u8)aa;
			Q_Y_wr[m] = (u8)aa;

			int bb = (int)((double)Q_UV[m] / quality + 0.5);
			if (bb <= 0)		bb = 1;
			if (bb > 0xFF)		bb = 0xFF;
			fit_Q_UV[RLE_ZigZag[m][0] * 8 + RLE_ZigZag[m][1]] = (u8)bb;
			Q_UV_wr[m] = (u8)bb;
		}
		init_quality_411 = quality;										//label the flag
	}

	//revise
	Jheader.DQT1.dqtacc_id = 0x01;
	Jheader.imheight = height;
	Jheader.imwidth = width;
	Jheader.clrY_sample = 0x11;
	Jheader.clrU_sample = 0x11;
	Jheader.clrV_sample = 0x11;
	Jheader.DHT_DC1.hufftype_id = 0x01;
	Jheader.DHT_AC1.hufftype_id = 0x11;
	
	//write header
	fwrite(BigEnd16(&Jheader.SOI),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.APP0),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.app0len),			sizeof(u16), 1, fp);
	fwrite(Jheader.app0id, 1, 5, fp);
	fwrite(BigEnd16(&Jheader.jfifver),			sizeof(u16), 1, fp);
	fwrite(&Jheader.xyunit, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.xden),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.yden),				sizeof(u16), 1, fp);
	fwrite(&Jheader.thumbh, 1, 1, fp);
	fwrite(&Jheader.thumbv, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.DQT0.label),		sizeof(u16), 1, fp);	//DQT_Lumi(Y)
	fwrite(BigEnd16(&Jheader.DQT0.dqtlen),		sizeof(u16), 1, fp);
	fwrite(&Jheader.DQT0.dqtacc_id, 1, 1, fp);
	fwrite(Q_Y_wr, 1, 64, fp);
	fwrite(BigEnd16(&Jheader.DQT1.label),		sizeof(u16), 1, fp);	//DQT_Chro(UV)
	fwrite(BigEnd16(&Jheader.DQT1.dqtlen),		sizeof(u16), 1, fp);
	fwrite(&Jheader.DQT1.dqtacc_id, 1, 1, fp);
	fwrite(Q_UV_wr, 1, 64, fp);
	fwrite(BigEnd16(&Jheader.SOF0),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.sof0len),			sizeof(u16), 1, fp);
	fwrite(&Jheader.sof0acc, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.imheight),			sizeof(u16), 1, fp);	//image_height
	fwrite(BigEnd16(&Jheader.imwidth),			sizeof(u16), 1, fp);	//image_width
	fwrite(&Jheader.clrcomponent, 1, 1, fp);
	fwrite(&Jheader.clrY_id, 1, 1, fp);
	fwrite(&Jheader.clrY_sample, 1, 1, fp);								//Y_sample_factor
	fwrite(&Jheader.clrY_QTable, 1, 1, fp);
	fwrite(&Jheader.clrU_id, 1, 1, fp);
	fwrite(&Jheader.clrU_sample, 1, 1, fp);								//U_sample_factor
	fwrite(&Jheader.clrU_QTable, 1, 1, fp);
	fwrite(&Jheader.clrV_id, 1, 1, fp);
	fwrite(&Jheader.clrV_sample, 1, 1, fp);								//V_sample_factor
	fwrite(&Jheader.clrV_QTable, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.DHT_DC0.label),	sizeof(u16), 1, fp);	//DHT_Lumi(Y)_DC
	fwrite(BigEnd16(&Jheader.DHT_DC0.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_DC0.hufftype_id, 1, 1, fp);
	fwrite(Standard_Luminance_DC_NRCodes, 1, 16, fp);
	fwrite(Standard_Luminance_DC_Values, 1, 12, fp);
	fwrite(BigEnd16(&Jheader.DHT_AC0.label),	sizeof(u16), 1, fp);	//DHT_Lumi(Y)_AC
	fwrite(BigEnd16(&Jheader.DHT_AC0.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_AC0.hufftype_id, 1, 1, fp);
	fwrite(Standard_Luminance_AC_NRCodes, 1, 16, fp);
	fwrite(Standard_Luminance_AC_Values, 1, 162, fp);
	fwrite(BigEnd16(&Jheader.DHT_DC1.label),	sizeof(u16), 1, fp);	//DHT_Chro(UV)_DC
	fwrite(BigEnd16(&Jheader.DHT_DC1.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_DC1.hufftype_id, 1, 1, fp);
	fwrite(Standard_Chrominance_DC_NRCodes, 1, 16, fp);
	fwrite(Standard_Chrominance_DC_Values, 1, 12, fp);
	fwrite(BigEnd16(&Jheader.DHT_AC1.label),	sizeof(u16), 1, fp);	//DHT_Chro(UV)_AC
	fwrite(BigEnd16(&Jheader.DHT_AC1.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_AC1.hufftype_id, 1, 1, fp);
	fwrite(Standard_Chrominance_AC_NRCodes, 1, 16, fp);
	fwrite(Standard_Chrominance_AC_Values, 1, 162, fp);
	fwrite(BigEnd16(&Jheader.SOS),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.soslen),			sizeof(u16), 1, fp);
	fwrite(&Jheader.component, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.Y_id_dht),			sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.U_id_dht),			sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.V_id_dht),			sizeof(u16), 1, fp);
	fwrite(&Jheader.SpectrumS, 1, 1, fp);
	fwrite(&Jheader.SpectrumE, 1, 1, fp);
	fwrite(&Jheader.SpectrumC, 1, 1, fp);


	int	newByte = 0, newBytePos = 7;
	int	prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;

	for (int yPos = 0; yPos < height; yPos += 8) {
		for (int xPos = 0; xPos < width; xPos += 8) {
			double MCU_Y[8][8] = { 0.0 }, MCU_U[8][8] = { 0.0 }, MCU_V[8][8] = { 0.0 };
			for (u8 i = 0; i < 8; i++) {
				for (u8 j = 0; j < 8; j++) {
					MCU_Y[i][j] = (double)yuv_Y[(yPos + i) * width + xPos + j] - 128.0;
					MCU_U[i][j] = (double)yuv_U[(yPos + i) * width + xPos + j] - 128.0;
					MCU_V[i][j] = (double)yuv_V[(yPos + i) * width + xPos + j] - 128.0;
				}
			}


			double		res_FDCT_Y[8][8] = { 0.0 }, res_FDCT_U[8][8] = { 0.0 }, res_FDCT_V[8][8] = { 0.0 };
			int			res_Quanti_Y[8][8] = { 0 }, res_Quanti_U[8][8] = { 0 }, res_Quanti_V[8][8] = { 0 };
			vector<int> vrle_Y, vrle_U, vrle_V;
			vector<int> vbitcode_Y, vbitcode_U, vbitcode_V;
			BitString	outBitStr_Y[200], outBitStr_U[200], outBitStr_V[200];
			int			BitStrCount_Y = 0, BitStrCount_U = 0, BitStrCount_V = 0;

			//Y-channel
			Forward_DCT(res_FDCT_Y, MCU_Y);
			Quantize_down_process(res_Quanti_Y, res_FDCT_Y, fit_Q_Y);
			RLE_process(&vrle_Y, res_Quanti_Y, &prev_DC_Y);
			Bit_Coding_process(&vrle_Y, &vbitcode_Y);
			BitStrCount_Y = Huffman_encoding_process(&vbitcode_Y, HuffTbl_Y_DC, HuffTbl_Y_AC, outBitStr_Y);
			write_BitString_Sequence(outBitStr_Y, BitStrCount_Y, &newByte, &newBytePos, fp);

			//U-channel
			Forward_DCT(res_FDCT_U, MCU_U);
			Quantize_down_process(res_Quanti_U, res_FDCT_U, fit_Q_UV);
			RLE_process(&vrle_U, res_Quanti_U, &prev_DC_U);
			Bit_Coding_process(&vrle_U, &vbitcode_U);
			BitStrCount_U = Huffman_encoding_process(&vbitcode_U, HuffTbl_UV_DC, HuffTbl_UV_AC, outBitStr_U);
			write_BitString_Sequence(outBitStr_U, BitStrCount_U, &newByte, &newBytePos, fp);

			//V-channel
			Forward_DCT(res_FDCT_V, MCU_V);
			Quantize_down_process(res_Quanti_V, res_FDCT_V, fit_Q_UV);
			RLE_process(&vrle_V, res_Quanti_V, &prev_DC_V);
			Bit_Coding_process(&vrle_V, &vbitcode_V);
			BitStrCount_V = Huffman_encoding_process(&vbitcode_V, HuffTbl_UV_DC, HuffTbl_UV_AC, outBitStr_V);
			write_BitString_Sequence(outBitStr_V, BitStrCount_V, &newByte, &newBytePos, fp);
		}
	}
	if (newBytePos != 7) {
		fwrite(&newByte, 1, 1, fp);	//防止JPEG的最后一点点bit流没有被写进文件
	}

	fwrite(BigEnd16(&Jheader.EOI),				sizeof(u16), 1, fp);
}




/*
 * intro:	根据需要可以改写文件头或者采样因子
 * param:	quality	可取[0.3 , 20.0], 取1.0时使用标准量化表, 注意quality约大图像质量越好
 * note:	本函数没有使用fclose()来关闭文件
 *			yuv_Y,yuv_U,yuv_V必须是yuv444格式的数组,即YUV三个分量应该同等大小
 *			写入到JPEG中后是411格式,即该函数实现 yuv444 --> jpeg(411)
 */
void Write_JPEG_420(FILE* fp, const int width, const int height, const u8* yuv_Y, const u8* yuv_U, const u8* yuv_V, double quality)
{
	struct s_JPEG_header Jheader;
	if (quality < 0.3)		quality = 0.3;
	if (quality > 20.0)		quality = 20.0;

	//init
	if (initHuff_flag == 0) {
		Build_Huffman_Table(Standard_Luminance_DC_NRCodes, Standard_Luminance_DC_Values, HuffTbl_Y_DC);
		Build_Huffman_Table(Standard_Luminance_AC_NRCodes, Standard_Luminance_AC_Values, HuffTbl_Y_AC);
		Build_Huffman_Table(Standard_Chrominance_DC_NRCodes, Standard_Chrominance_DC_Values, HuffTbl_UV_DC);
		Build_Huffman_Table(Standard_Chrominance_AC_NRCodes, Standard_Chrominance_AC_Values, HuffTbl_UV_AC);
		initHuff_flag = 1;												//label the flag
	}

	//quality
	if (quality != init_quality_411) {									//Inverse-ZigZag Quantize Table to fit the quality
		for (int m = 0; m < 64; m++) {
			int aa = (int)((double)Q_Y[m] / quality + 0.5);
			if (aa <= 0)		aa = 1;
			if (aa > 0xFF)		aa = 0xFF;
			fit_Q_Y[RLE_ZigZag[m][0] * 8 + RLE_ZigZag[m][1]] = (u8)aa;	//Inv-ZigZag
			Q_Y_wr[m] = (u8)aa;

			int bb = (int)((double)Q_UV[m] / quality + 0.5);
			if (bb <= 0)		bb = 1;
			if (bb > 0xFF)		bb = 0xFF;
			fit_Q_UV[RLE_ZigZag[m][0] * 8 + RLE_ZigZag[m][1]] = (u8)bb;
			Q_UV_wr[m] = (u8)bb;
		}
		init_quality_411 = quality;										//label the flag
	}

	//revise
	Jheader.DQT1.dqtacc_id = 0x01;
	Jheader.imheight = height;
	Jheader.imwidth = width;
	Jheader.clrY_sample = 0x22;											//yuv411(420)
	Jheader.clrU_sample = 0x11;
	Jheader.clrV_sample = 0x11;
	Jheader.DHT_DC1.hufftype_id = 0x01;
	Jheader.DHT_AC1.hufftype_id = 0x11;

	//write header
	fwrite(BigEnd16(&Jheader.SOI),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.APP0),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.app0len),			sizeof(u16), 1, fp);
	fwrite(Jheader.app0id, 1, 5, fp);
	fwrite(BigEnd16(&Jheader.jfifver),			sizeof(u16), 1, fp);
	fwrite(&Jheader.xyunit, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.xden),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.yden),				sizeof(u16), 1, fp);
	fwrite(&Jheader.thumbh, 1, 1, fp);
	fwrite(&Jheader.thumbv, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.DQT0.label),		sizeof(u16), 1, fp);	//DQT_Lumi(Y)
	fwrite(BigEnd16(&Jheader.DQT0.dqtlen),		sizeof(u16), 1, fp);
	fwrite(&Jheader.DQT0.dqtacc_id, 1, 1, fp);
	fwrite(Q_Y_wr, 1, 64, fp);
	fwrite(BigEnd16(&Jheader.DQT1.label),		sizeof(u16), 1, fp);	//DQT_Chro(UV)
	fwrite(BigEnd16(&Jheader.DQT1.dqtlen),		sizeof(u16), 1, fp);
	fwrite(&Jheader.DQT1.dqtacc_id, 1, 1, fp);
	fwrite(Q_UV_wr, 1, 64, fp);
	fwrite(BigEnd16(&Jheader.SOF0),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.sof0len),			sizeof(u16), 1, fp);
	fwrite(&Jheader.sof0acc, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.imheight),			sizeof(u16), 1, fp);	//image_height
	fwrite(BigEnd16(&Jheader.imwidth),			sizeof(u16), 1, fp);	//image_width
	fwrite(&Jheader.clrcomponent, 1, 1, fp);
	fwrite(&Jheader.clrY_id, 1, 1, fp);
	fwrite(&Jheader.clrY_sample, 1, 1, fp);								//Y_sample_factor
	fwrite(&Jheader.clrY_QTable, 1, 1, fp);
	fwrite(&Jheader.clrU_id, 1, 1, fp);
	fwrite(&Jheader.clrU_sample, 1, 1, fp);								//U_sample_factor
	fwrite(&Jheader.clrU_QTable, 1, 1, fp);
	fwrite(&Jheader.clrV_id, 1, 1, fp);
	fwrite(&Jheader.clrV_sample, 1, 1, fp);								//V_sample_factor
	fwrite(&Jheader.clrV_QTable, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.DHT_DC0.label),	sizeof(u16), 1, fp);	//DHT_Lumi(Y)_DC
	fwrite(BigEnd16(&Jheader.DHT_DC0.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_DC0.hufftype_id, 1, 1, fp);
	fwrite(Standard_Luminance_DC_NRCodes, 1, 16, fp);
	fwrite(Standard_Luminance_DC_Values, 1, 12, fp);
	fwrite(BigEnd16(&Jheader.DHT_AC0.label),	sizeof(u16), 1, fp);	//DHT_Lumi(Y)_AC
	fwrite(BigEnd16(&Jheader.DHT_AC0.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_AC0.hufftype_id, 1, 1, fp);
	fwrite(Standard_Luminance_AC_NRCodes, 1, 16, fp);
	fwrite(Standard_Luminance_AC_Values, 1, 162, fp);
	fwrite(BigEnd16(&Jheader.DHT_DC1.label),	sizeof(u16), 1, fp);	//DHT_Chro(UV)_DC
	fwrite(BigEnd16(&Jheader.DHT_DC1.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_DC1.hufftype_id, 1, 1, fp);
	fwrite(Standard_Chrominance_DC_NRCodes, 1, 16, fp);
	fwrite(Standard_Chrominance_DC_Values, 1, 12, fp);
	fwrite(BigEnd16(&Jheader.DHT_AC1.label),	sizeof(u16), 1, fp);	//DHT_Chro(UV)_AC
	fwrite(BigEnd16(&Jheader.DHT_AC1.dhtlen),	sizeof(u16), 1, fp);
	fwrite(&Jheader.DHT_AC1.hufftype_id, 1, 1, fp);
	fwrite(Standard_Chrominance_AC_NRCodes, 1, 16, fp);
	fwrite(Standard_Chrominance_AC_Values, 1, 162, fp);
	fwrite(BigEnd16(&Jheader.SOS),				sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.soslen),			sizeof(u16), 1, fp);
	fwrite(&Jheader.component, 1, 1, fp);
	fwrite(BigEnd16(&Jheader.Y_id_dht),			sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.U_id_dht),			sizeof(u16), 1, fp);
	fwrite(BigEnd16(&Jheader.V_id_dht),			sizeof(u16), 1, fp);
	fwrite(&Jheader.SpectrumS, 1, 1, fp);
	fwrite(&Jheader.SpectrumE, 1, 1, fp);
	fwrite(&Jheader.SpectrumC, 1, 1, fp);


	int	newByte = 0, newBytePos = 7;
	int	prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;

	for (int yPos = 0; yPos < height; yPos += 16) {
		for (int xPos = 0; xPos < width; xPos += 16) {
			for (int u = 0; u < 4; u++)
			{
				double		MCU_Y[8][8] = { 0.0 };
				double		res_FDCT_Y[8][8] = { 0.0 };
				int			res_Quanti_Y[8][8] = { 0 };
				vector<int> vrle_Y;
				vector<int> vbitcode_Y;
				BitString	outBitStr_Y[200];
				int			BitStrCount_Y = 0;

				for (u8 i = 0; i < 8; i++) {
					for (u8 j = 0; j < 8; j++) {
						MCU_Y[i][j] = (double)yuv_Y[(yPos + i + (u / 2) * 8) * width + xPos + j + (u % 2) * 8] - 128.0;
					}
				}

				//Y-channel
				Forward_DCT(res_FDCT_Y, MCU_Y);
				Quantize_down_process(res_Quanti_Y, res_FDCT_Y, fit_Q_Y);
				RLE_process(&vrle_Y, res_Quanti_Y, &prev_DC_Y);
				Bit_Coding_process(&vrle_Y, &vbitcode_Y);
				BitStrCount_Y = Huffman_encoding_process(&vbitcode_Y, HuffTbl_Y_DC, HuffTbl_Y_AC, outBitStr_Y);
				write_BitString_Sequence(outBitStr_Y, BitStrCount_Y, &newByte, &newBytePos, fp);

			}

			double	MCU_U[8][8] = { 0.0 }, MCU_V[8][8] = { 0.0 };	//对yuv444的四个像素取色度平均得到的效果比只取左上角好很多
			for (u8 i = 0; i < 8; i++) {
				for (u8 j = 0; j < 8; j++) {
					MCU_U[i][j] += (double)yuv_U[(yPos + i * 2 + 0) * width + xPos + j * 2 + 0] - 128.0;
					MCU_U[i][j] += (double)yuv_U[(yPos + i * 2 + 0) * width + xPos + j * 2 + 1] - 128.0;
					MCU_U[i][j] += (double)yuv_U[(yPos + i * 2 + 1) * width + xPos + j * 2 + 0] - 128.0;
					MCU_U[i][j] += (double)yuv_U[(yPos + i * 2 + 1) * width + xPos + j * 2 + 1] - 128.0;
					MCU_U[i][j] /= 4.0;
					MCU_V[i][j] += (double)yuv_V[(yPos + i * 2 + 0) * width + xPos + j * 2 + 0] - 128.0;
					MCU_V[i][j] += (double)yuv_V[(yPos + i * 2 + 0) * width + xPos + j * 2 + 1] - 128.0;
					MCU_V[i][j] += (double)yuv_V[(yPos + i * 2 + 1) * width + xPos + j * 2 + 0] - 128.0;
					MCU_V[i][j] += (double)yuv_V[(yPos + i * 2 + 1) * width + xPos + j * 2 + 1] - 128.0;
					MCU_V[i][j] /= 4.0;
				}
			}

			double		res_FDCT_U[8][8] = { 0.0 }, res_FDCT_V[8][8] = { 0.0 };
			int			res_Quanti_U[8][8] = { 0 }, res_Quanti_V[8][8] = { 0 };
			vector<int> vrle_U, vrle_V;
			vector<int> vbitcode_U, vbitcode_V;
			BitString	outBitStr_U[200], outBitStr_V[200];
			int			BitStrCount_U = 0, BitStrCount_V = 0;

			//U-channel
			Forward_DCT(res_FDCT_U, MCU_U);
			Quantize_down_process(res_Quanti_U, res_FDCT_U, fit_Q_UV);
			RLE_process(&vrle_U, res_Quanti_U, &prev_DC_U);
			Bit_Coding_process(&vrle_U, &vbitcode_U);
			BitStrCount_U = Huffman_encoding_process(&vbitcode_U, HuffTbl_UV_DC, HuffTbl_UV_AC, outBitStr_U);
			write_BitString_Sequence(outBitStr_U, BitStrCount_U, &newByte, &newBytePos, fp);

			//V-channel
			Forward_DCT(res_FDCT_V, MCU_V);
			Quantize_down_process(res_Quanti_V, res_FDCT_V, fit_Q_UV);
			RLE_process(&vrle_V, res_Quanti_V, &prev_DC_V);
			Bit_Coding_process(&vrle_V, &vbitcode_V);
			BitStrCount_V = Huffman_encoding_process(&vbitcode_V, HuffTbl_UV_DC, HuffTbl_UV_AC, outBitStr_V);
			write_BitString_Sequence(outBitStr_V, BitStrCount_V, &newByte, &newBytePos, fp);
		}
	}

	if (newBytePos != 7) {
		newByte |= ~(0xFF << newBytePos);	//Huffman编码后剩余的bit应该用1去填充
		fwrite(&newByte, 1, 1, fp);			//防止JPEG的最后一点点bit流没有被写进文件
	}

	fwrite(BigEnd16(&Jheader.EOI), sizeof(u16), 1, fp);
}


