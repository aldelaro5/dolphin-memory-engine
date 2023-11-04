#pragma once

#include <QByteArray>
#include <QSettings>

#include "../../Common/CommonTypes.h"

class SConfig
{
public:
  static SConfig& getInstance();
  SConfig(SConfig const&) = delete;
  void operator=(SConfig const&) = delete;

  QByteArray getMainWindowGeometry() const;
  QByteArray getMainWindowState() const;
  QByteArray getSplitterState() const;

  QString getWatchModel() const;
  bool getAutoHook() const;

  int getTheme() const;

  int getWatcherUpdateTimerMs() const;
  int getFreezeTimerMs() const;
  int getScannerUpdateTimerMs() const;
  int getViewerUpdateTimerMs() const;
  u32 getMEM1Size() const;
  u32 getMEM2Size() const;

  int getViewerNbrBytesSeparator() const;

  void setMainWindowGeometry(QByteArray const&);
  void setMainWindowState(QByteArray const&);
  void setSplitterState(QByteArray const&);

  void setWatchModel(const QString& json);
  void setAutoHook(bool enabled);

  void setTheme(const int theme);

  void setWatcherUpdateTimerMs(const int watcherUpdateTimerMs);
  void setFreezeTimerMs(const int freezeTimerMs);
  void setScannerUpdateTimerMs(const int scannerUpdateTimerMs);
  void setViewerUpdateTimerMs(const int viewerUpdateTimerMs);
  void setMEM1Size(const u32 mem1SizeReal);
  void setMEM2Size(const u32 mem2SizeReal);

  void setViewerNbrBytesSeparator(const int viewerNbrBytesSeparator);

private:
  SConfig();
  ~SConfig();

  QSettings* m_settings;
};
