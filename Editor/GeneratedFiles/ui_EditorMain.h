/********************************************************************************
** Form generated from reading UI file 'EditorMain.ui'
**
** Created by: Qt User Interface Compiler version 5.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITORMAIN_H
#define UI_EDITORMAIN_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditorMain
{
public:

    void setupUi(QWidget *EditorMain)
    {
        if (EditorMain->objectName().isEmpty())
            EditorMain->setObjectName(QStringLiteral("EditorMain"));
        EditorMain->resize(400, 300);

        retranslateUi(EditorMain);

        QMetaObject::connectSlotsByName(EditorMain);
    } // setupUi

    void retranslateUi(QWidget *EditorMain)
    {
        EditorMain->setWindowTitle(QApplication::translate("EditorMain", "EditorMain", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class EditorMain: public Ui_EditorMain {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITORMAIN_H
