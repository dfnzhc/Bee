/**
 * @File ViewportWidget.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/21
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include <QWidget>

namespace bee {

class Engine;

class ViewportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ViewportWidget(Engine* pEngine, QWidget* parent = nullptr);
    ~ViewportWidget() override;

    bool setup();
    void shutdown();

protected:
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Engine*const _pEngine = nullptr;
};

} // namespace bee