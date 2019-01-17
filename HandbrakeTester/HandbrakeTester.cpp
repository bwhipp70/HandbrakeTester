// HandbrakeTester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// To Do:
// 1.  Clean this crap up!
//     Add ifdef debug for cout statements.
// 7.  Check for ffmpeg and fail out if not there.
// 8.  Integrate Boost libraries.
// 9.  Put quotes around filenames to catch spaces.
// A.  For the outside - decide to go .mkv for Plex?
//     Seems like Roku/Apple don't care, but the WebPlayer won't rewind (gets stuck) with MKV with any browser.
//     So, for now, stick with MP4.

// Lossless
// Needs to be:
// --encoder-profile high444
// --encoder-preset slower and veryslow produce identical images and file sizes.

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
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <sstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <cstdio>

using namespace std;

namespace logging = boost::log;

void init_logging()
{
	// Logging Levels:
	// trace, debug, info, warning, error, fatal
	// https://blog.scalyr.com/2018/07/getting-started-quickly-c-logging/
	
	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logging::trivial::debug
	);
}

std::wstring FormatTime(boost::posix_time::ptime now)
{
	// Modified from:  https://stackoverflow.com/questions/5018188/how-to-format-a-datetime-to-string-using-boost
	using namespace boost::posix_time;
	static std::locale loc(std::wcout.getloc(),
		new wtime_facet(L"%Y%m%d_%H%M%S"));

	std::basic_stringstream<wchar_t> wss;
	wss.imbue(loc);
	wss << now;
	return wss.str();
}


int main(int argc, char* argv[])
{

	// Set up base variables

	std::string OutputDir = "D:\\Temp\\HBTester_";
	std::string HandbrakeDataFile = "HandbrakeRun_";
	std::string Chapter = "-c 37";  // For Solo - A Star Wars Story
	std::string Subtitles = "--native-language eng --subtitle scan --subtitle-forced=1 --subtitle-default=1";
	std::string Extension;
	std::string HB_start = "\"\"C:\\Program Files\\Handbrake\\HandBrakeCLI.exe\"\"";
	std::string HB_arg1 = "--verbose=1 --no-dvdnav --main-feature --angle 1 --previews 30:0";
	std::string HB_arg2 = "--markers -O --ipod-atom --vfr -a 1,1 -E av_aac,copy:ac3 -6 dp12,auto -R Auto,Auto -B 160,0 -D 0,0 --gain 0,0 --audio-fallback ac3 --loose-anamorphic --modulus 2 --decomb";
	std::string HB_total;
	std::string HB_GeneralOptions = "--verbose=1 --no-dvdnav";
	std::string HB_SourceOptions  = "--main-feature --angle 1 --previews 30:0";  // Requires Chapter or Duration Info
	std::string HB_DestinationOptions = "--markers --optimize";  // Requires container Info
	std::string HB_VideoOptions = "--vfr --encoder-level 4.0";  // Requires encoder and quality Info
	std::string HB_AudioOptions = "--audio 1,1 --aencoder av_aac,copy:ac3 --audio-fallback ac3 --ab 160,0 --mixdown dpl2,auto --arate Auto,Auto --drc 0,0 --gain 0,0";
	std::string HB_PictureOptions = "--loose-anamorphic --modulus 2";  // Requires maxwidth
	std::string HB_FiltersOptions = "--decomb";
	std::string HB_SubtitlesOptions = "--native-language eng --subtitle scan --subtitle-forced=1 --subtitle-default=1";
	std::string OutputFile;
	std::string OutputLog;

	std::string HB_Lossless_Encode = "--encoder x264 --encoder-preset=veryslow  --encoder-profile=high444  --quality 0 --format mp4";
	std::string Lossless_fn;
//	std::string HB_Lossless_Chapter = "--start-at duration:375 --stop-at duration:30";
	std::string HB_Lossless_Chapter = "--chapters 37";
	// --chapters 1 --> Selects only Chapter 1
	// --start - at duration : 375 --stop - at duration : 30
	//		-> Start At is the number of seconds into the clip, Stop At is how long in seconds of the clip
	//		-> So this is a 30 second clip starting at 6 minutes, 15 seconds into the feature

	std::string ffmpeg_path = "D:\\Temp\\HBTester\\ffmpeg.exe";
	std::string ssim_call;

	int pos = 0;
	std::string ssim_ratio;
	std::string ssim_db;
	std::string psnr_ratio;

	// Still leaves input file, chapter, output file, container, encoder string, quality, subtitles, log file
	   
	// "C:\Program Files\Handbrake\HandBrakeCLI.exe" - i "%~1" --verbose = 1 --no - dvdnav --main - feature % Chapter% --angle 1 --previews 30:0 - o "%OutputDir%\%Name%.%DestExt%" - f % Container% --markers - O --ipod - atom % EncoderString% -q % Quality% --vfr - a 1, 1 - E av_aac, copy : ac3 - 6 dpl2, auto - R Auto, Auto - B 160, 0 - D 0, 0 --gain 0, 0 --audio - fallback ac3 --maxWidth %MaxWidth% --loose - anamorphic  --modulus 2 --decomb %Subtitles % 2 > "%OutputDir%\%Name%_log.txt"
	
	// Codec Combination Vectors
	// Format is:  CODEC, preset, profile, extension, container.
	// See bottom of file for valid combinations.

	vector< vector<string>> codec_list;
	codec_list.resize(120);  // Set arbitrarily high at first, will trim down later in code.

	// Place comments to skip sections.
	// For empty fields, use "" as opposed to " ".
	// Codecs 0-9 - x264
	codec_list[0] = { "x264", "veryslow", "high", "mp4", "av_mp4" };
	codec_list[1] = { "x264", "slower", "high", "mp4", "av_mp4" };
	codec_list[2] = { "x264", "slow", "high", "mp4", "av_mp4" };
	// Codecs 10-19 - x264_10bit
	codec_list[10] = { "x264_10bit", "veryslow", "high10", "mp4", "av_mp4" };
	codec_list[11] = { "x264_10bit", "slower", "high10", "mp4", "av_mp4" };
	codec_list[12] = { "x264_10bit", "slow", "high10", "mp4", "av_mp4" };
	// Codecs 20-29 - nvenc_h264
	codec_list[20] = { "nvenc_h264", "slow", "high", "mp4", "av_mp4" };
	codec_list[21] = { "nvenc_h264", "hq", "high", "mp4", "av_mp4" };
	// Codecs 30-39 - x265
	codec_list[30] = { "x265", "veryslow", "main", "mp4", "av_mp4" };
	codec_list[31] = { "x265", "slower", "main", "mp4", "av_mp4" };
	codec_list[32] = { "x265", "slow", "main", "mp4", "av_mp4" };
	// Codecs 40-49 - x265_10bit
	codec_list[40] = { "x265_10bit", "veryslow", "main10", "mp4", "av_mp4" };
	codec_list[41] = { "x265_10bit", "slower", "main10", "mp4", "av_mp4" };
	codec_list[42] = { "x265_10bit", "slow", "main10", "mp4", "av_mp4" };
	// Codecs 50-59 - x265_12bit	
	codec_list[50] = { "x265_12bit", "veryslow", "main12", "mp4", "av_mp4" };
	codec_list[51] = { "x265_12bit", "slower", "main12", "mp4", "av_mp4" };
	codec_list[52] = { "x265_12bit", "slow", "main12", "mp4", "av_mp4" };
	// Codecs 60-69 - nvenc_h265
	codec_list[60] = { "nvenc_h265", "slow", "main", "mp4", "av_mp4" };
	codec_list[61] = { "nvenc_h265", "hq", "main", "mp4", "av_mp4" };
	// Codecs 70-79 - VP8
	codec_list[70] = { "VP8", "veryslow", "high", "mkv", "av_mkv" };
	codec_list[71] = { "VP8", "slower", "high", "mkv", "av_mkv" };
	codec_list[72] = { "VP8", "slow", "high", "mkv", "av_mkv" };
	// Codecs 80-89 - VP9
	codec_list[80] = { "VP9", "veryslow", "high", "mkv", "av_mkv" };
	codec_list[81] = { "VP9", "slower", "high", "mkv", "av_mkv" };
	codec_list[82] = { "VP9", "slow", "high", "mkv", "av_mkv" };
	// Codecs 90-99 - theora
	codec_list[90] = { "theora", "", "", "mkv", "av_mkv" };
	// Codecs 100-109 - mpeg4
	codec_list[100] = { "mpeg4", "", "", "mp4", "av_mp4" };
	// Codecs 110-119 - mpeg2
	codec_list[110] = { "mpeg2", "", "", "mp4", "av_mp4" };

	BOOST_LOG_TRIVIAL(debug) << "Size of codec_list = " << codec_list.size();


	// Shrink the two dimensional vector to minimum size.
	// And remove empty entries.

	int loop;
	int sizeofvector;

	sizeofvector = codec_list.size();

	BOOST_LOG_TRIVIAL(debug) << "Shrinking the two dimensional vector.";

	for (loop = 0; loop <= sizeofvector - 1; loop = loop + 1) {
	//	BOOST_LOG_TRIVIAL(debug) << "codec_list entry #" << loop << " size is: " << codec_list[loop].size();
	//	BOOST_LOG_TRIVIAL(debug) << "codec_list entry #" << loop << " empty is: " << codec_list[loop].empty();
		if (codec_list[loop].empty())
		{
	//		BOOST_LOG_TRIVIAL(debug) << "Removing codec_list entry #" << loop;
			codec_list.erase(codec_list.begin() + loop);
			if (loop < static_cast<int>(codec_list.size()) )
			{
				loop = loop - 1;
			//	BOOST_LOG_TRIVIAL(debug) << "Decrementing Loop.";
			}
			else
			{
				loop = sizeofvector;
			//	BOOST_LOG_TRIVIAL(debug) << "Got to the end, exiting.";
			}
		}

	}

	sizeofvector = codec_list.size();
	BOOST_LOG_TRIVIAL(debug) << "Compressed codec_list from 120 to " << sizeofvector;

	for (loop = 0; loop <= sizeofvector - 1; loop = loop + 1) {
	//	BOOST_LOG_TRIVIAL(debug) << "codec_list entry #" << loop << " is: " << codec_list[loop][0];
	}

	int MaxWidth;
	int QStart = 16;  // Start of Quality range; 0 is lossless/placebo; 51 is the worst. 
	int QEnd = 24;    // End of Quality range; 0 is lossless/placebo; 51 is the worst.
	int loop_enc = 0;
	int loop_q = 0;
	int max_enc = 0;

	BOOST_LOG_TRIVIAL(debug) << "OutputDir = " << OutputDir;
//	BOOST_LOG_TRIVIAL(debug) << "Length = " << argc;

//  Get the current time to name the working directory.  Using local timezone.
	boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

	std::wstring ws(FormatTime(now));
	BOOST_LOG_TRIVIAL(debug) << "Date_Time = " << ws;

	const std::string date_time(ws.begin(), ws.end());

	OutputDir = OutputDir + date_time;
	BOOST_LOG_TRIVIAL(debug) << "New Output Directory = " << OutputDir;

	boost::filesystem::path data_dir(OutputDir);
	BOOST_LOG_TRIVIAL(debug) << "Directory Exists? " << boost::filesystem::exists(data_dir);

	boost::filesystem::create_directory(data_dir);

	// Check to see if a file was dragged and then set the MaxWidth based on the extension.

	std::experimental::filesystem::path p;
	std::string fn;
	std::string filename;
	std::string fullfilename;
	
	BOOST_LOG_TRIVIAL(debug) << "Number of files dropped (2 = 1 file, etc.) (argc) = " << argc;

	if (argc > 1)
	{
		fullfilename = argv[1];
		std::experimental::filesystem::path p(argv[1]);
		BOOST_LOG_TRIVIAL(debug) << "Source File!";
		fn = p.stem().string();
		filename = p.filename().string();
	} else {
		fullfilename = OutputDir + "\\bad.txt";
		BOOST_LOG_TRIVIAL(debug) << "No Source File!";
		fn = "NoSourceFile";
		filename = "NoSourceFile.iso";
	}

	BOOST_LOG_TRIVIAL(debug) << "Full Filename = " << filename;
	BOOST_LOG_TRIVIAL(debug) << "Filename No Ext = " << fn;

	std::string f_ext = filename.substr(filename.find_last_of(".") + 1);

	BOOST_LOG_TRIVIAL(debug) << "Extension Portion = " << f_ext;

	if (f_ext == "iso")
	{
		MaxWidth = 1280;
	}
	else {
		MaxWidth = 1920;
	}

	BOOST_LOG_TRIVIAL(debug) << "MaxWidth = " << MaxWidth;
	
	Lossless_fn = OutputDir + "\\" + fn + "_lossless.mp4";

	HB_total = HB_start + " " +
		HB_GeneralOptions + " " +
		HB_SourceOptions + " " + HB_Lossless_Chapter + " " +
		HB_DestinationOptions + " " +
		HB_VideoOptions + " " + HB_Lossless_Encode + " " +
		HB_AudioOptions + " " +
		HB_PictureOptions + " --maxWidth " + std::to_string(MaxWidth) + " " +
		HB_FiltersOptions + " " +
		HB_SubtitlesOptions + " " +
		"-i " + fullfilename + " " +
		"-o " + Lossless_fn + " " +
		"2> " + OutputDir + "\\" + fn + "_lossless.log";

//	BOOST_LOG_TRIVIAL(debug) << "The HB command is: " << HB_total;

	if (argc > 1) {
		// Call with System
		system(HB_total.c_str());
	}
	else {
		// Just echo to the screen
		BOOST_LOG_TRIVIAL(debug) << "No file to use.";
	}

	max_enc = codec_list.size();

//	system("pause");

	int counter = 0;
	int totalcount = max_enc * (QEnd - QStart + 1);  // Will always loop through once.
	std::array<char, 256> buffer;
	std::string result;

// Open the output file
	HandbrakeDataFile = OutputDir + "\\" + HandbrakeDataFile + date_time + ".csv";
	BOOST_LOG_TRIVIAL(debug) << "Opening the CSV output file:  " << HandbrakeDataFile << std::endl;

	std::ofstream csvfile;
	csvfile.open(HandbrakeDataFile);
	csvfile << "Handbrake Video Settings Comparison Tool\n";
	csvfile << "\n";
	csvfile << "Source File: " << fullfilename << std::endl;
	csvfile << "Run#,CODEC,Preset,Profile,DestExt,Container,Quality,SSIM(dB),SSIM,PSNR,Total Seconds,File Size,DateTime\n";

	cout << "Handbrake Video Settings Comparison Tool\n";
	cout << std::endl;
	cout << "Source File: " << fullfilename << std::endl;
	cout << "Destination Directory:  " << OutputDir << std::endl;
	cout << "FFmpeg.exe Path:  " << ffmpeg_path << std::endl;
	cout << "CSV Data File:  " << HandbrakeDataFile << std::endl;
	
	cout << "Quality Range:  From - " << QStart << " to " << QEnd << std::endl;
	cout << "Number of different encoder selections:  " << max_enc << std::endl;
	cout << "Total number of iterations:  " << totalcount << std::endl;
	cout << std::endl;
	cout << "Run#,CODEC,Preset,Profile,DestExt,Container,Quality,SSIM(dB),SSIM,PSNR,Total Seconds,File Size,DateTime\n";

	for (loop_enc = 0; loop_enc < max_enc; loop_enc = loop_enc + 1) {

	// Start to loop through the Quality settings

		for (loop_q = QStart; loop_q <= QEnd; loop_q = loop_q + 1) {
			
		//  Keep track of how many loops and as an index in the CSV file.
			counter = counter + 1;

		//  Grab current Date/Time for reference in the CSV file.

		//  current date/time based on current system
			time_t now = time(0);

		//  convert now to string form
		//	char* dt = ctime_s(&now);
			char dt[26];
			errno_t err = ctime_s(dt, 26, &now);

		//  BOOST_LOG_TRIVIAL(debug) << "The local date and time is: " << dt << endl;

			time_t curr_time;
			struct tm timeinfo;
			char dt_string[100];
			time(&curr_time);
			localtime_s(&timeinfo, &curr_time);

			strftime(dt_string, 50, "%Y%m%d_%H%M", &timeinfo);
			BOOST_LOG_TRIVIAL(debug) << "Date and Time for File = " << dt_string;

			OutputFile = OutputDir + "\\" + fn + "_" + codec_list[loop_enc][0] + "_" + "q" + std::to_string(loop_q) + "_" +
				codec_list[loop_enc][1] +"_" + codec_list[loop_enc][2] + 
				"." + codec_list[loop_enc][3];
			BOOST_LOG_TRIVIAL(debug) << "Output File: " << OutputFile;

			OutputLog = OutputDir + "\\" + fn + "_" + codec_list[loop_enc][0] + "_" + "q" + std::to_string(loop_q) + "_" +
				codec_list[loop_enc][1] + "_" + codec_list[loop_enc][2] +
				"_log.txt";
			BOOST_LOG_TRIVIAL(debug) << "Output Log File: " << OutputLog;

			HB_total = HB_start + " " +
				HB_GeneralOptions + " " +
				HB_SourceOptions + " " + 
				HB_DestinationOptions + " " + 
				"--format " + codec_list[loop_enc][4] + " " +
				HB_VideoOptions + " " + 
				"--encoder " + codec_list[loop_enc][0] + " " +
				"--encoder-preset " + codec_list[loop_enc][1] + " " +
				"--encoder-profile " + codec_list[loop_enc][2] + " " +
				"--quality " + std::to_string(loop_q) + " " +
				HB_AudioOptions + " " +
				HB_PictureOptions + " --maxWidth " + std::to_string(MaxWidth) + " " +
				HB_FiltersOptions + " " +
				HB_SubtitlesOptions + " " +
				"-i " + Lossless_fn + " " +
				"-o " + OutputFile + " " +
				"2> " + OutputLog;

			BOOST_LOG_TRIVIAL(debug) << "Loop Encoder = " << loop_enc;
			BOOST_LOG_TRIVIAL(debug) << "Loop Quality = " << loop_q;
			BOOST_LOG_TRIVIAL(debug) << "HB command = " << HB_total;
			
			// Start to time the Handbrake Encode
			auto start = std::chrono::system_clock::now();

			if (argc > 1) {
				// Call with System
				system(HB_total.c_str());
			}
			else {
				// Just echo to the screen
				BOOST_LOG_TRIVIAL(debug) << "No file to use.";
			}
			
			// Stop the timer, encode is finished.
			auto end = std::chrono::system_clock::now();

			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

			BOOST_LOG_TRIVIAL(debug) << "Elapsed time: " << elapsed_seconds.count() << "s";

			// Now run SSIM against it to get video compression quality metrics

			// Output for using two differently compressed files:
			//[Parsed_ssim_0 @ 0000021d52b99c00] SSIM Y:0.999472 (32.777502) U:0.999448 (32.579794) V:0.999451 (32.603229) All:0.999465 (32.714842)
			//[Parsed_psnr_1 @ 0000021d51a1a200] PSNR y:62.310421 u:62.821236 v:62.802943 average:62.471287 min:59.999097 max:65.930472

			// Output for identical files to show that they are identical:
			//[Parsed_ssim_0 @ 0000026c324fef00] SSIM Y:1.000000 (inf) U:1.000000 (inf) V:1.000000 (inf) All:1.000000 (inf)
			//[Parsed_psnr_1 @ 0000026c32579600] PSNR y:inf u:inf v:inf average:inf min:inf max:inf

			// https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/

			ssim_call = ffmpeg_path + " " +
				"-i " + OutputFile + " " +  // This is the file you are comparing
				"-i " + Lossless_fn + " " +  // This is the reference/lossless file
				"-lavfi \"ssim; [0:v][1:v]psnr\" -f null /dev/null 2>&1 | findstr /c:\"All:\" /c:\"average:\"";

//			BOOST_LOG_TRIVIAL(debug) << "The ssim_call = " << ssim_call;

			result = "";

			if (argc > 1) {
				// Call with System
				FILE* pipe = _popen(ssim_call.c_str(), "r");
				if (!pipe)
				{
					BOOST_LOG_TRIVIAL(debug) << "Couldn't start the ssim command.";
					return 0;
				}
				while (fgets(buffer.data(), 256, pipe) != NULL) {
					BOOST_LOG_TRIVIAL(debug) << "Reading...";
					result += buffer.data();
				}
				auto returnCode = _pclose(pipe);
				BOOST_LOG_TRIVIAL(debug) << "Result =" << result;
				BOOST_LOG_TRIVIAL(debug) << "returnCode = " << returnCode;
			}
			else {
				// Just echo to the screen
				BOOST_LOG_TRIVIAL(debug) << "No file to use.";
			}

			// Pull out the SSIM in dB and absolute value (values after "All:")
			// Pull out the average PSNR
			
			auto end_file = "0";
			
			if (argc > 1) {
				pos = 0;
				pos = result.find("All:");
				ssim_ratio = result.substr(pos + 4, result.npos); // Grab to end of the string
				pos = ssim_ratio.find("(");
				ssim_db = ssim_ratio.substr(pos + 1, ssim_db.npos);
				pos = ssim_db.find(")");
				ssim_db = ssim_db.substr(0, pos + 1);
				ssim_db = ssim_db.substr(0, ssim_db.size() - 1);
				ssim_ratio = ssim_ratio.substr(0, 8);
				BOOST_LOG_TRIVIAL(debug) << "ssim_ratio = ";
				BOOST_LOG_TRIVIAL(debug) << "ssim_db = ";

				pos = result.find("average:");
				psnr_ratio = result.substr(pos + 8, result.npos); // Grab to end of the string
				pos = psnr_ratio.find(" ");
				psnr_ratio = psnr_ratio.substr(0, pos);
				BOOST_LOG_TRIVIAL(debug) << "psnr_ratio = " << psnr_ratio;

				// Pull the encoded file output size.

				ifstream resultfile(OutputFile);
				resultfile.seekg(0, ios::end);
				auto end_file = resultfile.tellg();
				resultfile.close();
				BOOST_LOG_TRIVIAL(debug) << "Size of the encoded file is: " << end_file << " bytes.";
			}
			else {
				BOOST_LOG_TRIVIAL(debug) << "No file used.  Substituting fake values.";
				ssim_ratio = "N/A";
				ssim_db = "N/A";
				psnr_ratio = "N/A";
				auto end_file = "0";
			}
			
			// Ouput all the data to the CSV file and log to the screen.

//			csvfile << "Run#,CODEC,Preset,Profile,DestExt,Container,Quality,SSIM(dB),SSIM,PSNR.Total Seconds,File Size,DateTime\n";
			csvfile << counter << "," << codec_list[loop_enc][0] << "," << codec_list[loop_enc][1]
				<< "," << codec_list[loop_enc][2] << "," << codec_list[loop_enc][3]
				<< "," << codec_list[loop_enc][4] << "," << loop_q
				<< "," << ssim_db << "," << ssim_ratio << "," << psnr_ratio
				<< "," << elapsed_seconds.count() << "," << end_file << "," << dt_string << "\n";
			std::cout << counter << "," << codec_list[loop_enc][0] << "," << codec_list[loop_enc][1]
				<< "," << codec_list[loop_enc][2] << "," << codec_list[loop_enc][3]
				<< "," << codec_list[loop_enc][4] << "," << loop_q
				<< "," << ssim_db << "," << ssim_ratio << "," << psnr_ratio
				<< "," << elapsed_seconds.count() << "," << end_file << "," << dt_string << std::endl;
		/*	BOOST_LOG_TRIVIAL(debug) << counter << "," << codec_list[loop_enc][0] << "," << codec_list[loop_enc][1]
				<< "," << codec_list[loop_enc][2] << "," << codec_list[loop_enc][3]
				<< "," << codec_list[loop_enc][4] << "," << loop_q
				<< "," << ssim_db << "," << ssim_ratio << "," << psnr_ratio
				<< "," << elapsed_seconds.count() << "," << end_file << "," << dt_string;
		*/
		}
	
	}
	
	csvfile.close();

	system("pause");
}


// Valid CODEC Combinations
/*
Encoder Tables(best viewed in monpspaced font

Encoder Presets

Preset		x264	x264_10bit	nvenc_h264	mpeg4	mpeg2	x265	x265_10bit	x265_12bit	nvenc_h265	VP8	VP9	theora
default                              x                                                           x
placebo	      x          x                                    x         x           x
veryslow      x          x                                    x         x           x                    x   x
slower        x          x                                    x         x           x                    x   x
slow          x          x           x                        x         x           x            x       x   x
medium        x          x           x                        x         x           x            x       x   x
fast          x          x           x                        x         x           x            x       x   x
faster        x          x                                    x         x           x                    x   x
veryfast      x          x                                    x         x           x                    x   x
superfast     x          x                                    x         x           x
ultrafast     x          x                                    x         x           x
highperformance(hp)                  x                                                           x
highquality(hq)                      x                                                           x

fastdecode    x          x                                    x         x           x

Profile	x264	x264_10bit	nvenc_h264	mpeg4	mpeg2	x265	x265_10bit	x265_12bit	nvenc_h265	VP8	VP9	theora
auto	  x          x           x                        x                                  x
baseline  x                      x
main      x                      x                        x                                  x
mainstillpicture                                          x
main10                                                              x
main10 - intra                                                      x
main12                                                                          x
main12 - intra                                                                  x
high      x                      x
high10              x

Level		x264	x264_10bit	nvenc_h264	mpeg4	mpeg2	x265	x265_10bit	x265_12bit	nvenc_h265	VP8	VP9	theora
auto          x          x           x                                                           x
1.0           x          x           x                                                           x
1b            x          x           x
1.1           x          x           x
1.2           x          x           x
1.3           x          x           x
2.0           x          x           x                                                           x
2.1           x          x           x                                                           x
2.2           x          x           x
3.0           x          x           x                                                           x
3.1           x          x           x                                                           x
3.2           x          x           x
4.0           x          x           x                                                           x
4.1           x          x           x                                                           x
4.2           x          x           x
5.0           x          x           x                                                           x
5.1           x          x           x                                                           x
5.2           x          x           x                                                           x
6.0                                                                                              x
6.1                                                                                              x
6.2                                                                                              x

*/


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
