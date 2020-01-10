#include <iostream>
#include <cgv/utils/convert.h>
//spider includes
#include <artec/sdk/capturing/IScanner.h>
#include <artec/sdk/capturing/IArrayScannerId.h>
#include <artec/sdk/capturing/IFrameProcessor.h>
#include <artec/sdk/capturing/IFrame.h>
#include <artec/sdk/base/BaseSdkDefines.h>
#include <artec/sdk/base/Log.h>
#include <artec/sdk/base/io/ObjIO.h>
#include <artec/sdk/base/IFrameMesh.h>



#include "rgbd_spider.h"

using namespace std;

namespace asdk {
	using namespace artec::sdk::base;
	using namespace artec::sdk::capturing;
};
using asdk::TRef;
using asdk::TArrayRef;

namespace rgbd {


	rgbd_spider::rgbd_spider() {
		serial = "";
	}

	rgbd_spider::~rgbd_spider() {
		
	}

	bool rgbd_spider::attach(const std::string& serial)
	{
		if (is_attached()) {
			detach();
		}
		

		asdk::IArrayScannerId* scannerList;

		asdk::ErrorCode ec = asdk::enumerateScanners(&scannerList);

		if (ec != asdk::ErrorCode_OK)
		{
			std::cout << "rgbd_spider_driver: something went wrong\n";
			return 0;
		}
		asdk::ScannerId id;
		//const asdk::ScannerId* id = scanner->getId();
		int list_size = scannerList->getSize();
		//find the camera with the same id
		for (int i = 0; i < scannerList->getSize();++i) {
			asdk::ScannerId &scannerId = scannerList->getPointer()[i];
			if (cgv::utils::wstr2str(scannerId.serial).compare(serial) == 0) {
				asdk::ErrorCode ec = asdk::createScanner(&scanner, &scannerId);
				if (ec != asdk::ErrorCode_OK)
				{
					std::cerr << "rgbd_spider::attach: errors ocurred while creating scanner object for scanner with serial " << serial << endl;
					scanner.release();
					return false;
				}
				this->serial = serial;
				return true;
			}
		}
		return false;
	}

	bool rgbd_spider::is_attached() const
	{
		return serial != "";
	}

	bool rgbd_spider::detach()
	{
		if (this->is_attached()) {
			scanner.release();
			serial = "";
			return true;
		}
		return false;
	}
	
	bool rgbd_spider::check_input_stream_configuration(InputStreams is) const
	{
		static const unsigned streams_avaiable = IS_COLOR | IS_DEPTH | IS_INFRARED, streams_in = is;
		return (~(~streams_in | streams_avaiable)) == 0;
	}

	bool rgbd_spider::start_device(InputStreams is, std::vector<stream_format>& stream_formats)
	{
		if (!check_input_stream_configuration(is)) {
			cerr << "rgbd_spider::start_device : invalid stream configuration\n";
			return false;
		}
		if (!is_attached()) {
			cerr << "rgbd_spider::start_device : tried to start unattached device\n";
			return false;
		}
		if (is_running()) {
			return true;
		}

		
		if (is && IS_COLOR) {
			stream_formats.push_back(color_stream = stream_format(640, 480,PF_BGR, 30, 24));
		}
		if (is && IS_DEPTH) {
			stream_formats.push_back(depth_stream = stream_format(640, 480, PF_DEPTH, 30, 16));
		}
		if (is && IS_INFRARED) {
			stream_formats.push_back(ir_stream = stream_format(640, 480, PF_I, 30, 8));
		}
		return this->start_device(stream_formats);
	}

	bool rgbd_spider::start_device(const std::vector<stream_format>& stream_formats)
	{
		if (!is_attached()) {
			cerr << "rgbd_spider::start_device : tried to start unattached device\n";
			return false;
		}
		if (is_running()) {
			return true;
		}

		return false;
	}

	bool rgbd_spider::stop_device()
	{
		return false;
	}

	bool rgbd_spider::is_running() const
	{
		return false;
	}

	bool rgbd_spider::get_frame(InputStreams is, frame_type& frame, int timeOut)
	{
		if (!is_running()) {
			cerr << "rgbd_spider::get_frame called on device that is not running" << endl;
			return false;
		}
		if (!check_input_stream_configuration(is)) {
			cerr << "rgbd_spider::get_frame called with an invalid input stream configuration" << endl;
			return false;
		}
		return false;
	}

	void rgbd_spider::map_color_to_depth(const frame_type& depth_frame, const frame_type& color_frame, frame_type& warped_color_frame) const
	{

	}

	bool rgbd_spider::map_depth_to_point(int x, int y, int depth, float* point_ptr) const
	{
		return false;
	}

	void rgbd_spider::query_stream_formats(InputStreams is, std::vector<stream_format>& stream_formats) const
	{
		if (!is_attached()) {
			std::cerr << "rgbd_realsense::query_stream_formats:  device is not attached!\n";
			return;
		}


	}


	rgbd_spider_driver::rgbd_spider_driver() {
		
	}

	rgbd_spider_driver::~rgbd_spider_driver() {
		
	}

	unsigned rgbd_spider_driver::get_nr_devices() {
		asdk::IArrayScannerId* scannerList;

		asdk::ErrorCode ec = asdk::enumerateScanners(&scannerList);

		if (ec != asdk::ErrorCode_OK)
		{
			std::cout << "rgbd_spider_driver: something went wrong\n";
			return 0;
		}
		return scannerList->getSize();
	}

	std::string rgbd_spider_driver::get_serial(int i) {
		asdk::IArrayScannerId* scannerList;
		
		asdk::ErrorCode ec = asdk::enumerateScanners(&scannerList);

		if (ec != asdk::ErrorCode_OK)
		{
			std::cout << "rgbd_spider_driver: something went wrong\n";
			return "";
		}
		if (i >= scannerList->getSize()) {
			std::cout << "rgbd_spider_driver::get_serial scannerList index out of bounds\n";
			return "";
		}
		return cgv::utils::wstr2str(scannerList->getPointer()[i].serial);
	}

	rgbd_device* rgbd_spider_driver::create_rgbd_device() {
		return new rgbd_spider();
	}
}
#include "lib_begin.h"

extern CGV_API rgbd::driver_registration<rgbd::rgbd_spider_driver> rgbd_spider_driver_registration("spider_driver");