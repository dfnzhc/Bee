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
    explicit MainWindow(const class AppContext& appContext, QWidget* parent = nullptr);
    ~MainWindow() override;

    void setup();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildMenus();
    void buildToolBar();
    void buildStatusBar();
    void buildWidgets();

private:
    Ui::MainWindow* ui;

    const class AppContext& _appContext;

};
} // namespace bee
