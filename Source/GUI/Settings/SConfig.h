#pragma once

#include <memory>

#include <QByteArray>
#include <QLockFile>
#include <QSettings>

#include "../../Common/CommonTypes.h"

class SConfig
{
public:
  static SConfig& getInstance();

  SConfig();
  ~SConfig();

  SConfig(SConfig const&) = delete;
  void operator=(SConfig const&) = delete;

  QString getSettingsFilepath() const;

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
  int getScannerShowThreshold() const;
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
  void setScannerShowThreshold(int scannerShowThreshold);
  void setMEM1Size(const u32 mem1SizeReal);
  void setMEM2Size(const u32 mem2SizeReal);

  void setViewerNbrBytesSeparator(const int viewerNbrBytesSeparator);

  bool ownsSettingsFile() const;

private:
  void setValue(const QString& key, const QVariant& value);
  QVariant value(const QString& key, const QVariant& defaultValue) const;

  std::unique_ptr<QLockFile> m_lockFile;
  std::unordered_map<QString, QVariant> m_map;
  QSettings* m_settings{};
};
