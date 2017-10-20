#pragma once

#include <QWidget>
#include "ui_EditorMain.h"

class EditorMain : public QWidget
{
	Q_OBJECT

public:
	EditorMain(QWidget *parent = Q_NULLPTR);
	~EditorMain();

private:
	Ui::EditorMain ui;
};
