#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Editor.h"
#include <qtimer.h>

class RenderingGame;

class Editor : public QMainWindow
{
    Q_OBJECT

public:
    Editor(QWidget *parent = Q_NULLPTR);
	RenderingGame* pRenderingGame;
private:
    Ui::EditorClass ui;

	QTimer myTimer;
private slots:
	void Update();
};
