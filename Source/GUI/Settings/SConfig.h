#pragma once

#include <memory>
#include <unordered_map>

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

  SConfig(const SConfig&) = delete;
  SConfig(SConfig&&) = delete;
  SConfig& operator=(const SConfig&) = delete;
  SConfig& operator=(SConfig&&) = delete;

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

  void setTheme(int theme);

  void setWatcherUpdateTimerMs(int watcherUpdateTimerMs);
  void setFreezeTimerMs(int freezeTimerMs);
  void setScannerUpdateTimerMs(int scannerUpdateTimerMs);
  void setViewerUpdateTimerMs(int viewerUpdateTimerMs);
  void setScannerShowThreshold(int scannerShowThreshold);
  void setMEM1Size(u32 mem1SizeReal);
  void setMEM2Size(u32 mem2SizeReal);

  void setViewerNbrBytesSeparator(int viewerNbrBytesSeparator);

  bool ownsSettingsFile() const;

private:
  void setValue(const QString& key, const QVariant& value);
  QVariant value(const QString& key, const QVariant& defaultValue) const;

  std::unique_ptr<QLockFile> m_lockFile;
  std::unordered_map<QString, QVariant> m_map;
  QSettings* m_settings{};
};
