/**********************************************************************
 *
 * Filename:    test.cpp
 * 
 * Description: Example of how to use the videoServer. I wanted it to be super
 * 			simple and only require you to use one or two lines
 *
 * Notes:
 *
 * 
 * Copyright (c) 2018 by Kelly Klein.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include <videoServer.h>
#include <iostream>

int main(int argc, char *argv[]){
	VideoServer *server = new VideoServer("config/server.conf");

	std::cout << "made server. now listening" << std::endl;
	server->run();

	return 0;
}
