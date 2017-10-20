#include "Editor.h"
#include "RenderingGame.h"
Editor::Editor(QWidget *parent)
	: QMainWindow(parent), isGameRunning(false)
{
	ui.setupUi(this);

	connect(&myTimer, SIGNAL(timeout()), this, SLOT(Update()));
	myTimer.start(0);

	game.Initilize(100, 100);
	isGameRunning = true;
}

void Editor::Update()
{
	if (isGameRunning)
	{
		game.StartRunning();
		game.RenderFrame();
	}
}
