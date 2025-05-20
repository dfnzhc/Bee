/**
 * @File Main.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/27
 * @Brief This file is part of Bee.
 */

// #include <Bee.hpp>

#include <mimalloc.h>

#include <QApplication>
#include <QSurfaceFormat>

#include "Core/Error.hpp"
#include "Core/Logger.hpp"
#include "Core/Version.hpp"
#include "Widgets/AppContext.hpp"
#include "Widgets/MainWindow.hpp"

using namespace bee;

int main(int argc, char* argv[])
{
    if (mi_is_redirected()) {
        LogInfo("mimalloc is redirected!");
    }
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

#if 0
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setVersion(4, 6);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
#endif

    AppContext context;

    bool bResult = Guardian([&] {
        return context.initialize();
    });

    if (!bResult) {
        LogFatal("Failed to init app context!");
        return EXIT_FAILURE;
    }

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Bee");
    QApplication::setOrganizationDomain("Bee");
    QApplication::setApplicationName("Bee");
    QApplication::setApplicationVersion(QString("Ver %1.%2.%3").arg(BEE_VERSION_MAJOR).arg(BEE_VERSION_MINOR).arg(BEE_VERSION_PATCH));

    MainWindow window;
    window.move(10, 10);
    window.show();

    auto ret = app.exec();
    context.shutdown();

    return ret;
}