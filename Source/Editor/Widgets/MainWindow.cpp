/**
 * @File MainWindow.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/20
 * @Brief This file is part of Bee.
 */

#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "Engine/Engine.hpp"
#include "Core/AppContext.hpp"

#include <QKeyEvent>

#include "Core/Error.hpp"

using namespace bee;

namespace  {

constexpr int kViewportTabIdx = 0;

} // namespace 

MainWindow::MainWindow(const AppContext& appContext, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), _appContext(appContext)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setup()
{
    buildMenus();
    buildToolBar();
    buildStatusBar();
    buildWidgets();
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

void MainWindow::buildWidgets()
{
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}