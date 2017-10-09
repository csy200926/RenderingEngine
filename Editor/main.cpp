#include "RenderingGame.h"

#include <QtWidgets/QApplication>
#include "Editor.h"
int main(int argc, char *argv[])
{

	RenderingGame myGame;
	//QApplication a(argc, argv);
	myGame.Initilize(1920 / 4, 1440 / 4);	//Editor w;
	myGame.StartRunning();	//w.show();
	myGame.ShutDown();	//return a.exec();
	return 0;
}
