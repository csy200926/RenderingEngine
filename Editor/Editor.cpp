#include "RenderingGame.h"
#include "Editor.h"

Editor::Editor(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	pRenderingGame = new RenderingGame();
	pRenderingGame->Initilize(1024, 768);

	connect(&myTimer, SIGNAL(timeout()), this, SLOT(Update()));
	myTimer.start(0);
}
void Editor::Update()
{
	pRenderingGame->UpdateFrame();
	pRenderingGame->RenderFrame();
}