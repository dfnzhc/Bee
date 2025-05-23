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

ViewportWidget::ViewportWidget(Engine* pEngine, QWidget* parent) : QWidget(parent), _pEngine(pEngine)
{
    BEE_ASSERT(_pEngine, "pEngine must not be nullptr.");
    
    setLayout(new QVBoxLayout);
    setMouseTracking(true);

    // For test
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor{23, 23, 23});
    setAutoFillBackground(true); 
    setPalette(pal);
}

ViewportWidget::~ViewportWidget()
{
}

bool ViewportWidget::setup()
{
    EngineInitParams param;

    if (!_pEngine->initialize(param)) {
        // TODO: Log widget
        return false;
    }

    

    return true;
}

void ViewportWidget::shutdown()
{
    _pEngine->shutdown();
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