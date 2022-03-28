#include <cstdio>		//fread
#include <cstdlib>		//malloc, free

#include <iostream>
#include <vector>

#include "JPEG.h"

using namespace std;


//����:����DHT��ʱ��˳�������ͬ���ֳ����µ�(�������+1),�������볤��λ���ֵ�ӳ���len_tbl(ע�����max_in_ThisLen[17], len_tbl[17][130])
void Build_Huffman_Table(const u8* nr_codes, const u8* std_table, HuffType* huffman_table, u16* max_in_ThisLen, u8 huff_len_table[][130])
{
	unsigned char pos_in_table = 0;
	unsigned short code_value = 0;

	for (int k = 1; k <= 16; k++)
	{
		u8 num = 0;
		for (int j = 1; j <= nr_codes[k - 1]; j++)
		{
			huff_len_table[k][num] = std_table[pos_in_table];
			num++;
			huffman_table[std_table[pos_in_table]].code = code_value;
			huffman_table[std_table[pos_in_table]].len = k;
			pos_in_table++;
			code_value++;
		}
		max_in_ThisLen[k] = code_value;
		code_value <<= 1;
	}
	//jpeg��Huffman���벻�����1bit������,Ϊ�˲��ڶ�ȡ��һ����Ϊ1����Ϊ[1]=0���������Ϊ��һ��������ĳ������...
	//�������Ӧ�����۵�һ��������ɶ����Ϊ��ǰ���ش���max(��Ȼ[1]��������0),����ǿ��len=1ʱ��max����Ϊ0b
	max_in_ThisLen[0] = 0;		//���������ʵ������
	//max_in_ThisLen[1] = 0;


	//<ע��>
	//������Ϊ�Ϸ������ŵ������ų�����Ů�ֻ���ֽ(phonepaper.jpeg)����ʧ�ܣ�����ԭ������LumiAC��Huffman������len=1�����
	//��ʵ��ͼƬ���볤Ϊ1������ֻ��һ��,��0b,�볤Ϊ2������Ϊ��,�볤Ϊ3������Ϊ100b,101b,�볤Ϊ4��...
	//�������len=2û�й������Եõ����: max[1]=1b=1, max[2]=10b=2, max[3]=110b=6, max[4]=...
	//max[]�ǵ�����,��jpeg���������2bit����,����3bit�����ֱض�Ҳ����2bit��max,���max[2]�����㲢��Ӱ�������ȷ��
	//����,huffman����ʱ��bit����0���ж�����1bit���ȵ�����,������������Ҫע�͵�������max[2]=1
	//����������bit������bitΪ1ʱ,��Ϊ����restore_RLE()������>=���ж�max����,��˿��Ա�֤����׹��1bit���ֵ��ж���
}



//newBytePos��ʼ��Ϊ����,����Ϊ����ʱ˵�����뺯��ʱӦ�����¶�ȡ�µ�newByte
void rd_restore_HuffBit_RLE(vector<int>* pvrle, int* pnewByte, int* pnewBytePos, int* prev_DC, \
	const u16* max_first_DC, const u16* max_first_AC, u8 huff_lentbl_DC[][130], u8 huff_lentbl_AC[][130], \
	HuffType* rd_hufftbl_DC, HuffType* rd_hufftbl_AC, FILE* fp)
{
	//���ȳ��ֵ���RLE�����Huffman����,Ȼ��������ֵ��ֽڵ�֪�¸����ֵ��볤,����ȡ��������
	static unsigned short mmask[16] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768 };
	int value = 0, code_len = 0;
	BitString bitcode_1, bitcode_2;										//RLE����Զ��ĵ�һ��(�޸�ǰnrz/len�޸���nrz)�͵ڶ���(magnitude)

	//outer-func default newByte=0, newBytePos=-1
	while (value >= (int)max_first_DC[code_len])						//������ >=, ��Ϊmaxָʾ��ֵ�ǵ�ǰ�������+1,�����Ǻ������ֵ�ǰ׺
	{
		if (*pnewBytePos < 0) {
			if(*pnewByte == 0xFF)	fread(pnewByte, 1, 1, fp);			//0xFF֮������һ���������0x00(���Ե�)
			fread(pnewByte, 1, 1, fp);
			*pnewBytePos = 7;
		}
		value <<= 1;
		value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
		(*pnewBytePos)--;
		code_len++;
	}
	for (int p = 0; p < 130; p++) {										//DCӳ�����ʵ����̫��(ԶС��130)
		if (rd_hufftbl_DC[huff_lentbl_DC[code_len][p]].code == value) {	//���볤ӳ�����Ѱ����value����ϵ�����
			bitcode_1.code = huff_lentbl_DC[code_len][p];				//����û�д�����(��110b),����ֱ�Ӵ�(��0x5)
			bitcode_1.len = code_len;									//ע��DC���ֵ�nrz�϶���0,��ʡ��0x05��0x5
			pvrle->push_back(bitcode_1.code);							//DC��nrz�϶���0,����Ϊ�˼���AC���Ǳ�����0
			value = 0;
			code_len = 0;
			break;
		}
	}
	if (bitcode_1.code == 0) {
		bitcode_2.code = 0;
		bitcode_2.len = 0;
		pvrle->at(0) = 0;												//����һ���Bit�����޸�ΪRLE�����DC_nrz=0
		pvrle->push_back(bitcode_2.code + *prev_DC);					//��ֱ�����0ʱ��ʾ��ǰʵ��DCֵ����prev_DC
	}
	else {
		bitcode_2.len = bitcode_1.code & 0x0F;
		pvrle->at(0) = 0;												//�޸���һ��
		for (int p = 0; p < bitcode_2.len; p++) {						//��һ��ָ���˷������ֵ��볤
			if (*pnewBytePos < 0) {
				if (*pnewByte == 0xFF)	fread(pnewByte, 1, 1, fp);		//0xFF֮������һ���������0x00(���Ե�)
				fread(pnewByte, 1, 1, fp);
				*pnewBytePos = 7;
			}
			value <<= 1;
			//value |= (*newByte & mmask[*newBytePos]);
			value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
			(*pnewBytePos)--;
			code_len++;
			if ((p == 0) && (value == 0))
				bitcode_2.code = -1;									//��ʱ��code�ø�,����ָʾ�׸�bit����1(�ڷ��ȱ����д�����)
		}
		if (bitcode_2.code < 0) {
			int tt = (-1) * (1 << bitcode_2.len) + 1;
			bitcode_2.code = value + tt + *prev_DC;
			pvrle->push_back(bitcode_2.code);
		}
		else {
			bitcode_2.code = value + *prev_DC;
			pvrle->push_back(bitcode_2.code);
		}
		*prev_DC = bitcode_2.code;										//����prev_DC
		value = 0;
		code_len = 0;
		bitcode_1.code = 0;	bitcode_1.len = 0;
		bitcode_2.code = 0;	bitcode_2.len = 0;
	}

	//AC part
	int nums = 0;
	//��¼RLE�����ԭʼ������AC���ֵĳ���(63),ע������ĸ���Ҳ���¼...
	//(��Ȼ�����޷�������63,�����β������0ʱҲ�ǲ�����EOB��)
	for (nums = 0; nums < 63; nums++)
	{
		while (value >= (int)max_first_AC[code_len])
		{
			if (*pnewBytePos < 0) {
				if (*pnewByte == 0xFF)	fread(pnewByte, 1, 1, fp);		//0xFF֮������һ���������0x00(���Ե�)
				fread(pnewByte, 1, 1, fp);
				*pnewBytePos = 7;
			}
			value <<= 1;
			value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
			(*pnewBytePos)--;
			code_len++;
		}
		for (int p = 0; p < 130; p++) {
			if (rd_hufftbl_AC[huff_lentbl_AC[code_len][p]].code == value) {	//���볤ӳ�����Ѱ����value����ϵ�����
				bitcode_1.code = huff_lentbl_AC[code_len][p];			//������ֱ�Ӵ�����ӳ��(Ҳ�������ֶ�Ӧ��ֵ)
				bitcode_1.len = code_len;
				pvrle->push_back(bitcode_1.code);
				value = 0;
				code_len = 0;
				break;
			}
		}
		if (bitcode_1.code == 0xF0) {									//����AC���ֵ�(15,0)
			bitcode_2.code = 0;
			bitcode_2.len = 0;
			pvrle->back() = 15;
			pvrle->push_back(bitcode_2.code);
			nums += 15;
		}
		else if (bitcode_1.code == 0x00) {								//EOB:����AC���ֵ�(0,0)
			bitcode_2.code = 0;
			bitcode_2.len = 0;
			pvrle->back() = 0;
			pvrle->push_back(bitcode_2.code);
			nums = 63;													//auto-break the loop
			//break;
		}
		else {
			bitcode_2.len = bitcode_1.code & 0x0F;
			pvrle->back() >>= 4;										//����һ���Bit����(��0x25)�޸�ΪRLE����(��0x02)
			nums += pvrle->back();
			for (int p = 0; p < bitcode_2.len; p++) {					//��һ��ָ���˷������ֵ��볤
				if (*pnewBytePos < 0) {
					if (*pnewByte == 0xFF)	fread(pnewByte, 1, 1, fp);	//0xFF֮������һ���������0x00(���Ե�)
					fread(pnewByte, 1, 1, fp);
					*pnewBytePos = 7;
				}
				value <<= 1;
				value = value + ((*pnewByte & mmask[*pnewBytePos]) ? 1 : 0);
				(*pnewBytePos)--;
				code_len++;
				if ((p == 0) && (value == 0))
					bitcode_2.code = -1;								//��ʱ��code�ø�,����ָʾ�׸�bit����1(�ڷ��ȱ����д�����)
			}
			if (bitcode_2.code < 0) {
				int tt = (-1) * (1 << bitcode_2.len) + 1;
				bitcode_2.code = value + tt;
				pvrle->push_back(bitcode_2.code);
				value = 0;
				code_len = 0;
			}
			else {
				bitcode_2.code = value;
				pvrle->push_back(bitcode_2.code);
				value = 0;
				code_len = 0;
			}
		}
		bitcode_1.code = 0;	bitcode_1.len = 0;
		bitcode_2.code = 0;	bitcode_2.len = 0;
	}
}



//���ú���֮ǰ�Ѿ�����ֽ����,�����ڲ�����������ZigZag����
void rd_restore_InvQuantize(double afterInvQ[][8], vector<int>* pvrle, const u8* Qm)
{
	static int	temp[8][8] = { 0 };	//�����staticʡ�Իᱨstack corrupted�Ĵ���,�����(Ҳ��ʱ��VS2019�Ĵ����Ż�����)
	int i = 0, j = 1;
	temp[0][0] = pvrle->at(1);											//true DC
	for (size_t v = 2; v < pvrle->size(); v+=2) {
		if (pvrle->at(v) == 0 && pvrle->at(v + 1) == 0) {				//EOB
			while (i <= 7 && j <= 7) {
				temp[i][j] = 0;
				j++;
				if (j >= 8) { i++; j = 0; }
			}
			//break;
		}
		else {
			for (int z = 0; z < pvrle->at(v); z++) {					//nrz
				temp[i][j] = 0;
				j++;
				if (j >= 8) { i++; j = 0; }
			}
			temp[i][j] = pvrle->at(v + 1);								//non-zero value
			j++;
			if (j >= 8) { i++; j = 0; }
		}
	}
	//inv-Quantize
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			temp[i][j] = temp[i][j] * Qm[i * 8 + j];
		}
	}
	//inv-ZigZag
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			afterInvQ[RLE_ZigZag[i * 8 + j][0]][RLE_ZigZag[i * 8 + j][1]] = (double)temp[i][j];
		}
	}
}
 


//2ByteС��ת��Ϊ���
inline  u16 rd_BigEnd16(u16 val)
{
	u8	b1 = (u8)(val);
	u8	b2 = (u8)(val >> 8);
	u16 temp = (u16)b2 + ((u16)b1 << 8);
	return temp;
}


//������ȫ��Ԫ�ؽ������,������DC/AC_NRCodes�����sum��ȷ��DC/AC_Values����ĳ���
static u16 sum_arr(u8* arr, u8 len_arr)
{
	u16 sum = 0;
	for (u8 i = 0; i < len_arr; i++)
		sum += arr[i];
	return sum;
}




//Ϊ�˷������8/16pixel��MCU��Ԥ��ȡSOF0��clrY_sample�����ж�jpeg�Ĳ�������
//��������Ԥ��ȡ Y_sample/height/width
void pre_rd_SOF0_info(FILE* fp, u8* pY_sample, u16* pimw, u16* pimh)
{
	u16 seg_label_t = 0;
	u16 seg_size_t = 0;
	u8  dump = 0;					//ռλ�ֽ�,���ڱ��治��ܵ��ֽ�

	fread(&seg_label_t, 2, 1, fp);	//SOI
	fread(&seg_label_t, 2, 1, fp);
	cout << ">[find]whether this jpeg has valid SOF0 information or not ..." << endl;
	while (seg_label_t != 0xC0FF)
	{
		if (seg_label_t == 0x01FF)	//TEM
			fread(&seg_label_t, 2, 1, fp);
		else {
			fread(&seg_size_t, 2, 1, fp);
			fseek(fp, rd_BigEnd16(seg_size_t) - 2, SEEK_CUR);
			fread(&seg_label_t, 2, 1, fp);
		}
	}
	cout << "<      valid SOF0 label has been found." << endl;
	fread(&seg_size_t, 2, 1, fp);	//seg_len
	fread(&dump, 1, 1, fp);			//acc
	fread(pimh, 2, 1, fp);
	*pimh = rd_BigEnd16(*pimh);		//imheight
	fread(pimw, 2, 1, fp);
	*pimw = rd_BigEnd16(*pimw);		//imwidth
	fread(&dump, 1, 1, fp);			//color_component
	fread(&dump, 1, 1, fp);			//clrY_id = 0x01
	fread(pY_sample, 1, 1, fp);		//0x11(444) or 0x22(420)

	fseek(fp, 0, SEEK_SET);			//rewind to the start of the file
	return;
}




//Ԥ��ȡDHT�εĲ�����Ϣ���õ�Huffman���Ļ�����Ϣ�������������ʽ��ȡ
//��ʵ����Ԥ��ȡ4��Values�ĳ��ȣ����㺯�����ⲿ����malloc
void pre_rd_DHT_info(FILE* fp, u16* pLumi_DC_Values_len, u16* pLumi_AC_Values_len, u16* pChromi_DC_Values_len, u16* pChromi_AC_Values_len)
{
	u16 seg_label_t = 0;
	u16 seg_size_t = 0;

	fread(&seg_label_t, 2, 1, fp);	//SOI
	fread(&seg_label_t, 2, 1, fp);
	cout << ">[find]whether this jpeg has valid DHT information or not ..." << endl;
	while (seg_label_t != 0xC4FF)
	{
		if (seg_label_t == 0x01FF)	//TEM
			fread(&seg_label_t, 2, 1, fp);
		else {
			fread(&seg_size_t, 2, 1, fp);
			fseek(fp, rd_BigEnd16(seg_size_t) - 2, SEEK_CUR);
			fread(&seg_label_t, 2, 1, fp);
		}
	}
	cout << "<      valid DHT label has been found." << endl;
	fread(&seg_size_t, 2, 1, fp);
	seg_size_t = rd_BigEnd16(seg_size_t);
	if(seg_size_t >= 0x00E0)			//���DHT�ε���������0xE0����ʱ��Ϊ���ǵ���label����4��������
	{
		for (u8 i = 0; i < 4; i++)
		{
			u8 dht_id_t = 0;
			u8 nrcodes[16] = { 0 };
			fread(&dht_id_t, 1, 1, fp);
			if (dht_id_t == 0x00) {						//Lumi DC
				fread(nrcodes, 1, 16, fp);
				*pLumi_DC_Values_len = sum_arr(nrcodes, 16);
				fseek(fp, *pLumi_DC_Values_len, SEEK_CUR);
			}
			else if (dht_id_t == 0x01) {				//Chromi DC
				fread(nrcodes, 1, 16, fp);
				*pChromi_DC_Values_len = sum_arr(nrcodes, 16);
				fseek(fp, *pChromi_DC_Values_len, SEEK_CUR);
			}
			else if (dht_id_t == 0x10) {				//Lumi AC
				fread(nrcodes, 1, 16, fp);
				*pLumi_AC_Values_len = sum_arr(nrcodes, 16);
				fseek(fp, *pLumi_AC_Values_len, SEEK_CUR);
			}
			else if (dht_id_t == 0x11) {				//Chromi AC
				fread(nrcodes, 1, 16, fp);
				*pChromi_AC_Values_len = sum_arr(nrcodes, 16);
				fseek(fp, *pChromi_AC_Values_len, SEEK_CUR);
			}
			else;
		}
	}
	else								//����ƽ��״��: ÿ��label����һ����(��4���ظ�DHT��)
	{
		fseek(fp, -4, SEEK_CUR);		//rewind 2+2 Byte
		for (int t = 0; t < 4; t++)
		{
			u8  dht_id_t = 0;
			u8  nrcodes[16] = { 0 };
			fread(&seg_label_t, 2, 1, fp);
			fread(&seg_size_t, 2, 1, fp);
			seg_size_t = rd_BigEnd16(seg_size_t);
			fread(&dht_id_t, 1, 1, fp);
			if (dht_id_t == 0x00) {
				fread(nrcodes, 1, 16, fp);
				*pLumi_DC_Values_len = seg_size_t - 2 - 1 - 16;
				fseek(fp, *pLumi_DC_Values_len, SEEK_CUR);
			}
			else if (dht_id_t == 0x10) {
				fread(nrcodes, 1, 16, fp);
				*pLumi_AC_Values_len = seg_size_t - 2 - 1 - 16;
				fseek(fp, *pLumi_AC_Values_len, SEEK_CUR);
			}
			else if (dht_id_t == 0x01) {
				fread(nrcodes, 1, 16, fp);
				*pChromi_DC_Values_len = seg_size_t - 2 - 1 - 16;
				fseek(fp, *pChromi_DC_Values_len, SEEK_CUR);
			}
			else if (dht_id_t == 0x11) {
				fread(nrcodes, 1, 16, fp);
				*pChromi_AC_Values_len = seg_size_t - 2 - 1 - 16;
				fseek(fp, *pChromi_AC_Values_len, SEEK_CUR);
			}
			else;
		}
	}

	fseek(fp, 0, SEEK_SET);				//rewind to the start of the file
	return;
}





/*
 * intro:	jpeg����
 * param:	if <bool> toYUV420 == false
 *				�����yuv_Y,yuv_U,yuv_V������yuv444��ʽ������,��YUV��������Ӧ��ͬ�ȴ�С
 *			else
 *				�����yuv_Y,yuv_U,yuv_V������yuv420��ʽ��С
 * note:	������û��ʹ��fclose()���ر��ļ�
 *			��������ֻ�ܽ���444��420�����ʵ�JPEGͼƬ��YUV������422�����ʴ�����ǿյ�
 *			ע��jpeg����ʱӦ�ø����ļ����ɶ�Ӧ��DQT��DHT, Ϊ�˼�����Ҳ������ʹ�ñ�׼��
 *			ע��jpeg�ļ��г��˶�ͷ֮������������������0xFF�󶼻����һ���������0x00
 *			ע��jpeg�ļ������ںܶ����ڰ��������Ϣ�Ķ�, ����jpeg�����̶�ͷ���ֵ����ɸ�0xFF(��������֧�ִ���Ҫ��)
 */
void Dec_JPEG_to_YUV(FILE* fp, u8* yuv_Y, u8* yuv_U, u8* yuv_V, bool toYUV420 = true)
{
	struct s_JPEG_header Jheader;
	u16 Lumi_DC_Values_len = 0, Lumi_AC_Values_len = 0, Chromi_DC_Values_len = 0, Chromi_AC_Values_len = 0;
	pre_rd_DHT_info(fp, &Lumi_DC_Values_len, &Lumi_AC_Values_len, &Chromi_DC_Values_len, &Chromi_AC_Values_len);
	if (Lumi_DC_Values_len == 0 || Lumi_AC_Values_len == 0 || Chromi_DC_Values_len == 0 || Chromi_AC_Values_len == 0) {
		cout << "[ERROR]DHT len error." << endl;
		return;
	}
	cout << " [info]DHT pre_read is successful:" << endl;
	cout << "       LumiDC(0x00).size = " << Lumi_DC_Values_len + 19 << " \t\tLumiAC(0x10).size = " << Lumi_AC_Values_len + 19 << endl;
	cout << "       ChromiDC(0x01).size = " << Chromi_DC_Values_len + 19 << " \tChromiAC(0x11).size = " << Chromi_AC_Values_len + 19 << endl;
	u8	rd_Lumi_DC_NRCodes[16] = { 0 }, rd_Lumi_AC_NRCodes[16] = { 0 };
	u8	rd_Chromi_DC_NRCodes[16] = { 0 }, rd_Chromi_AC_NRCodes[16] = { 0 };
	u8* rd_Lumi_DC_Values = (u8*)malloc(sizeof(u8) * Lumi_DC_Values_len);
	u8* rd_Chromi_DC_Values = (u8*)malloc(sizeof(u8) * Chromi_DC_Values_len);
	u8* rd_Lumi_AC_Values = (u8*)malloc(sizeof(u8) * Lumi_AC_Values_len);
	u8* rd_Chromi_AC_Values = (u8*)malloc(sizeof(u8) * Chromi_AC_Values_len);
	if (rd_Lumi_DC_Values == NULL || rd_Chromi_DC_Values == NULL || rd_Lumi_AC_Values == NULL || rd_Chromi_AC_Values == NULL)
		return;

	u16 height = 0, width = 0;
	u16 seg_label_t = 0;
	u16 seg_size_t = 0;
	u16 res_sync = 0;

	fread(&Jheader.SOI, 2, 1, fp);
	Jheader.SOI = rd_BigEnd16(Jheader.SOI);

	fread(&seg_label_t, 2, 1, fp);
	while (seg_label_t != 0xDAFF)
	{
		switch (seg_label_t)
		{
		case 0xE0FF:							// APP0/JFIF (�������APPn)
			fread(&seg_size_t, 2, 1, fp);
			Jheader.APP0 = rd_BigEnd16(seg_label_t);			Jheader.app0len = rd_BigEnd16(seg_size_t);
			fread(Jheader.app0id, 1, 5, fp);
			fread(&Jheader.jfifver, 2, 1, fp);					Jheader.jfifver = rd_BigEnd16(Jheader.jfifver);
			fread(&Jheader.xyunit, 1, 1, fp);
			fread(&Jheader.xden, 2, 1, fp);						Jheader.xden = rd_BigEnd16(Jheader.xden);
			fread(&Jheader.yden, 2, 1, fp);						Jheader.yden = rd_BigEnd16(Jheader.yden);
			fread(&Jheader.thumbh, 1, 1, fp);
			fread(&Jheader.thumbv, 1, 1, fp);
			break;

		case 0xE1FF:							// APP1/Exif (���������Ϣ��)
			fread(&seg_size_t, 2, 1, fp);
			fseek(fp, rd_BigEnd16(seg_size_t) - 2, SEEK_CUR);
			break;

		case 0xDBFF:							// DQT
			fread(&seg_size_t, 2, 1, fp);
			if (seg_size_t == 0x8400)			//0x84 = 132 = 2+(1+64)*2	->	����label��������DQT�����
			{
				for (int t = 0; t < 2; t++)
				{
					u8	dqt_id_t = 0;
					fread(&dqt_id_t, 1, 1, fp);
					if (dqt_id_t == 0) {
						Jheader.DQT0.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT0.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT0.dqtacc_id = dqt_id_t;
						fread(Jheader.DQT0.QTable, 1, 64, fp);
					}
					else if (dqt_id_t == 1) {
						Jheader.DQT1.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT1.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT1.dqtacc_id = dqt_id_t;
						fread(Jheader.DQT1.QTable, 1, 64, fp);
					}
					else;
				}
			}
			else if (seg_size_t == 0x4300)		//0x43 = 67 = 2+1+64		->	ÿ��label������һ��DQT�����(������)
			{
				fseek(fp, -4, SEEK_CUR);		//rewind 2+2 Byte
				for (int t = 0; t < 2; t++)
				{
					u8	dqt_id_t = 0;
					fread(&seg_label_t, 2, 1, fp);
					fread(&seg_size_t, 2, 1, fp);
					fread(&dqt_id_t, 1, 1, fp);
					if (dqt_id_t == 0) {
						Jheader.DQT0.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT0.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT0.dqtacc_id = dqt_id_t;
						fread(Jheader.DQT0.QTable, 1, 64, fp);
					}
					else if (dqt_id_t == 1) {
						Jheader.DQT1.label = rd_BigEnd16(seg_label_t);
						Jheader.DQT1.dqtlen = rd_BigEnd16(seg_size_t);
						Jheader.DQT1.dqtacc_id = dqt_id_t;
						fread(Jheader.DQT1.QTable, 1, 64, fp);
					}
					else;
				}
			}
			else;
			break;

		case 0xC0FF:							// SOF0
			fread(&seg_size_t, 2, 1, fp);
			Jheader.SOF0 = rd_BigEnd16(seg_label_t);			Jheader.sof0len = rd_BigEnd16(seg_size_t);
			fread(&Jheader.sof0acc, 1, 1, fp);
			fread(&Jheader.imheight, 2, 1, fp);					Jheader.imheight = rd_BigEnd16(Jheader.imheight);	//image height/pixel
			fread(&Jheader.imwidth, 2, 1, fp);					Jheader.imwidth = rd_BigEnd16(Jheader.imwidth);		//image width/pixel
			width = Jheader.imwidth;
			height = Jheader.imheight;
			fread(&Jheader.clrcomponent, 1, 1, fp);
			for (int t = 0; t < Jheader.clrcomponent; t++) {	//clr_c=3:YCbCr, =4:CMYK, ����JFIFֻ֧��YCbCr(YUV)
				u8	clr_id_t = 0, clr_hvsample_t = 0, clr_Qid_t = 0;
				fread(&clr_id_t, 1, 1, fp);
				fread(&clr_hvsample_t, 1, 1, fp);
				fread(&clr_Qid_t, 1, 1, fp);
				if (clr_id_t == 0x01) {
					Jheader.clrY_id = clr_id_t;
					Jheader.clrY_sample = clr_hvsample_t;
					Jheader.clrY_QTable = clr_Qid_t;
				}
				else if (clr_id_t == 0x02) {
					Jheader.clrU_id = clr_id_t;
					Jheader.clrU_sample = clr_hvsample_t;
					Jheader.clrU_QTable = clr_Qid_t;
				}
				else if (clr_id_t == 0x03) {
					Jheader.clrV_id = clr_id_t;
					Jheader.clrV_sample = clr_hvsample_t;
					Jheader.clrV_QTable = clr_Qid_t;
				}
				else;
			}
			break;

		case 0xC4FF:							// DHT
			fread(&seg_size_t, 2, 1, fp);
			seg_size_t = rd_BigEnd16(seg_size_t);

			//���DHT�ε���������0xE0����ʱ��Ϊ���ǵ���label����4��������
			//���������¶��Ƕ������ĸ���С�Ķ�����DHT��: ÿ��label����һ����(��4���ظ�DHT��)
			if (seg_size_t < 0x00E0)
				fseek(fp, -4, SEEK_CUR);			//rewind 2+2 Byte
			for (u8 t = 0; t < 4; t++)
			{
				u8 dht_id_t = 0;
				if (seg_size_t < 0x00E0) {
					fread(&seg_label_t, 2, 1, fp);
					fread(&seg_size_t, 2, 1, fp);
					seg_size_t = rd_BigEnd16(seg_size_t);
				}
				fread(&dht_id_t, 1, 1, fp);
				if (dht_id_t == 0x00) {			//Lumi DC
					Jheader.DHT_DC0.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_DC0.hufftype_id = dht_id_t;
					fread(rd_Lumi_DC_NRCodes, 1, 16, fp);
					Jheader.DHT_DC0.dhtlen = 16 + Lumi_DC_Values_len + 1 + 2;	//codes + values + id + self_seg_len
					fread(rd_Lumi_DC_Values, 1, Lumi_DC_Values_len, fp);
				}
				else if (dht_id_t == 0x01) {	//Chromi DC
					Jheader.DHT_DC1.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_DC1.hufftype_id = dht_id_t;
					fread(rd_Chromi_DC_NRCodes, 1, 16, fp);
					Jheader.DHT_DC1.dhtlen = 16 + Chromi_DC_Values_len + 1 + 2;	//codes + values + id + self_seg_len
					fread(rd_Chromi_DC_Values, 1, Chromi_DC_Values_len, fp);
				}
				else if (dht_id_t == 0x10) {	//Lumi AC
					Jheader.DHT_AC0.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_AC0.hufftype_id = dht_id_t;
					fread(rd_Lumi_AC_NRCodes, 1, 16, fp);
					Jheader.DHT_AC0.dhtlen = 16 + Lumi_AC_Values_len + 1 + 2;	//codes + values + id + self_seg_len
					fread(rd_Lumi_AC_Values, 1, Lumi_AC_Values_len, fp);
				}
				else if (dht_id_t == 0x11) {	//Chromi AC
					Jheader.DHT_AC1.label = rd_BigEnd16(seg_label_t);
					Jheader.DHT_AC1.hufftype_id = dht_id_t;
					fread(rd_Chromi_AC_NRCodes, 1, 16, fp);
					Jheader.DHT_AC1.dhtlen = 16 + Chromi_AC_Values_len + 1 + 2;	//codes + values + id + self_seg_len
					fread(rd_Chromi_AC_Values, 1, Chromi_AC_Values_len, fp);
				}
				else;
			}
			break;

		case 0xDDFF:							// DRI
			fread(&seg_size_t, 2, 1, fp);
			fread(&res_sync, 2, 1, fp);
			res_sync = rd_BigEnd16(res_sync);
			break;

		case 0x01FF:							// TEM (label only)
			break;

		default:								// PhotoShopͼƬ��APP13/14, ICCɫ�����˵�APP2, ʣ���APPn �� �����ı�ʶ����ֱ������
			fread(&seg_size_t, 2, 1, fp);
			fseek(fp, rd_BigEnd16(seg_size_t) - 2, SEEK_CUR);
			break;
		}

		fread(&seg_label_t, 2, 1, fp);
	}

	//JPEG�̶�SOS�εĺ������ͼ���ѹ������
	//��SOS��֮ǰ�Ȱ�ǰ���DHT�����Huffman���Huffman�볤���ұ�
	//���µı���Ϊ��ֹ����ռ��̫���ջ�Ѿ�����JPEG.h�ļ���(ȫ��)
	//u16	max_first_Lumi_DC[17] = { 0 }, max_first_Lumi_AC[17] = { 0 }, max_first_Chromi_DC[17] = { 0 }, max_first_Chromi_AC[17] = { 0 };
	//u8	HuffTbl_len_Y_DC[17][130] = { 0 }, HuffTbl_len_Y_AC[17][130] = { 0 }, HuffTbl_len_UV_DC[17][130] = { 0 }, HuffTbl_len_UV_AC[17][130] = { 0 };
	HuffType* rd_HuffTbl_Y_DC = (HuffType*)malloc(sizeof(HuffType) * Jheader.DHT_DC0.dhtlen - 2 - 1 - 16);
	HuffType* rd_HuffTbl_Y_AC = (HuffType*)malloc(sizeof(HuffType) * 256);
	HuffType* rd_HuffTbl_UV_DC = (HuffType*)malloc(sizeof(HuffType) * Jheader.DHT_DC1.dhtlen - 2 - 1 - 16);
	HuffType* rd_HuffTbl_UV_AC = (HuffType*)malloc(sizeof(HuffType) * 256);
	if (rd_HuffTbl_Y_DC == NULL || rd_HuffTbl_Y_AC == NULL || rd_HuffTbl_UV_DC == NULL || rd_HuffTbl_UV_AC == NULL)
		return;
	Build_Huffman_Table(rd_Lumi_DC_NRCodes, rd_Lumi_DC_Values, rd_HuffTbl_Y_DC, max_first_Lumi_DC, HuffTbl_len_Y_DC);
	Build_Huffman_Table(rd_Lumi_AC_NRCodes, rd_Lumi_AC_Values, rd_HuffTbl_Y_AC, max_first_Lumi_AC, HuffTbl_len_Y_AC);
	Build_Huffman_Table(rd_Chromi_DC_NRCodes, rd_Chromi_DC_Values, rd_HuffTbl_UV_DC, max_first_Chromi_DC, HuffTbl_len_UV_DC);
	Build_Huffman_Table(rd_Chromi_AC_NRCodes, rd_Chromi_AC_Values, rd_HuffTbl_UV_AC, max_first_Chromi_AC, HuffTbl_len_UV_AC);

	// SOS
	fread(&seg_size_t, 2, 1, fp);
	Jheader.SOS = rd_BigEnd16(seg_label_t);					Jheader.soslen = rd_BigEnd16(seg_size_t);
	fread(&Jheader.component, 1, 1, fp);
	fread(&Jheader.Y_id_dht, 2, 1, fp);						Jheader.Y_id_dht = rd_BigEnd16(Jheader.Y_id_dht);
	fread(&Jheader.U_id_dht, 2, 1, fp);						Jheader.U_id_dht = rd_BigEnd16(Jheader.U_id_dht);
	fread(&Jheader.V_id_dht, 2, 1, fp);						Jheader.V_id_dht = rd_BigEnd16(Jheader.V_id_dht);
	fread(&Jheader.SpectrumS, 1, 1, fp);		//the following three items are fixed
	fread(&Jheader.SpectrumE, 1, 1, fp);
	fread(&Jheader.SpectrumC, 1, 1, fp);

	// ��ȡԭʼѹ������
	int newByte = 0, newBytePos = -1;
	int	prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;
	if (Jheader.clrY_sample == 0x11)						//jpeg in yuv444
	{
		u16 res_sync_count = 0;	//DRI����Ҫ�ĸ�λ������
		for (int yPos = 0; yPos < height; yPos += 8) {
			for (int xPos = 0; xPos < width; xPos += 8) {
				res_sync_count++;
				if (res_sync != 0 && res_sync_count > res_sync) {	//DRI��λ��ֱ���͸�λHuffman����
					newByte = 0, newBytePos = -1;
					prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;
					fseek(fp, 2, SEEK_CUR);							//����RSTm��ǩ0xFFDm
					res_sync_count = 1;
				}
				int			MCU_Y[8][8] = { 0 };
				double		res_IDCT_Y[8][8] = { 0.0 };
				double		res_afterInvQ_Y[8][8] = { 0.0 };
				vector<int> vrle_Y;

				//Y-channel
				rd_restore_HuffBit_RLE(&vrle_Y, &newByte, &newBytePos, &prev_DC_Y, max_first_Lumi_DC, max_first_Lumi_AC, \
					HuffTbl_len_Y_DC, HuffTbl_len_Y_AC, rd_HuffTbl_Y_DC, rd_HuffTbl_Y_AC, fp);
				rd_restore_InvQuantize(res_afterInvQ_Y, &vrle_Y, Jheader.DQT0.QTable);
				Inverse_DCT(res_IDCT_Y, res_afterInvQ_Y);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_Y[i][j] = round_double(res_IDCT_Y[i][j] + 128.0);
						if (MCU_Y[i][j] < 0x00)		MCU_Y[i][j] = 0x00;
						if (MCU_Y[i][j] > 0xFF)		MCU_Y[i][j] = 0xFF;
					}
				}

				int			MCU_U[8][8] = { 0 }, MCU_V[8][8] = { 0 };
				double		res_IDCT_U[8][8] = { 0.0 }, res_IDCT_V[8][8] = { 0.0 };
				double		res_afterInvQ_U[8][8] = { 0.0 }, res_afterInvQ_V[8][8] = { 0.0 };
				vector<int> vrle_U, vrle_V;

				//U-channel
				rd_restore_HuffBit_RLE(&vrle_U, &newByte, &newBytePos, &prev_DC_U, max_first_Chromi_DC, max_first_Chromi_AC, \
					HuffTbl_len_UV_DC, HuffTbl_len_UV_AC, rd_HuffTbl_UV_DC, rd_HuffTbl_UV_AC, fp);
				rd_restore_InvQuantize(res_afterInvQ_U, &vrle_U, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_U, res_afterInvQ_U);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_U[i][j] = round_double(res_IDCT_U[i][j] + 128.0);
						if (MCU_U[i][j] < 0x00)		MCU_U[i][j] = 0x00;
						if (MCU_U[i][j] > 0xFF)		MCU_U[i][j] = 0xFF;
					}
				}

				//V-channel
				rd_restore_HuffBit_RLE(&vrle_V, &newByte, &newBytePos, &prev_DC_V, max_first_Chromi_DC, max_first_Chromi_AC, \
					HuffTbl_len_UV_DC, HuffTbl_len_UV_AC, rd_HuffTbl_UV_DC, rd_HuffTbl_UV_AC, fp);
				rd_restore_InvQuantize(res_afterInvQ_V, &vrle_V, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_V, res_afterInvQ_V);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_V[i][j] = round_double(res_IDCT_V[i][j] + 128.0);
						if (MCU_V[i][j] < 0x00)		MCU_V[i][j] = 0x00;
						if (MCU_V[i][j] > 0xFF)		MCU_V[i][j] = 0xFF;
					}
				}

				for (u8 i = 0; i < 8; i++) {	//write Y to YUV
					if (yPos + i >= height)							//ͼƬYƽ����²��Ե
						break;
					for (u8 j = 0; j < 8; j++) {
						if (xPos + j >= width)						//ͼƬYƽ����Ҳ��Ե
							break;//continue;
						yuv_Y[(yPos + i) * width + xPos + j] = MCU_Y[i][j];
					}
				} 
				if (toYUV420 == true) {		//(jpeg444)write UV to YUV_420
					for (u8 i = 0; i < 4; i++) {
						if (yPos / 2 + i >= (height + 1) / 2)		//ͼƬUV420ƽ����²��Ե(�ѿ��������߶ȵ�ͼƬ)
							break;
						for (u8 j = 0; j < 4; j++) {
							if (xPos / 2 + j >= (width + 1) / 2)	//ͼƬUV420ƽ����Ҳ��Ե(�ѿ���������ȵ�ͼƬ)
								break;//continue;
							double U_t = 0.0, V_t = 0.0;
							U_t += MCU_U[i * 2 + 0][j * 2 + 0];
							U_t += MCU_U[i * 2 + 0][j * 2 + 1];
							U_t += MCU_U[i * 2 + 1][j * 2 + 0];
							U_t += MCU_U[i * 2 + 1][j * 2 + 1];
							yuv_U[(yPos / 2 + i) * ((width + 1) / 2) + xPos / 2 + j] = (u8)(U_t / 4.0 + 0.5);	//U: width/2

							V_t += MCU_V[i * 2 + 0][j * 2 + 0];
							V_t += MCU_V[i * 2 + 0][j * 2 + 1];
							V_t += MCU_V[i * 2 + 1][j * 2 + 0];
							V_t += MCU_V[i * 2 + 1][j * 2 + 1];
							yuv_V[(yPos / 2 + i) * ((width + 1) / 2) + xPos / 2 + j] = (u8)(V_t / 4.0 + 0.5);	//V: width/2
						}
					}
				}
				else {							//(jpeg444)write UV to YUV_444
					for (u8 i = 0; i < 8; i++) {
						if (yPos + i >= height)						//ͼƬUVƽ����²��Ե
							break;
						for (u8 j = 0; j < 8; j++) {
							if (xPos + j >= width)					//ͼƬUVƽ����Ҳ��Ե
								break;//continue;
							yuv_U[(yPos + i) * width + xPos + j] = MCU_U[i][j];
							yuv_V[(yPos + i) * width + xPos + j] = MCU_V[i][j];
						}
					}
				}
			}
		}
	}
	else if (Jheader.clrY_sample == 0x22)					//yuv411(e.g. yuv420p = l420)
	{
		u16 res_sync_count = 0;	//DRI����Ҫ�ĸ�λ������
		for (int yPos = 0; yPos < height; yPos += 16) {
			for (int xPos = 0; xPos < width; xPos += 16) {
				for (int u = 0; u < 4; u++)
				{
					res_sync_count++;
					if (res_sync != 0 && res_sync_count > res_sync) {	//DRI��λ��ֱ���͸�λHuffman����
						newByte = 0, newBytePos = -1;
						prev_DC_Y = 0, prev_DC_U = 0, prev_DC_V = 0;
						fseek(fp, 2, SEEK_CUR);							//����RSTm��ǩ0xFFDm
						res_sync_count = 1;
					}
					int			MCU_Y[8][8] = { 0 };
					double		res_IDCT_Y[8][8] = { 0.0 };
					double		res_afterInvQ_Y[8][8] = { 0.0 };
					vector<int> vrle_Y;

					//Y-channel
					rd_restore_HuffBit_RLE(&vrle_Y, &newByte, &newBytePos, &prev_DC_Y, max_first_Lumi_DC, max_first_Lumi_AC, \
						HuffTbl_len_Y_DC, HuffTbl_len_Y_AC, rd_HuffTbl_Y_DC, rd_HuffTbl_Y_AC, fp);
					rd_restore_InvQuantize(res_afterInvQ_Y, &vrle_Y, Jheader.DQT0.QTable);
					Inverse_DCT(res_IDCT_Y, res_afterInvQ_Y);
					for (int i = 0; i < 8; i++) {
						for (int j = 0; j < 8; j++) {
							MCU_Y[i][j] = round_double(res_IDCT_Y[i][j] + 128.0);
							if (MCU_Y[i][j] < 0x00)		MCU_Y[i][j] = 0x00;
							if (MCU_Y[i][j] > 0xFF)		MCU_Y[i][j] = 0xFF;
						}
					}
					for (u8 i = 0; i < 8; i++) {			//write Y to YUV
						if (yPos + i + (u / 2) * 8 >= height)		//ͼƬYƽ����²��Ե
							break;
						for (u8 j = 0; j < 8; j++) {
							if (xPos + j + (u % 2) * 8 >= width)	//ͼƬYƽ����Ҳ��Ե
								break;//continue;
							yuv_Y[(yPos + i + (u / 2) * 8) * width + xPos + j + (u % 2) * 8] = MCU_Y[i][j];
						}
					}
				}

				int			MCU_U[8][8] = { 0 }, MCU_V[8][8] = { 0 };
				double		res_IDCT_U[8][8] = { 0.0 }, res_IDCT_V[8][8] = { 0.0 };
				double		res_afterInvQ_U[8][8] = { 0.0 }, res_afterInvQ_V[8][8] = { 0.0 };
				vector<int> vrle_U, vrle_V;

				//U-channel
				rd_restore_HuffBit_RLE(&vrle_U, &newByte, &newBytePos, &prev_DC_U, max_first_Chromi_DC, max_first_Chromi_AC, \
					HuffTbl_len_UV_DC, HuffTbl_len_UV_AC, rd_HuffTbl_UV_DC, rd_HuffTbl_UV_AC, fp);
				rd_restore_InvQuantize(res_afterInvQ_U, &vrle_U, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_U, res_afterInvQ_U);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_U[i][j] = round_double(res_IDCT_U[i][j] + 128.0);
						if (MCU_U[i][j] < 0x00)		MCU_U[i][j] = 0x00;
						if (MCU_U[i][j] > 0xFF)		MCU_U[i][j] = 0xFF;
					}
				}

				//V-channel
				rd_restore_HuffBit_RLE(&vrle_V, &newByte, &newBytePos, &prev_DC_V, max_first_Chromi_DC, max_first_Chromi_AC, \
					HuffTbl_len_UV_DC, HuffTbl_len_UV_AC, rd_HuffTbl_UV_DC, rd_HuffTbl_UV_AC, fp);
				rd_restore_InvQuantize(res_afterInvQ_V, &vrle_V, Jheader.DQT1.QTable);
				Inverse_DCT(res_IDCT_V, res_afterInvQ_V);
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						MCU_V[i][j] = round_double(res_IDCT_V[i][j] + 128.0);
						if (MCU_V[i][j] < 0x00)		MCU_V[i][j] = 0x00;
						if (MCU_V[i][j] > 0xFF)		MCU_V[i][j] = 0xFF;
					}
				}

				if (toYUV420 == true) {		//(jpeg420)write UV to YUV_420
					for (u8 i = 0; i < 8; i++) {
						if (yPos / 2 + i >= (height + 1) / 2)		//ͼƬUV420ƽ����²��Ե(�ѿ��������߶ȵ�ͼƬ)
							break;
						for (u8 j = 0; j < 8; j++) {
							if (xPos / 2 + j >= (width + 1) / 2)	//ͼƬUV420ƽ����Ҳ��Ե(�ѿ���������ȵ�ͼƬ)
								break;//continue;
							yuv_U[(yPos / 2 + i) * ((width + 1) / 2) + xPos / 2 + j] = MCU_U[i][j];	//U: width/2
							yuv_V[(yPos / 2 + i) * ((width + 1) / 2) + xPos / 2 + j] = MCU_V[i][j];	//V: width/2
						}
					}
				}
				else {							//(jpeg420)write UV to YUV_444
					for (u8 i = 0; i < 8; i++) {
						for (u8 j = 0; j < 8; j++) {
							for (u8 u = 0; u < 4; u++) {
								if (yPos + i * 2 + u / 2 >= height || xPos + j * 2 + u % 2 >= width)
									break;//continue;
								yuv_U[(yPos + i * 2 + u / 2) * width + xPos + j * 2 + u % 2] = MCU_U[i][j];
								yuv_V[(yPos + i * 2 + u / 2) * width + xPos + j * 2 + u % 2] = MCU_V[i][j];
							}
							/*yuv_U[(yPos + i * 2 + 0) * width + xPos + j * 2 + 0] = MCU_U[i][j];
							yuv_U[(yPos + i * 2 + 0) * width + xPos + j * 2 + 1] = MCU_U[i][j];
							yuv_U[(yPos + i * 2 + 1) * width + xPos + j * 2 + 0] = MCU_U[i][j];
							yuv_U[(yPos + i * 2 + 1) * width + xPos + j * 2 + 1] = MCU_U[i][j];

							yuv_V[(yPos + i * 2 + 0) * width + xPos + j * 2 + 0] = MCU_V[i][j];
							yuv_V[(yPos + i * 2 + 0) * width + xPos + j * 2 + 1] = MCU_V[i][j];
							yuv_V[(yPos + i * 2 + 1) * width + xPos + j * 2 + 0] = MCU_V[i][j];
							yuv_V[(yPos + i * 2 + 1) * width + xPos + j * 2 + 1] = MCU_V[i][j];*/
						}
					}
				}
			}
		}
	}
	else if (Jheader.clrY_sample == 0x21)					//yuv422
	{
		//yuv422
	}
	else;

	// EOI
	fread(&seg_label_t, 2, 1, fp);
	Jheader.EOI = rd_BigEnd16(seg_label_t);

	// free all malloc space
	free(rd_HuffTbl_Y_DC);
	free(rd_HuffTbl_Y_AC);
	free(rd_HuffTbl_UV_DC);
	free(rd_HuffTbl_UV_AC);
	free(rd_Lumi_DC_Values);
	free(rd_Chromi_DC_Values);
	free(rd_Lumi_AC_Values);
	free(rd_Chromi_AC_Values);
	return;
}


