#include "SConfig.h"

#include <cassert>
#include <iostream>

#include <QDir>
#include <QFileInfo>
#include <qcoreapplication.h>
#include <qfile.h>

namespace
{
SConfig* g_instance{};
}

SConfig::SConfig()
{
  assert(!g_instance && "Only a single SConfig instance is allowed");
  g_instance = this;

  const QString exeDir = QCoreApplication::applicationDirPath();
  const QString portableFilePath = exeDir + "/portable.txt";
  QFile file(portableFilePath);
  if (file.exists())
  {
    m_settings = new QSettings(exeDir + "/settings.ini", QSettings::IniFormat);
  }
  else
  {
    const QString organization{"dolphin-memory-engine"};
    const QString application{"dolphin-memory-engine"};
    m_settings =
        new QSettings(QSettings::IniFormat, QSettings::UserScope, organization, application);
  }

  const QString lockFilepath{m_settings->fileName() + "_lock"};
  QDir().mkpath(QFileInfo{lockFilepath}.absolutePath());
  m_lockFile = std::make_unique<QLockFile>(lockFilepath);
  if (!m_lockFile->tryLock())
  {
    std::cerr << "ERROR: Unable to lock \"" + lockFilepath.toStdString() +
                     "\". Is another instance running?\n";
  }
}

SConfig::~SConfig()
{
  delete m_settings;
  m_lockFile.reset();

  assert(g_instance == this && "Inconsistent handling of the SConfig global instance");
  g_instance = nullptr;
}

SConfig& SConfig::getInstance()
{
  assert(g_instance && "SConfig must be instantiated first");
  return *g_instance;
}

QString SConfig::getSettingsFilepath() const
{
  return m_settings->fileName();
}

QString SConfig::getWatchModel() const
{
  return value("watchModel", QString{}).toString();
}

QString SConfig::getStructDefs() const
{
  return value("structDefs", QString{}).toString();
}

bool SConfig::getAutoHook() const
{
  return value("autoHook", true).toBool();
}

QByteArray SConfig::getMainWindowGeometry() const
{
  return value("mainWindowSettings/mainWindowGeometry", QByteArray{}).toByteArray();
}

QByteArray SConfig::getMainWindowState() const
{
  return value("mainWindowSettings/mainWindowState", QByteArray{}).toByteArray();
}

QByteArray SConfig::getSplitterState() const
{
  return value("mainWindowSettings/splitterState", QByteArray{}).toByteArray();
}

int SConfig::getTheme() const
{
  return value("coreSettings/theme", 0).toInt();
}

int SConfig::getWatcherUpdateTimerMs() const
{
  return value("timerSettings/watcherUpdateTimerMs", 100).toInt();
}

int SConfig::getFreezeTimerMs() const
{
  return value("timerSettings/freezeTimerMs", 10).toInt();
}

int SConfig::getScannerUpdateTimerMs() const
{
  return value("timerSettings/scannerUpdateTimerMs", 10).toInt();
}

int SConfig::getViewerUpdateTimerMs() const
{
  return value("timerSettings/viewerUpdateTimerMs", 100).toInt();
}

int SConfig::getScannerShowThreshold() const
{
  return value("scannerSettings/scannerShowThreshold", 1000).toInt();
}

int SConfig::getViewerNbrBytesSeparator() const
{
  return value("viewerSettings/nbrBytesSeparator", 1).toInt();
}

u32 SConfig::getMEM1Size() const
{
  return value("memorySettings/MEM1Size", 24u * 1024 * 1024).toUInt();
}

u32 SConfig::getMEM2Size() const
{
  return value("memorySettings/MEM2Size", 64u * 1024 * 1024).toUInt();
}

void SConfig::setWatchModel(const QString& json)
{
  setValue("watchModel", json);
}

void SConfig::setStructDefs(const QString& json)
{
  setValue("structDefs", json);
}

void SConfig::setAutoHook(const bool enabled)
{
  setValue("autoHook", enabled);
}

void SConfig::setMainWindowGeometry(QByteArray const& geometry)
{
  setValue("mainWindowSettings/mainWindowGeometry", geometry);
}

void SConfig::setMainWindowState(QByteArray const& state)
{
  setValue("mainWindowSettings/mainWindowState", state);
}

void SConfig::setSplitterState(QByteArray const& state)
{
  setValue("mainWindowSettings/splitterState", state);
}

void SConfig::setTheme(const int theme)
{
  setValue("coreSettings/theme", theme);
}

void SConfig::setWatcherUpdateTimerMs(const int updateTimerMs)
{
  setValue("timerSettings/watcherUpdateTimerMs", updateTimerMs);
}

void SConfig::setFreezeTimerMs(const int freezeTimerMs)
{
  setValue("timerSettings/freezeTimerMs", freezeTimerMs);
}

void SConfig::setScannerUpdateTimerMs(const int scannerUpdateTimerMs)
{
  setValue("timerSettings/scannerUpdateTimerMs", scannerUpdateTimerMs);
}

void SConfig::setViewerUpdateTimerMs(const int viewerUpdateTimerMs)
{
  setValue("timerSettings/viewerUpdateTimerMs", viewerUpdateTimerMs);
}

void SConfig::setScannerShowThreshold(const int scannerShowThreshold)
{
  setValue("scannerSettings/scannerShowThreshold", scannerShowThreshold);
}

void SConfig::setViewerNbrBytesSeparator(const int viewerNbrBytesSeparator)
{
  setValue("viewerSettings/nbrBytesSeparator", viewerNbrBytesSeparator);
}

void SConfig::setMEM1Size(const u32 mem1SizeReal)
{
  setValue("memorySettings/MEM1Size", mem1SizeReal);
}

void SConfig::setMEM2Size(const u32 mem2SizeReal)
{
  setValue("memorySettings/MEM2Size", mem2SizeReal);
}

bool SConfig::getAutoloadLastFile() const
{
  return value("autoloadLastFile", false).toBool();
}

void SConfig::setAutoloadLastFile(const bool enabled)
{
  setValue("autoloadLastFile", enabled);
}

QString SConfig::getLastLoadedFile() const
{
  return value("lastLoadedFile", QString{}).toString();
}

void SConfig::setLastLoadedFile(const QString& fileName)
{
  setValue("lastLoadedFile", fileName);
}

bool SConfig::ownsSettingsFile() const
{
  return m_lockFile->isLocked();
}

bool SConfig::getCollapseGroupsOnSave() const
{
  return value("collapseGroupsOnSave", false).toBool();
}

void SConfig::setCollapseGroupsOnSave(const bool enabled)
{
  setValue("collapseGroupsOnSave", enabled);
}

void SConfig::setValue(const QString& key, const QVariant& value)
{
  m_map[key] = value;

  if (ownsSettingsFile())
  {
    m_settings->setValue(key, value);
  }
}

QVariant SConfig::value(const QString& key, const QVariant& defaultValue) const
{
  const auto it = m_map.find(key);
  if (it != m_map.cend())
  {
    return it->second;
  }
  return m_settings->value(key, defaultValue);
}
