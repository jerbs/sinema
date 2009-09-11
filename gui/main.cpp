#ifndef SYNCTEST
#include "player/MediaPlayer.hpp"
#else
#include "player/SyncTest.hpp"
#endif

#include <iostream>

int main(int argc, char *argv[])
{
#ifndef SYNCTEST

    if (argc != 2)
    {
	std::cout << "Usage: " << argv[0] << " <file>" << std::endl;
	exit(-1);
    }

    MediaPlayer mediaPlayer;
    mediaPlayer(argv[1]);

#else

    SyncTestApp syncTestApp;
    syncTestApp();
    
#endif


    return 0;
}
