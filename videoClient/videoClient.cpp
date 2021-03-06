/**********************************************************************
 *
 * Filename:    videoClient.cpp
 * 
 * Description: Implementations of videoClient class
 *
 * Notes:
 *
 * 
 * Copyright (c) 2018 by Kelly Klein.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include "videoClient.h"

using namespace videoStreamer;

VideoClient::VideoClient(std::string configFile/*,bool input*/){
	gst_init(NULL,NULL);

	GstDebugLevel dbglevel = gst_debug_get_default_threshold();
	dbglevel = GST_LEVEL_INFO;
	//terminalInput = input;
	ConfigReader reader(configFile);
	if(!reader.exists()){
        	std::cout << "no config file found using defaults" << std::endl;
        	server = "192.168.2.2";

	}else{
	        server = reader.find("server");
	}
	connected = false;
	running = true;
	std::cout << "server address is " << server << std::endl;
	control = new Socket(CONTROL_CLIENT_PORT);
	//heartbeat = new Socket(HEARTBEAT_CLIENT_PORT);
}

void VideoClient::run(){	
	std::cout << "connecting..." << std::endl;
	Message init;
	init.type = INIT;
	init.length = 1;
	init.data = (uint8_t *)malloc(1);

	//initally blocks until the client manages to connect to the server
	while(!connected){
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> span = t2 - t1;
		control->write(&init,server,CONTROL_PORT);
		std::cout << "init crc: " << (int)init.crc << std::endl;
		while(span.count() < 1000){
			if(control->available()){
				Message message;
				std::string sender;
				int port;
				control->getMessage(&message,sender,port);
				onMessage(message);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			t2 = std::chrono::high_resolution_clock::now();
			span = t2 - t1;
		}
	}

	//now launches this into the background and returns.
	messageThread = new std::thread([this](){
		while(running){
			/*while(!connected){
				std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
				std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> span = t2 - t1;
				control->write(&init,server,CONTROL_PORT);
				while(span.count() < 1000){
					if(control->available()){
						Message message;
						std::string sender;
						int port;
						control->getMessage(&message,sender,port);
						onMessage(message);
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					t2 = std::chrono::high_resolution_clock::now();
					span = t2 - t1;
				}
			}*/

			std::chrono::high_resolution_clock::time_point lagT1 = std::chrono::high_resolution_clock::now();
			Message message;
			std::string sender;
			int port;
			while(connected){
				if(control->available()){
					if(control->getMessage(&message,sender,port)){
						onMessage(message);
					}else{
						std::cout << "nacking message " << message.type << std::endl;
						std::cout << "length: " << message.length << std::endl;
						std::string data = std::string((char *)message.data);
						std::cout << data << std::endl;
						Message nack;
						nack.type = NACK;
						nack.length = 1;
						nack.data = (uint8_t *)malloc(1);
						control->write(&nack,server,CONTROL_PORT);
					}
				}
				std::chrono::high_resolution_clock::time_point lagT2 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> time = lagT2 - lagT1;
				if(time.count() > 2000){
					double lag = control->checkLag(10, server, CONTROL_PORT, CONTROL_CLIENT_PORT);
					for(int i = 0; i < devices.size(); i++){
						if(devices[i].playing){
							GstClockTime time;
							devices[i].lastTimestamp = time;
							g_object_set(devices[i].overlay, "text", std::string("lag time: " + std::to_string(lag) + "ms").c_str(), NULL);
						}
					}
					lagT1 = std::chrono::high_resolution_clock::now();
				}else{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
}

void VideoClient::onMessage(Message message){
	Message responce;
	switch(message.type){
	case INIT:{
		responce.type = (MessageType)STATUS;
		responce.length = 1;
		responce.data = (uint8_t *)malloc(1);
		connected = true;
		std::cout << "Connected to server" << std::endl;
	break;
	}
	case STATUS:{
		responce.type = ACK;
		responce.length = 1;
		responce.data = (uint8_t *)malloc(1);
		//std::cout << "\n\navailable devices:" << std::endl;
		//std::cout << std::endl;
		for(int i = 0; i < message.length;){
			ClientDevice device;
			std::string tmp;
			tmp += message.data[i+1];
			tmp += message.data[i+2];
			tmp += message.data[i+3];
			tmp += message.data[i+4];
			device.port = std::stoi(tmp);
			tmp = message.data[i+5];
			device.deviceType = (DeviceType)std::stoi(tmp);
			tmp = message.data[i+6];
			tmp += message.data[i+7];
			int length = std::stoi(tmp);
			tmp = "";
			for(int j = 0; j < length; j++){
				device.name += message.data[j+8+i];
			}
			device.server = server;
			//std::cout << device.name + "\t\tenabled";
			/*if(device.deviceType == AUDIO){
				std::cout << "\t\taudio device" << std::endl;
			}else if(device.deviceType == VIDEO){
				std::cout << "\t\tvideo device" << std::endl;
			}else if(device.deviceType == RASPI){
				std::cout << "\t\traspberry pi device" << std::endl;
			}*/
			i += 8 + length;
			device.playing = false;
			device.lastTimestamp = 0;
			devices.push_back(device);
		}
	break;
	}
	case STARTED:{
		std::cout << devices[message.data[CAMERA_INDEX]].name << " stream started" << std::endl;
		startDevice(message.data[CAMERA_INDEX]);
		responce.type = ACK;
		responce.length = 1;
		responce.data = (uint8_t *)malloc(1);
	break;
	}
	case STOPPED:{
		std::cout << devices[message.data[CAMERA_INDEX]].name << " stream stopped" << std::endl;
		stopDevice(message.data[CAMERA_INDEX]);
		responce.type = ACK;
		responce.length = 1;
		responce.data = (uint8_t *)malloc(1);
		stopDevice(message.data[CAMERA_INDEX]);
	break;
	}
	};

	control->write(&responce,server,CONTROL_PORT);
}

void VideoClient::setRecordingLocation(std::string loc){
	recordingLocation = loc;
}

void VideoClient::startDevice(int deviceIndex){
	ClientDevice &device = devices[deviceIndex];
	if(device.deviceType == AUDIO){
		std::string binStr = "udpsrc port=" + std::to_string(device.port) + " caps=\"application/x-rtp\" ! queue ! rtppcmudepay ! mulawdec ! audioconvert ! alsasink";
                device.pipeline = gst_parse_launch(binStr.c_str(),NULL);
		device.bus = gst_element_get_bus(device.pipeline);
		gst_bus_add_signal_watch(device.bus);
                gst_element_set_state(device.pipeline,GST_STATE_PLAYING);
		device.playing = true;
		return;
	}
	device.source = gst_element_factory_make("udpsrc",NULL);
	g_object_set(G_OBJECT(device.source),"port",device.port,NULL);
	device.caps = gst_caps_new_simple("application/x-rtp",
		"media",G_TYPE_STRING,"video",
		"framerate",GST_TYPE_FRACTION,framerate,1,
		"encoding-name",G_TYPE_STRING,"H264",
		"payload",G_TYPE_INT,"96",
		NULL);

	//amd/intel
	/*g_object_set(device.source,"caps",device.caps,NULL);
	device.depay = gst_element_factory_make("rtph264depay",NULL);
	device.decodeBin = gst_element_factory_make("vaapidecodebin",NULL);
	g_object_set(device.decodeBin,"max-size-buffers",100,NULL);
	device.overlay = gst_element_factory_make("textoverlay",NULL);
	g_object_set(device.overlay,"text","lag: 0ms","valignment",2,"halignment",2,"shaded-background",true,NULL);
	device.sink = gst_element_factory_make("vaapisink",NULL);*/

	//nvidia
	g_object_set(device.source,"caps",device.caps,NULL);
	device.depay = gst_element_factory_make("rtph264depay",NULL);
	device.decodeBin = gst_element_factory_make("avdec_h264",NULL);
	device.overlay = gst_element_factory_make("textoverlay",NULL);
	g_object_set(device.overlay,"text","lag: 0ms","valignment",2,"halignment",2,"shaded-background",true,NULL);
	device.sink = gst_element_factory_make("xvimagesink",NULL);

	if(!device.source || !device.depay || !device.decodeBin || !device.overlay || !device.sink){
		std::cout << "failed to create elements" << std::endl;
		exit(-1);
	}

	device.pipeline = gst_pipeline_new(std::string(device.name + "-pipeline").c_str());
	gst_bin_add_many(GST_BIN(device.pipeline), device.source, device.depay, device.decodeBin, device.overlay, device.sink, NULL);

	for(int i = 0; i < device.displays.size(); i++){
		if(!device.displays[i]){
			std::cout << "failed to create display at: " << i << std::endl;
		}
		gst_bin_add(GST_BIN(device.pipeline),device.displays[i]);
	}

	/*
		testbin
		gst-launch-1.0 udpsrc port=4444 caps ="application/x-rtp, media=(string)video, framerate=30/1, encoding-name=(string)H264, payload=(int)96" ! rtph264depay ! vaapidecodebin threads=8 ! x264enc ! filesink location=test.mp4
		maybe need a parser? h264parse

		gst-launch-1.0 udpsrc port=4444 caps ="application/x-rtp, media=(string)video, framerate=30/1, encoding-name=(string)H264, payload=(int)96" ! rtph264depay ! vaapidecodebin threads=8 ! vaapisink

		gst-launch-1.0 -e v4l2src device=/dev/video0 ! videoscale method=0 ! videorate drop-only=true ! videoconvert ! video/x-raw,format=I420,width=1920,height=1080,framerate=30/1 ! tee name=t ! queue ! x264enc ! filesink location=test.mp4 async=0 t. ! queue ! vaapisink
	*/
	if(recordingLocation != ""){
		device.tee = gst_element_factory_make("tee","tee");
		device.recordingDecodeBin = gst_element_factory_make("vaapidecodebin",NULL);
		device.recordingEncoder = gst_element_factory_make("x264enc",NULL);
		device.displayQueue = gst_element_factory_make("queue",NULL);
		device.recordQueue = gst_element_factory_make("queue",NULL);
		device.recordingMux = gst_element_factory_make("mp4mux",NULL);
		device.recordingSink = gst_element_factory_make("filesink",NULL);
		g_object_set(device.recordingSink,"location","test.mp4", "async", "0",NULL);

		if(!device.recordingSink || !device.recordingDecodeBin || !device.recordingEncoder || !device.tee || !device.recordingMux || !device.displayQueue || !device.recordQueue){
			std::cout << "failed to create recording sink" << std::endl;
		}

		//if recording location, do recording pipelines
		gst_bin_add_many(GST_BIN(device.pipeline), device.tee, device.displayQueue, device.recordQueue, device.recordingDecodeBin, device.recordingEncoder, device.recordingMux, device.recordingSink, NULL);
		if(!gst_element_link_many(device.source, device.depay, device.tee, NULL)){
			std::cout << "failed to link tee sink" << std::endl;
		}
		if(!gst_element_link_many(device.tee, device.recordQueue, device.recordingDecodeBin, device.recordingEncoder, device.recordingMux, device.recordingSink, NULL)){
			std::cout << "failed to link file sink" << std::endl;
		}
		if(!gst_element_link_many(device.tee, device.displayQueue, device.decodeBin, device.overlay, device.sink, NULL)){
			std::cout << "failed to link devices" << std::endl;
			exit(-1);
		}
	}else{
		if(!gst_element_link_many(device.source, device.depay, device.decodeBin, device.overlay, device.sink, NULL)){
			std::cout << "failed to link devices" << std::endl;
			exit(-1);
		}
		for(int i = 0; i < device.displays.size(); i++){
			if(!gst_element_link(device.source,device.displays[i])){
				std::cout << "failed to link device at: " << i << std::endl;
				exit(-1);
			}
		}
	}

	device.bus = gst_element_get_bus(device.pipeline);
	gst_bus_add_signal_watch(device.bus);
	gst_element_set_state(device.pipeline,GST_STATE_PLAYING);
	device.playing = true;
}

void VideoClient::stopDevice(int deviceIndex){
	if(devices[deviceIndex].playing){
		ClientDevice &device = devices[deviceIndex];
		device.playing = false;
		gst_element_set_state(device.pipeline,GST_STATE_NULL);
		gst_object_unref(device.pipeline);
		gst_object_unref(device.bus);
	}
}

bool VideoClient::requestStart(int index){
	if(index < devices.size()){
		Message message;
		message.type = START;
		message.length = 1;
		message.data = (uint8_t *)malloc(1);
		message.data[CAMERA_INDEX] = index;
		control->write(&message,server,CONTROL_PORT);
	}

	return false;
}

bool VideoClient::requestStop(int index){
	if(index < devices.size()){
		Message message;
		message.type = STOP;
		message.length = 1;
		message.data = (uint8_t *)malloc(1);
		message.data[CAMERA_INDEX] = index;
		control->write(&message,server,CONTROL_PORT);

		//stopDevice(index);
	}

	return false;
}

bool VideoClient::requestList(std::vector<std::string> &list){
	if(devices.size() > 0){
		list.clear();
		for(int i = 0; i < devices.size(); i++){
			list.push_back(devices[i].name);
		}
		return true;
	}

	return false;
}

bool VideoClient::updateSettings(int type,int value){
	Message message;
	message.type = (MessageType)SET;
	message.length = 2;
	message.data = (uint8_t *)malloc(2);
	switch(type){
	case fps:
		message.data[0] = FRAMERATE;
		switch(value){
		case F10:
			framerate = value;
			message.data[1] = value;
		break;
		case F20:
			framerate = value;
			message.data[1] = value;
		break;
		case F30:
			framerate = value;
			message.data[1] = value;
		break;
		default:
			return false;
		break;
		};
	break;
	case resolution:
		message.data[0] = videoStreamer::RESOLUTION;
		switch(value){
		case HD:
			message.data[1] = HD;
		break;
		case HD720:
			message.data[1] = HD720;
		break;
		case SD:
			message.data[1] = SD;
		break;
		default:
			return false;
		break;
		};
	break;
	default:
		return false;
	break;
	};

	control->write(&message,server,CONTROL_PORT);
	return true;
}

int VideoClient::addDisplay(int deviceIndex,std::string initMessage){
	devices[deviceIndex].displays.push_back(gst_element_factory_make("textoverlay",NULL));
	g_object_set(devices[deviceIndex].displays.back(),"text",initMessage.c_str(),"valignment",2,"halignment",1,"shaded-background",true,NULL);

	return devices[deviceIndex].displays.size() - 1;
}

void VideoClient::update(int deviceIndex,int handler,std::string message){
	g_object_set(devices[deviceIndex].displays[handler], "text", message.c_str(), NULL);
}

bool VideoClient::isConnected(){
	return connected;
}

void VideoClient::kill(){
	Message message;
	message.type = (MessageType)QUIT;
	message.length = 1;
	message.data = (uint8_t *)malloc(1);
	control->write(&message,server,CONTROL_PORT);
	connected = false;
	//messageThread->join();
}

VideoClient::~VideoClient(){
	connected = false;
	running = false;
	messageThread->join();
	//heartbeatThread->join();
	//commandThread->join();
}
