/**********************************************************************
 *
 * Filename:    main.cpp
 * 
 * Description: Example of how to use the videoClient class
 *
 * Notes:
 *
 * 
 * Copyright (c) 2018 by Kelly Klein.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include <string>
#include <chrono>

#include "videoClient.h"
#include "videoStreamer_global.h"

using namespace videoStreamer;

VideoClient *client;
std::vector<std::string> feeds;

bool connected = false;
bool running = true;

void onCommand(std::string command){
	std::vector<std::string> commandsplit = useful::split(command,' ');
	if(commandsplit[0] == "start"){
		bool found = false;
		int feedIndex = 0;
		for(int i = 0; i < feeds.size(); i++){
			if(commandsplit[1] == feeds[i]){
				feedIndex = i;
				found = true;
			}
		}
		if(!found){
			std::cout << "could not find " << commandsplit[1] << std::endl;
			return;
		}
		client->requestStart(feedIndex);
	}else if(commandsplit[0] == "stop"){
		bool found = false;
		int feedIndex = 0;
		for(int i = 0; i < feeds.size(); i++){
			if(commandsplit[1] == feeds[i]){
				std::cout << "stopping " << feeds[i] << " stream" << std::endl;
				feedIndex = i;
				found = true;
			}
		}
		if(!found){
			std::cout << "could not find " << commandsplit[1] << std::endl;
			return;
		}
		client->requestStop(feedIndex);
	}else if(commandsplit[0] == "set"){
			int type = 0;
			int setting = 0;
		if(commandsplit[1] == "resolution"){
			if(commandsplit[2] == "1080"){
				setting = HD;
				std::cout << "setting resolution to 1080p" << std::endl;
			}else if(commandsplit[2] == "720"){
				setting = HD720;
				std::cout << "setting resolution to 720p" << std::endl;
			}else if(commandsplit[2] == "480"){
				setting = SD;
				std::cout << "setting resolution to 480p" << std::endl;
			}else{
				std::cout << "not a resolution" << std::endl;
				return;
			}
			type = resolution;
		}else if(commandsplit[1] == "framerate"){
			if(commandsplit[2] == "10"){
				setting = F10;
				std::cout << "setting framerate to 10fps" << std::endl;
			}else if(commandsplit[2] == "20"){
				setting = F20;
				std::cout << "setting framerate to 20fps" << std::endl;
			}else if(commandsplit[2] == "30"){
				setting = F30;
				std::cout << "setting framerate to 30fps" << std::endl;
			}else{
				std::cout << "not a valid frame rate" << std::endl;
				return;
			}
			type = fps;
		}else{
			std::cout << "nope, not an option" << std::endl;
			return;
		}
		client->updateSettings(type,setting);
	}else if(commandsplit[0] == "status"){
		//just repeat what devices are available
		std::cout << "VideoClient","available devices" << std::endl;
		for(std::string feed : feeds){
			std::cout << "\t" << feed << std::endl;
		}
	}else if(commandsplit[0] == "exit"){
		client->kill();
		exit(0);
	}else{
		std::cout << "not recognized brah" << std::endl;
		return;
	}

}

int main(int argc,char *argv[]){
	client = new VideoClient("/opt/project-remoteVideo/config/videoClient.conf");
	client->run();

	if(!client->requestList(feeds)){
		std::cout << "VideoClient","no video streams? server probably isnt setup right" << std::endl;
		client->kill();
		exit(-1);
	}

	std::cout << "VideoClient","available devices" << std::endl;
	for(std::string feed : feeds){
		std::cout << "\t" << feed << std::endl;
	}

	std::string command;
	std::cout << "\nCommands: \nstart \t\t\t{device name} \nstop \t\t\t{device name} \nset \tresolution \t{1080 | 720 | 480} \n\tframerate \t{10 | 20 | 30} \nstatus \nexit" << std::endl;
	while(running){
		getline(std::cin,command);
		onCommand(command);
	}

	return 0;
}
