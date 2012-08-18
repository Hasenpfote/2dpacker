
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
using namespace std;
#include "packer.h"

static DataArray inputs;
static Packer packer;
static unsigned int padding = 0;
static bool aligned = false;
static std::vector<std::string> input_images;
static bool with_alpha = false;
static std::string output_path;
static std::vector<int> params;

typedef enum{
	OutputMode_WriteOnly,
	OutputMode_DisplayOnly,
	OutputMode_Both,
	OutputMode_Unsupported,
	OutputMode_Size = OutputMode_Unsupported,
}OutputModeType;

static OutputModeType om_type = OutputMode_Unsupported;

typedef enum{
	OutputFormat_Bmp,
	OutputFormat_Png,
	OutputFormat_Jpg,
	OutputFormat_Unsupported,
	OutputFormat_Size = OutputFormat_Unsupported
}OutputFormatType;

static OutputFormatType ofmt_type = OutputFormat_Unsupported;

static OutputFormatType getExtension(const std::string& path){
	string::size_type index = path.find_last_of(".");
	if(index == string::npos)
		return OutputFormat_Unsupported;
	std::string ext = path.substr(index);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if(ext.compare(".bmp") == 0)
		return OutputFormat_Bmp;
	if(ext.compare(".png") == 0)
		return OutputFormat_Png;
	if(ext.compare(".jpg") == 0)
		return OutputFormat_Jpg;
	return OutputFormat_Unsupported;
}

static int getChannels(OutputFormatType type){
	switch(type){
	case OutputFormat_Bmp: return 3;
	case OutputFormat_Png: return (with_alpha)? 4 : 3;
	case OutputFormat_Jpg: return 3;
	default:
		break;
	}
	return 0;
}

/**
 * 準備
 */
static void setup(){
	std::vector<std::string>::iterator it = input_images.begin();
	for(; it != input_images.end(); it++){
		cv::Mat img = cv::imread(*it, -1);
		if(img.empty()){
			std::cerr << "[warrning] could not read file \"" << *it << "\"" << std::endl;
			continue;
		}
		std::cout << "[info] file:\"" << *it << "\" w:" << img.size().width << " h:" << img.size().height << " ch:" << img.channels() << std::endl;
		inputs.push_back(Data(*it, img.size().width, img.size().height));
		if(img.channels() == 4)
			with_alpha = true;
	}
}

/**
 * 出力フォーマットに基づき入力データを調整する
 */
static void adjustmentImage(cv::Mat& img, int ch){
	// BGR(A)に分離
	std::vector<cv::Mat> mv;
	cv::split(img, mv);
	if(img.channels() == 1){
		// gray-scale
		if(ch == 3){	// to bgr
			mv.push_back(mv[0].clone());
			mv.push_back(mv[0].clone());
			mv.push_back(mv[0].clone());
		}
		else{			// to bgra
			mv.push_back(mv[0].clone());
			mv.push_back(mv[0].clone());
			mv.push_back(mv[0].clone());
			cv::Mat alpha(img.size(), CV_8UC1, cv::Scalar(255));
			mv.push_back(alpha.clone());
		}
		cv::merge(mv, img);
	}
	else
	if(img.channels() == 3){
		// bgr to bgra
		cv::Mat alpha(img.size(), CV_8UC1, cv::Scalar(255));
		mv.push_back(alpha.clone());
		cv::merge(mv, img);
	}
	else{			
		// bgra to bgr
		mv.pop_back();
		cv::merge(mv, img);
	}
}

/**
 * packerに基づきイメージを結合する
 */
static void combineImages(){
	// 出力フォーマットの選別
	const int ch = getChannels(ofmt_type);
	const int type = (ch == 4)? CV_8UC4 : CV_8UC3;
	const cv::Scalar scalar = (ch == 4)? cv::Scalar(127, 127, 127, 255) : cv::Scalar(127, 127, 127);	// 初期色

	cv::Mat dst(cv::Size(packer.getW(), packer.getH()), type, scalar);

	int count = 0;	// for debug
	DataArray::iterator it = inputs.begin();
	for(; it != inputs.end(); it++){
		if(!it->fit)
			continue;	// skip

		cv::Mat src = cv::imread(it->path, -1);
		if(src.channels() != ch){
			adjustmentImage(src, ch);
		}
		cv::Mat roi = dst(cv::Rect(it->fit->rect.getX() + packer.getPadding(), it->fit->rect.getY() + packer.getPadding(), it->w, it->h));
		src.copyTo(roi);
		// for debug
#if 0
		cv::rectangle(dst, cv::Rect(it->fit->rect.getX(), it->fit->rect.getY(), it->fit->rect.getW(), it->fit->rect.getH()), cv::Scalar(255, 255, 255), 1);
		std::ostringstream oss;
		oss << it->w << "x" << it->h << "(" << count << ")";
		count++;
		const std::string size = oss.str();
		cv::putText(dst, size, cv::Point(it->fit->rect.getX() + 1, it->fit->rect.getY() + 21), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
		cv::putText(dst, size, cv::Point(it->fit->rect.getX(), it->fit->rect.getY() + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
#endif
	}
	// 出力
	if((om_type == OutputMode_WriteOnly) || (om_type == OutputMode_Both)){
		bool ret = cv::imwrite(output_path, dst, params);
	}
	if((om_type == OutputMode_DisplayOnly) || (om_type == OutputMode_Both)){
		std::ostringstream oss;
		oss << "packed image(" << packer.getW() << " x " << packer.getH() << ")";
		const std::string name = oss.str();
		cv::namedWindow(name, CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO);
		cv::imshow(name, dst);
		if(ch == 4){
			std::vector<cv::Mat> mv;
			cv::split(dst, mv);
			cv::Mat dsta = mv[3].clone();
			cv::imshow("packed image(alpha)", dsta);
		}
	}
}

/**
 * エントリ
 */
int main(int argc, char *argv[]){
#if	0	// for debug
	std::vector<std::string> output_array;

	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0163.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0164.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0165.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0166.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0167.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0168.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0169.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0170.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0172.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0173.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0174.jpg;");
	output_array.push_back("E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\miku.png;");

	char* _argv[] = {
		"",
		"-i=E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0163.jpg;E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\eyes0164.jpg;",
//		"-o=E:\\GitHubRepository\\2dpacker\\2dpacker\\image\\packed.png",
//		"-p=100",
		"-pad=0",
		"-aln=false",
		"-d=true"
	};

	std::string output("-i=");
	if(!output_array.empty()){
		std::vector<std::string>::iterator it = output_array.begin();
		for(; it != output_array.end(); it++){
			output.append(*it);
		}
		_argv[1] = const_cast<char*>(output.c_str());
	}

	argv = _argv;
	argc = sizeof(_argv) / sizeof(_argv[0]);
#endif
	// {：パーサ文字列の開始
	// }：パーサ文字列の終了
	// |：セパレータ
	// ショートネーム，フルネーム，デフォルト値，ヘルプ文
	const char* keys = {
		"{i|input     |      |input files}"
		"{o|output    |      |output file}"
		"{p|param     |-1    |output parameter}"
		"{pad|padding |0     |padding pixel}"
		"{aln|aligned |false |aligned power of two boundary}"
		"{d|display   |false |display images}"
	};

	cv::CommandLineParser parser(argc, argv, keys);
	if(argc == 1){
		parser.printParams();
		return EXIT_SUCCESS;
	}

	std::string param_i = parser.get<std::string>("i");
	const std::string param_o = parser.get<std::string>("o");
	int param_p = parser.get<int>("p");
	const unsigned int param_pad = parser.get<unsigned int>("pad");
	const bool param_aln = parser.get<bool>("aln");
	const bool param_d = parser.get<bool>("d");

	if(param_i.empty()){
		std::cout << "[error] empty input data." << std::endl;
		return EXIT_FAILURE;
	}
	else{
#if 0
		std::stringstream ss(param_i);
		std::string temp;
		while(ss >> temp){
			input_images.push_back(temp);
		}
#else
		std::string temp;
		while(param_i.find(";", 0) != std::string::npos){
			size_t pos = param_i.find(";", 0);
			temp = param_i.substr(0, pos);
			param_i.erase(0, pos + 1);
			input_images.push_back(temp);
		}
#endif
	}
	// 出力設定
	padding = param_pad;
	aligned = param_aln;
	output_path = param_o;
	if(!output_path.empty()){
		ofmt_type = getExtension(output_path);
		if(ofmt_type == OutputFormat_Unsupported){
			std::cerr << "[error] unsupported output format." << std::endl;
			return EXIT_FAILURE;
		}
		if(ofmt_type == OutputFormat_Jpg){
			params.push_back(CV_IMWRITE_JPEG_QUALITY);
			param_p = (param_p == -1)? 95 : param_p;	// 0-100(default=95)
			params.push_back(param_p);
		}
		else
		if(ofmt_type == OutputFormat_Png){
			params.push_back(CV_IMWRITE_PNG_COMPRESSION);
			param_p = (param_p == -1)? 3 : param_p;	// 0-9(default=3)
			params.push_back(param_p);
		}
//		else
//		if(output_type == Output_Pnm){
//			params.push_back(CV_IMWRITE_PXM_BINARY);
//			param_p = (param_p == -1)? 1 : param_p;	// 0:ascii 1:binary(default=1)
//			params.push_back(param_p);
//		}
		om_type = (param_d)? OutputMode_Both : OutputMode_WriteOnly;
	}
	else{
		ofmt_type = OutputFormat_Png;
		om_type = OutputMode_DisplayOnly;
	}

	// 準備を行う
	setup();

	// パッキング
	packer.setPadding(padding);
	packer.setAligned(aligned);
	if(!packer.pack(inputs)){
		std::cerr << "[error] failed to pack." << std::endl;
		return EXIT_FAILURE;
	}

	// 画像の合成
	combineImages();

	cv::waitKey(0);

	return EXIT_SUCCESS;
}
