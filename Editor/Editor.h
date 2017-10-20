#pragma once

#include <QtWidgets/QMainWindow>

#include "ui_Editor.h"

#include <qtimer.h>

#include "RenderingGame.h"

class Editor : public QMainWindow
{
	Q_OBJECT

public:
	Editor(QWidget *parent = Q_NULLPTR);

private:
	Ui::EditorClass ui;
	QTimer myTimer;

	RenderingGame game;

	bool isGameRunning;

private slots:
	void Update();
};
