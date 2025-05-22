/**
 * @File ViewportWidget.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/5/21
 * @Brief This file is part of Bee.
 */

#include "ViewportWidget.hpp"

#include <qboxlayout.h>
#include <QLabel>

#include "Core/Error.hpp"
#include "Engine/Engine.hpp"
#include "Utils/EventTools.hpp"

using namespace bee;

ViewportWidget::ViewportWidget(const Engine* pEngine, QWidget* parent) : QScrollArea(parent), _pEngine(pEngine)
{
    BEE_ASSERT(_pEngine, "pEngine must not be nullptr.");
    
    setLayout(new QVBoxLayout);
    setMouseTracking(true);
    setBackgroundRole(QPalette::Dark); // TODO: background color not working

    // For test
    // QPalette pal = QPalette();
    // pal.setColor(QPalette::Window, QColor{23, 23, 23});
    // setAutoFillBackground(true); 
    // setPalette(pal);

    QLabel *imageLabel = new QLabel;
    auto pm = QPixmap(1920, 1080);
    pm.fill(QColor{23, 23, 23});
    imageLabel->setPixmap(pm);
    setWidget(imageLabel);
}

ViewportWidget::~ViewportWidget()
{
}

bool ViewportWidget::setup()
{

    return true;
}

void ViewportWidget::shutdown()
{
}

bool ViewportWidget::event(QEvent* event)
{
    const auto& type = event->type();
    
    if (type == QEvent::Type::KeyPress ||
        type == QEvent::Type::KeyRelease) {
        _pEngine->onInputEvent(EventTools::Map(static_cast<QKeyEvent*>(event)));
    }
    
    if (type == QEvent::Type::MouseMove ||
        type == QEvent::Type::MouseButtonPress ||
        type == QEvent::Type::MouseButtonDblClick ||
        type == QEvent::Type::MouseButtonRelease) {
        _pEngine->onInputEvent(EventTools::Map(static_cast<QMouseEvent*>(event)));
    }

    if (type == QEvent::Type::Wheel) {
        _pEngine->onInputEvent(EventTools::Map(static_cast<QWheelEvent*>(event)));
    }
    
    return QWidget::event(event);
}

void ViewportWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}