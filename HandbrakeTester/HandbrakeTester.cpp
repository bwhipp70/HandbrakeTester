// HandbrakeTester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// To Do:
// 1.  Clean this crap up!
//     Add ifdef debug for cout statements.
// 2.  Integrate SSIM and PSNR from ffmpeg - determines how good of quality the rip is
//     Same file = 1.0 (inf)
//     Worst case I've seen = 0.842 (8.04 db) - lossless to Q=51/Ultrafast
//     Works with both .mkv and .mp4
//     Call it as a function and pass back the four numbers?
//     Eventually just build it into versus external?
// 3.  Make the settings of choice cleaner and easier to include
//     Maybe ifdef?
//     Use lengths to self generate (for x264 {a, b, c, d}, fix the containers to mkv (most common)
// 4.  Change to a given length and position versus chapter
//     Create the base file then use that for all rips - will speed up considerably!
//     Example:  Solo - 6:15 to 6:45
//		--start - at <string:number>
//		Start encoding at a given duration(in seconds), frame, or pts(on a 90kHz clock) (e.g.duration:10, frame : 300, pts : 900000)
//		--stop - at  <string:number>
//		Stop encoding at a given duration(in seconds), frame, or pts(on a 90kHz clock) (e.g.duration:10, frame : 300, pts : 900000)
// 5.  Translate the encoding strings from the batch file (organization and use of --)
// 6.  Create a new output folder with T/D stamp for each run.
// 7.  Check for ffmpeg and fail out if not there.
// A.  For the outside - decide to go .mkv for Plex?
//     Seems like Roku/Apple don't care, but the WebPlayer won't rewind (gets stuck) with MKV with any browser.
//     So, for now, stick with MP4.

// Test with changes (trying out GitHub).

// Links
// https://trac.ffmpeg.org/wiki/Encode/H.264
// https://blogs.msdn.microsoft.com/benjaminperkins/2017/04/04/setting-up-and-using-github-in-visual-studio-2017/
// https://stackoverflow.com/questions/486087/how-to-call-an-external-program-with-parameters
// https://www.geeksforgeeks.org/system-call-in-c/
// https://stackoverflow.com/questions/13982917/trying-to-run-string-command-through-system
// https://stackoverflow.com/questions/2642551/windows-c-system-call-with-spaces-in-command
// https://www.codeproject.com/Articles/1842/A-newbie-s-elementary-guide-to-spawning-processes
// https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
// http://www.cplusplus.com/reference/cstdio/printf/
// https://stackoverflow.com/questions/13550864/error-c4996-ctime-this-function-or-variable-may-be-unsafe
// https://codescracker.com/cpp/program/cpp-program-merge-two-arrays.htm
// https://www.dreamincode.net/forums/topic/61790-number-of-elements-in-an-array/
// https://stackoverflow.com/questions/201718/concatenating-two-stdvectors
// https://stackoverflow.com/questions/4430780/how-can-i-extract-the-file-name-and-extension-from-a-path-in-c
// https://stackoverflow.com/questions/52470593/stdfilesystemcreate-directories-visual-studio-2017
// https://www.programiz.com/cpp-programming/library-function/ctime/strftime
// https://stackoverflow.com/questions/45401822/how-to-convert-filesystem-path-to-string
// https://stackoverflow.com/questions/25201131/writing-csv-files-from-c
// https://stackoverflow.com/questions/5185617/how-can-calculate-the-size-of-any-file-in-c-or-visual-c-net-or-c
// https://stackoverflow.com/questions/36645510/returning-2-values-within-a-function
// https://stackoverflow.com/questions/6394741/can-a-c-function-return-more-than-one-value
// http://www.cplusplus.com/forum/beginner/117874/
// https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix

#include "pch.h"
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <time.h>
#include <stdio.h>
#include <string>
#include <array>
#include <vector>
#include <fstream>


using namespace std;

int main(int argc, char* argv[])
{

	// Set up base variables

	std::string OutputDir = "D:\\Temp\\HBTester";
	std::string HandbrakeDataFile = "D:\\Temp\\HandbrakeRun.csv";
	std::string Chapter = "-c 37";  // For Solo - A Star Wars Story
	std::string Subtitles = "--native-language eng --subtitle scan --subtitle-forced=1 --subtitle-default=1";
	std::string Extension;
	std::string HB_start = "\"\"C:\\Program Files\\Handbrake\\HandBrakeCLI.exe\"\" -i";
	std::string HB_arg1 = "--verbose=1 --no-dvdnav --main-feature --angle 1 --previews 30:0";
	std::string HB_arg2 = "--markers -O --ipod-atom --vfr -a 1,1 -E av_aac,copy:ac3 -6 dp12,auto -R Auto,Auto -B 160,0 -D 0,0 --gain 0,0 --audio-fallback ac3 --loose-anamorphic --modulus 2 --decomb";
	std::string HB_total;
	std::string OutputFile;
	std::string OutputLog;

	// Still leaves input file, chapter, output file, container, encoder string, quality, subtitles, log file
	   
	// "C:\Program Files\Handbrake\HandBrakeCLI.exe" - i "%~1" --verbose = 1 --no - dvdnav --main - feature % Chapter% --angle 1 --previews 30:0 - o "%OutputDir%\%Name%.%DestExt%" - f % Container% --markers - O --ipod - atom % EncoderString% -q % Quality% --vfr - a 1, 1 - E av_aac, copy : ac3 - 6 dpl2, auto - R Auto, Auto - B 160, 0 - D 0, 0 --gain 0, 0 --audio - fallback ac3 --maxWidth %MaxWidth% --loose - anamorphic  --modulus 2 --decomb %Subtitles % 2 > "%OutputDir%\%Name%_log.txt"
	
	vector<string> codec;
	vector<string> preset;
	vector<string> profile;
	vector<string> dest_ext;
	vector<string> container;

	// Encoder = x264
	vector<string> codec_x264 = { "x264", "x264", "x264" };
	vector<string> preset_x264 = { "veryslow", "slower", "slow" };
	vector<string> profile_x264 = { "high", "high", "high" };
	vector<string> dest_ext_x264 = { "mp4", "mp4", "mp4" };
	vector<string> container_x264 = { "av_mp4", "av_mp4", "av_mp4" };

	// Encoder = x264_10bit
	vector<string> codec_x264_10bit = { "x264_10bit", "x264_10bit", "x264_10bit" };
	vector<string> preset_x264_10bit = { "veryslow", "slower", "slow" };
	vector<string> profile_x264_10bit = { "high10", "high10", "high10" };
	vector<string> dest_ext_x264_10bit = { "mp4", "mp4", "mp4" };
	vector<string> container_x264_10bit = { "av_mp4", "av_mp4", "av_mp4" };

	// Encoder = nvenc_h264
	vector<string> codec_nvenc_h264 = { "nvenc_h264", "nvenc_h264" };
	vector<string> preset_nvenc_h264 = { "slow", "hq" };
	vector<string> profile_nvenc_h264 = { "high", "high" };
	vector<string> dest_ext_nvenc_h264 = { "mp4", "mp4" };
	vector<string> container_nvenc_h264 = { "av_mp4", "av_mp4" };

	// Encoder = x265
	vector<string> codec_x265 = { "x265", "x265", "x265" };
	vector<string> preset_x265 = { "veryslow", "slower", "slow" };
	vector<string> profile_x265 = { "main", "main", "main" };
	vector<string> dest_ext_x265 = { "mp4", "mp4", "mp4" };
	vector<string> container_x265 = { "av_mp4", "av_mp4", "av_mp4" };

	// Encoder = x265_10bit
	vector<string> codec_x265_10bit = { "x265_10bit", "x265_10bit", "x265_10bit" };
	vector<string> preset_x265_10bit = { "veryslow", "slower", "slow" };
	vector<string> profile_x265_10bit = { "main10", "main10", "main10" };
	vector<string> dest_ext_x265_10bit = { "mp4", "mp4", "mp4" };
	vector<string> container_x265_10bit = { "av_mp4", "av_mp4", "av_mp4" };

	// Encoder = x265_12bit
	vector<string> codec_x265_12bit = { "x265_12bit", "x265_12bit", "x265_12bit" };
	vector<string> preset_x265_12bit = { "veryslow", "slower", "slow" };
	vector<string> profile_x265_12bit = { "main12", "main12", "main12" };
	vector<string> dest_ext_x265_12bit = { "mp4", "mp4", "mp4" };
	vector<string> container_x265_12bit = { "av_mp4", "av_mp4", "av_mp4" };

	// Encoder = nvenc_h265
	vector<string> codec_nvenc_h265 = { "nvenc_h265", "nvenc_h265" };
	vector<string> preset_nvenc_h265 = { "slow", "hqslower" };
	vector<string> profile_nvenc_h265 = { "main", "main" };
	vector<string> dest_ext_nvenc_h265 = { "mp4", "mp4" };
	vector<string> container_nvenc_h265 = { "av_mp4", "av_mp4" };

	// Encoder = VP8
	vector<string> codec_VP8 = { "VP8", "VP8", "VP8" };
	vector<string> preset_VP8 = { "veryslow", "slower", "slow" };
	vector<string> profile_VP8 = { "high", "high", "high" };
	vector<string> dest_ext_VP8 = { "mkv", "mkv", "mkv" };
	vector<string> container_VP8 = { "av_mkv", "av_mkv", "av_mkv" };

	// Encoder = VP9
	vector<string> codec_VP9 = { "VP9", "VP9", "VP9" };
	vector<string> preset_VP9 = { "veryslow", "slower", "slow" };
	vector<string> profile_VP9 = { "high", "high", "high" };
	vector<string> dest_ext_VP9 = { "mkv", "mkv", "mkv" };
	vector<string> container_VP9 = { "av_mkv", "av_mkv", "av_mkv" };

	// Encoder = theora
	vector<string> codec_theora = { "theora" };
	vector<string> preset_theora = { "" };
	vector<string> profile_theora = { "" };
	vector<string> dest_ext_theora = { "mkv" };
	vector<string> container_theora = { "av_mkv" };

	// Encoder = mpeg4
	vector<string> codec_mpeg4 = { "mpeg4" };
	vector<string> preset_mpeg4 = { " " };
	vector<string> profile_mpeg4 = { " " };
	vector<string> dest_ext_mpeg4 = { "mp4" };
	vector<string> container_mpeg4 = { "av_mp4" };

	// Encoder = mpeg2
	vector<string> codec_mpeg2 = { "mpeg2" };
	vector<string> preset_mpeg2 = { " " };
	vector<string> profile_mpeg2 = { " " };
	vector<string> dest_ext_mpeg2 = { "mp4" };
	vector<string> container_mpeg2 = { "av_mp4" };


	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_x264.begin()),
		std::make_move_iterator(codec_x264.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_x264_10bit.begin()),
		std::make_move_iterator(codec_x264_10bit.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_nvenc_h264.begin()),
		std::make_move_iterator(codec_nvenc_h264.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_x265.begin()),
		std::make_move_iterator(codec_x265.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_x265_10bit.begin()),
		std::make_move_iterator(codec_x265_10bit.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_x265_12bit.begin()),
		std::make_move_iterator(codec_x265_12bit.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_nvenc_h265.begin()),
		std::make_move_iterator(codec_nvenc_h265.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_VP8.begin()),
		std::make_move_iterator(codec_VP8.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_VP9.begin()),
		std::make_move_iterator(codec_VP9.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_theora.begin()),
		std::make_move_iterator(codec_theora.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_mpeg4.begin()),
		std::make_move_iterator(codec_mpeg4.end())
	);
	codec.insert(
		codec.end(),
		std::make_move_iterator(codec_mpeg2.begin()),
		std::make_move_iterator(codec_mpeg2.end())
	);


	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_x264.begin()),
		std::make_move_iterator(preset_x264.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_x264_10bit.begin()),
		std::make_move_iterator(preset_x264_10bit.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_nvenc_h264.begin()),
		std::make_move_iterator(preset_nvenc_h264.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_x265.begin()),
		std::make_move_iterator(preset_x265.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_x265_10bit.begin()),
		std::make_move_iterator(preset_x265_10bit.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_x265_12bit.begin()),
		std::make_move_iterator(preset_x265_12bit.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_nvenc_h265.begin()),
		std::make_move_iterator(preset_nvenc_h265.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_VP8.begin()),
		std::make_move_iterator(preset_VP8.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_VP9.begin()),
		std::make_move_iterator(preset_VP9.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_theora.begin()),
		std::make_move_iterator(preset_theora.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_mpeg4.begin()),
		std::make_move_iterator(preset_mpeg4.end())
	);
	preset.insert(
		preset.end(),
		std::make_move_iterator(preset_mpeg2.begin()),
		std::make_move_iterator(preset_mpeg2.end())
	);

	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_x264.begin()),
		std::make_move_iterator(profile_x264.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_x264_10bit.begin()),
		std::make_move_iterator(profile_x264_10bit.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_nvenc_h264.begin()),
		std::make_move_iterator(profile_nvenc_h264.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_x265.begin()),
		std::make_move_iterator(profile_x265.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_x265_10bit.begin()),
		std::make_move_iterator(profile_x265_10bit.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_x265_12bit.begin()),
		std::make_move_iterator(profile_x265_12bit.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_nvenc_h265.begin()),
		std::make_move_iterator(profile_nvenc_h265.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_VP8.begin()),
		std::make_move_iterator(profile_VP8.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_VP9.begin()),
		std::make_move_iterator(profile_VP9.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_theora.begin()),
		std::make_move_iterator(profile_theora.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_mpeg4.begin()),
		std::make_move_iterator(profile_mpeg4.end())
	);
	profile.insert(
		profile.end(),
		std::make_move_iterator(profile_mpeg2.begin()),
		std::make_move_iterator(profile_mpeg2.end())
	);

	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_x264.begin()),
		std::make_move_iterator(dest_ext_x264.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_x264_10bit.begin()),
		std::make_move_iterator(dest_ext_x264_10bit.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_nvenc_h264.begin()),
		std::make_move_iterator(dest_ext_nvenc_h264.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_x265.begin()),
		std::make_move_iterator(dest_ext_x265.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_x265_10bit.begin()),
		std::make_move_iterator(dest_ext_x265_10bit.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_x265_12bit.begin()),
		std::make_move_iterator(dest_ext_x265_12bit.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_nvenc_h265.begin()),
		std::make_move_iterator(dest_ext_nvenc_h265.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_VP8.begin()),
		std::make_move_iterator(dest_ext_VP8.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_VP9.begin()),
		std::make_move_iterator(dest_ext_VP9.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_theora.begin()),
		std::make_move_iterator(dest_ext_theora.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_mpeg4.begin()),
		std::make_move_iterator(dest_ext_mpeg4.end())
	);
	dest_ext.insert(
		dest_ext.end(),
		std::make_move_iterator(dest_ext_mpeg2.begin()),
		std::make_move_iterator(dest_ext_mpeg2.end())
	);

	container.insert(
		container.end(),
		std::make_move_iterator(container_x264.begin()),
		std::make_move_iterator(container_x264.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_x264_10bit.begin()),
		std::make_move_iterator(container_x264_10bit.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_nvenc_h264.begin()),
		std::make_move_iterator(container_nvenc_h264.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_x265.begin()),
		std::make_move_iterator(container_x265.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_x265_10bit.begin()),
		std::make_move_iterator(container_x265_10bit.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_x265_12bit.begin()),
		std::make_move_iterator(container_x265_12bit.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_nvenc_h265.begin()),
		std::make_move_iterator(container_nvenc_h265.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_VP8.begin()),
		std::make_move_iterator(container_VP8.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_VP9.begin()),
		std::make_move_iterator(container_VP9.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_theora.begin()),
		std::make_move_iterator(container_theora.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_mpeg4.begin()),
		std::make_move_iterator(container_mpeg4.end())
	);
	container.insert(
		container.end(),
		std::make_move_iterator(container_mpeg2.begin()),
		std::make_move_iterator(container_mpeg2.end())
	);



	int MaxWidth;
	int Quality;
	int QStart = 18;
	int QEnd = 24;
	int loop_enc = 0;
	int loop_q = 0;
	int max_enc = 0;

	cout << "OutputDir = " << OutputDir << ".\n";
	cout << "Length = " << argc << "\n";


//	std::experimental::filesystem::path p(HandbrakeDataFile);
//	std::cout << "filename and extension: " << p.filename() << std::endl; // "file.ext"
//	std::cout << "filename only: " << p.stem() << std::endl;              // "file"

//	std::string filename = p.stem().string();

	// Check to see if a file was dragged and then set the MaxWidth based on the extension.

	std::experimental::filesystem::path p;
	std::string fn;
	std::string filename;
	std::string fullfilename;


	cout << "argc =" << argc << "\n";

	if (argc > 1)
	{
		fullfilename = argv[1];
		std::experimental::filesystem::path p(argv[1]);
		cout << "Source File! \n"; 
		fn = p.stem().string();
		filename = p.filename().string();
		std::cout << "filename only: " << fn << std::endl;
		std::cout << "filename only: " << p.stem() << std::endl;
	} else {
		fullfilename = OutputDir + "\\bad.txt";
		cout << "No Source File! \n"; 
		fn = "NoSourceFile";
		filename = "NoSourceFile.iso";
//		std::experimental::filesystem::path p("C:\\NoSourceFile.iso");
//		cout << "No Source File \n";
//		std::string fn = p.stem().string();
//		std::cout << "filename only: " << fn << std::endl; 
//		std::cout << "filename only: " << p.stem() << std::endl;
	}

//	std::string fn = p.stem().string();
	cout << "Full Filename = " << filename << "\n";
	cout << "Filename No Ext = " << fn << "\n";

//	std::experimental::filesystem::path p(fn);

//	std::string filename = p.stem().string();

	std::string f_ext = filename.substr(filename.find_last_of(".") + 1);

	cout << "Extension Portion = " << f_ext << "\n";

//	if (fn.substr(fn.find_last_of(".") + 1) == "iso")
	if (f_ext == "iso")
	{
		MaxWidth = 1280;
	}
	else {
		MaxWidth = 1920;
	}

//	if (argc > 1)
//	{
//		cout << "Sample File = " << argv[1] << "\n";
//		std::experimental::filesystem::path p(argv[1]);
//		
//		std::string fn = argv[1];
//		if (fn.substr(fn.find_last_of(".") + 1) == "iso") {
//			MaxWidth = 1280;
//		}
//		else {
//			MaxWidth = 1920;
//		}
//	}
//	else {
//		std::experimental::filesystem::path p("C:\\NoFileInput.none");
//		std::string filename = "NoFileInput";
//		std::string filename = p.stem().string();
//		cout << "No File Input \n";
//		cout << "Filename = " << filename << "\n";
//		MaxWidth = 0;
//	}

	cout << "Filename = " << fn << "\n";
	cout << "MaxWidth = " << MaxWidth << "\n";
	
	max_enc = codec.size();
	cout << "Size of Array = " << max_enc << "\n";
	
	cout << "0 - " << codec[0] << "\n";
	cout << "1 - " << codec[1] << "\n";
	cout << "2 - " << codec[2] << "\n";
	cout << "3 - " << codec[3] << "\n";
	cout << "4 - " << codec[4] << "\n";

	system("pause");

// Open the output file
	std::ofstream csvfile;
	csvfile.open(HandbrakeDataFile);
	csvfile << "Handbrake Video Settings Comparison Tool\n";
	csvfile << "\n";
	csvfile << "Source File: " << fullfilename << "\n";
	csvfile << "Run#,CODEC,Preset,Profile,DestExt,Container,Quality,Total Seconds,File Size,DateTime\n";

	cout << "Handbrake Video Settings Comparison Tool\n";
	cout << "\n";
	cout << "Source File: " << fullfilename << "\n";
	cout << "Run#,CODEC,Preset,Profile,DestExt,Container,Quality,Total Seconds,File Size,DateTime\n";


	int counter = 0;

	for (loop_enc = 0; loop_enc < max_enc; loop_enc = loop_enc + 1) {
//	for (loop_enc = 0; loop_enc < 1; loop_enc = loop_enc + 1) {

	// Start to loop through the Quality settings

		for (loop_q = QStart; loop_q <= QEnd; loop_q = loop_q + 1) {
			
			counter = counter + 1;

			// current date/time based on current system
			time_t now = time(0);

			// convert now to string form
//			char* dt = ctime_s(&now);
			char dt[26];
			errno_t err = ctime_s(dt, 26, &now);

			cout << "The local date and time is: " << dt << endl;

			time_t curr_time;
			tm * curr_tm;
			struct tm timeinfo;
			char dt_string[100];
			time(&curr_time);
			localtime_s(&timeinfo, &curr_time);
//			curr_tm = localtime(&curr_time);

//			strftime(dt_string, 50, "%Y%m%d_%H%M", curr_tm);
			strftime(dt_string, 50, "%Y%m%d_%H%M", &timeinfo);
			cout << "Date and Time for File = " << dt_string << "\n";

			OutputFile = OutputDir + "\\" + fn + "_" + codec[loop_enc] + "_" + "q" + std::to_string(loop_q) + "_" + std::to_string(MaxWidth) + "_" + dt_string + "." + dest_ext[loop_enc];
			cout << "Output File: " << OutputFile + "\n";

			OutputLog = OutputDir + "\\" + fn + "_" + codec[loop_enc] + "_" + "q" + std::to_string(loop_q) + "_" + std::to_string(MaxWidth) + "_" + dt_string + "_log.txt";
			cout << "Output File: " << OutputLog + "\n";

			HB_total = HB_start + " " + fullfilename + " " + HB_arg1 + " " + Chapter + " -o " + OutputFile +
				" -e " + codec[loop_enc] +
				" --encoder-preset " + preset[loop_enc] +
				" --encoder-profile " + profile[loop_enc] +
				" --encoder-level 4.0 " +
				Subtitles + " " + HB_arg2 + " " +
				" -f " + container[loop_enc] +
				" -q " + std::to_string(loop_q) +
				" 2> " + OutputLog;
			
			auto start = std::chrono::system_clock::now();

			cout << "Loop Encoder = " << loop_enc << "\n";
			cout << "Loop Quality = " << loop_q << "\n";
			cout << "HB command = " << HB_total << "\n";

			if (argc > 1) {
				// Call with System
				system(HB_total.c_str());
//				cout << "File was dragged and dropped.\n";
			}
			else {
				// Just echo to the screen
				cout << "No file to use.\n";
			}
					   
//			system("dir D:\\Temp");

			auto end = std::chrono::system_clock::now();

			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

//			std::cout << "finished computation at " << std::time_t(&end_time)
//				<< "elapsed time: " << elapsed_seconds.count() << "s\n";
			std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

			long end_file;
			ifstream resultfile(OutputFile);
			resultfile.seekg(0, ios::end);
			end_file = resultfile.tellg();
			resultfile.close();
			cout << "size is: " << end_file << " bytes.\n";

//			csvfile << "Run#,CODEC,Preset,Profile,DestExt,Container,Quality,Total Seconds,File Size,DateTime\n";
			csvfile << counter << "," << codec[loop_enc] << "," << preset[loop_enc] << "," << profile[loop_enc] << "," << dest_ext[loop_enc]
				<< "," << container[loop_enc] << "," << loop_q << "," << elapsed_seconds.count() << "," << end_file << "," << dt_string << "\n";
			cout << counter << "," << codec[loop_enc] << "," << preset[loop_enc] << "," << profile[loop_enc] << "," << dest_ext[loop_enc]
				<< "," << container[loop_enc] << "," << loop_q << "," << elapsed_seconds.count() << "," << end_file << "," << dt_string << "\n";

		}

	
	}

	csvfile.close();

	system("pause");
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
