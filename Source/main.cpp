#include <QApplication>
#include <QCommandLineParser>

#include "GUI/MainWindow.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  QApplication::setApplicationName("Dolphin Memory Engine");
  QApplication::setApplicationVersion("0.9.0");

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QObject::tr("A RAM search made specifically to search, monitor and edit "
                  "the Dolphin Emulator's emulated memory."));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.process(app);

  MainWindow window;
  window.show();
  return app.exec();
}
