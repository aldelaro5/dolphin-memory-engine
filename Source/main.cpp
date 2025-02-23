#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>

#include "GUI/MainWindow.h"
#include "GUI/Settings/SConfig.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  QApplication::setApplicationName("Dolphin Memory Engine");
  QApplication::setApplicationVersion("1.2.5");

  SConfig config;  // Initialize global settings object

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QObject::tr("A RAM search made specifically to search, monitor and edit "
                  "the Dolphin Emulator's emulated memory."));
  parser.addHelpOption();
  parser.addVersionOption();

  const QCommandLineOption dolphinProcessNameOption(
      QStringList() << "d" << "dolphin-process-name",
      QObject::tr("Specify custom name for the Dolphin Emulator process. By default, "
                  "platform-specific names are used (e.g. \"Dolphin.exe\" on Windows, or "
                  "\"dolphin-emu\" on Linux or macOS)."),
      "dolphin_process_name");
  parser.addOption(dolphinProcessNameOption);

  parser.process(app);

  const QString dolphinProcessName{parser.value(dolphinProcessNameOption)};
  if (!dolphinProcessName.isEmpty())
  {
    qputenv("DME_DOLPHIN_PROCESS_NAME", dolphinProcessName.toStdString().c_str());
  }

  MainWindow window;

  if (!config.ownsSettingsFile())
  {
    QMessageBox box(
        QMessageBox::Warning, QObject::tr("Another instance is already running"),
        QObject::tr(
            "Changes made to settings will not be preserved in this session. This includes changes "
            "to the watch list, which will need to be saved manually into a file."),
        QMessageBox::Ok);
    box.setWindowIcon(window.windowIcon());
    box.exec();
  }

  window.show();
  return QApplication::exec();
}
