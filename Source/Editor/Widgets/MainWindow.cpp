/**
 * @File MainWindow.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/20
 * @Brief This file is part of Bee.
 */

#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "Engine/Engine.hpp"

#include <QKeyEvent>

using namespace bee;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    buildMenus();
    buildToolBar();
    buildStatusBar();
    buildLogWidget();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::buildMenus()
{
    /// File actions
    ui->actionFileExit->setShortcut(QKeySequence::Quit);
    connect(ui->actionFileExit, SIGNAL(triggered()), SLOT(close())); // TODO：how to handle close event? 
}

void MainWindow::buildToolBar()
{
}

void MainWindow::buildStatusBar()
{
}

void MainWindow::buildLogWidget()
{
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}