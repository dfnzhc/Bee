/**
 * @File MainWindow.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
} // namespace Ui

namespace bee {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildMenus();
    void buildToolBar();
    void buildStatusBar();
    void buildLogWidget();

private:
    Ui::MainWindow* ui;

    std::unique_ptr<class Engine> _pEngine;
};
} // namespace bee
