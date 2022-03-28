#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>		//malloc, free
#include <cstring>

#include <iostream>
#include <string>
#include <vector>

#include "JPEG.h"

using namespace std;


string generate_obj_name(string ori_name, string ext_name);
void active_JPEG_dec(string path_dep, string path_des, string fmt);


//在cmd中使用ffmpeg打开yuv文件指令: ffplay -f rawvideo -pixel_format yuv420p -s 1080*2340 xxx.yuv
int main(int argc, char* argv[])
{
	string	path = "C:\\Users\\93052\\Desktop\\SDcard\\PICTURE\\jpg\\";
	//string	path_deprt = "D:\\边志奇\\研一参赛\\中兴捧月2021\\data\\";
	//string	filename = "myself_ps.jpg";
	string	filename = "fun.jpg";
	string	ext_name = ".yuv";

	active_JPEG_dec(path + filename, path + generate_obj_name(filename, ext_name), "toYUV420");
	
	return 0;
}


string generate_obj_name(string ori_name, string ext_name)
{
	string obj_name = ori_name;
	size_t pos = obj_name.rfind(".", obj_name.size());
	return obj_name.substr(0, pos) + ext_name;
}


void active_JPEG_dec(string path_dep, string path_des, string fmt)
{
	if (fmt != "toYUV420" && fmt != "toYUV444") {
		cout << "[ERROR]invalid argument <opt>." << endl;
		return;
	}
	//打印警告
	cout << " -------------------------------------------------------------------------" << endl;
	cout << " [warn]1.对于超大尺寸图片应该分大块依次处理以提高速度  2.程序不支持yuv422 " << endl;
	cout << " -------------------------------------------------------------------------" << endl;

	cout << " [info]source path: " << path_dep << endl;
	cout << " [info]destination: " << path_des << endl;
	FILE* fp_rd, * fp_wr;
	if ((fp_rd = fopen(path_dep.c_str(), "rb")) == NULL) {
		cout << "[ERROR]failed to open jpeg file." << endl;
		return;
	}
	cout << "+[note]open jpeg file successfully." << endl;

	//预读取图片尺寸和采样格式
	u8  sample_t = 0;
	u16 width = 0, height = 0;
	pre_rd_SOF0_info(fp_rd, &sample_t, &width, &height);
	//if (imw_t != width || imh_t != height)		return;
	cout << "\n [info]image's width = " << width << ", height = " << height << endl;
	if (sample_t == 0x11)
		cout << "+[note]this jpeg is in jpeg444 format, ";
	else if (sample_t == 0x22)
		cout << "+[note]this jpeg is in jpeg420 format, ";
	else {
		cout << "[ERROR]invalid jpeg sample format or yuv422." << endl;
		return;
	}
	cout << "and you expect to convert this jpeg " << fmt << "." << endl;

	//根据YUV格式修正U/V平面的大小
	size_t uvplane_size = 0;
	if (fmt == "toYUV420")		uvplane_size = (size_t)((height + 1) / 2) * ((width + 1) / 2);
	else						uvplane_size = (size_t)height * width;

	//开辟Y/U/V的内存
	u8* yuv_Y = (u8*)malloc(sizeof(u8) * width * height);
	u8* yuv_U = (u8*)malloc(sizeof(u8) * uvplane_size);
	u8* yuv_V = (u8*)malloc(sizeof(u8) * uvplane_size);
	if (yuv_Y == NULL || yuv_U == NULL || yuv_V == NULL)
		return;
	
	//解码
	cout << "\n#[work]start to decode jpeg ..." << endl;
	if (fmt == "toYUV420")
		Dec_JPEG_to_YUV(fp_rd, yuv_Y, yuv_U, yuv_V, true);
	else
		Dec_JPEG_to_YUV(fp_rd, yuv_Y, yuv_U, yuv_V, false);
	cout << "#[work]decode finished." << endl;

	fclose(fp_rd);
	fp_rd = NULL;
	cout << "+[note]jpeg file is close." << endl;
	if ((fp_wr = fopen(path_des.c_str(), "wb")) == NULL) {
		cout << "[ERROR]failed to generate obj file." << endl;
		return;
	}
	cout << "+[note]generate obj file successfully." << endl;

	//写入
	cout << "\n#[work]writing data to obj file ..." << endl;
	fwrite(yuv_Y, (size_t)width * height, 1, fp_wr);
	fwrite(yuv_U, (size_t)uvplane_size, 1, fp_wr);
	fwrite(yuv_V, (size_t)uvplane_size, 1, fp_wr);
	cout << "#[work]write data finished." << endl;

	fclose(fp_wr);
	fp_wr = NULL;
	cout << "+[note]obj file is close." << endl;

	//释放空间
	free(yuv_Y);
	free(yuv_U);
	free(yuv_V);
	cout << " [info]all procedure finished successfully.\n > Congrats! <";
	return;
}

