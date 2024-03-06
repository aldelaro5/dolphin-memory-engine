#include <QApplication>

#include "GUI/MainWindow.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  QApplication::setApplicationName("Dolphin Memory Engine");
  QApplication::setApplicationVersion("0.9.0");

  MainWindow window;
  window.show();
  return app.exec();
}
