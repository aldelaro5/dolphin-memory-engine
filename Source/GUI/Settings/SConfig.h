#pragma once

#include <QSettings>

class SConfig
{
public:
  static SConfig& getInstance();
  SConfig(SConfig const&) = delete;
  void operator=(SConfig const&) = delete;

  int getWatcherUpdateTimerMs() const;
  int getFreezeTimerMs() const;
  int getScannerUpdateTimerMs() const;
  int getViewerUpdateTimerMs() const;

  void setWatcherUpdateTimerMs(const int watcherUpdateTimerMs);
  void setFreezeTimerMs(const int freezeTimerMs);
  void setScannerUpdateTimerMs(const int scannerUpdateTimerMs);
  void setViewerUpdateTimerMs(const int viewerUpdateTimerMs);

private:
  SConfig();
  ~SConfig();

  QSettings* m_settings;
};
