#include "SConfig.h"

SConfig::SConfig()
{
  m_settings = new QSettings("settings.ini", QSettings::IniFormat);
}

SConfig::~SConfig()
{
  delete m_settings;
}

SConfig& SConfig::getInstance()
{
  static SConfig instance;
  return instance;
}

QByteArray SConfig::getMainWindowGeometry() const
{
  return m_settings->value("mainWindowSettings/mainWindowGeometry", QByteArray{}).toByteArray();
}

QByteArray SConfig::getMainWindowState() const
{
  return m_settings->value("mainWindowSettings/mainWindowState", QByteArray{}).toByteArray();
}

QByteArray SConfig::getSplitterState() const
{
  return m_settings->value("mainWindowSettings/splitterState", QByteArray{}).toByteArray();
}

int SConfig::getWatcherUpdateTimerMs() const
{
  return m_settings->value("timerSettings/watcherUpdateTimerMs", 100).toInt();
}

int SConfig::getFreezeTimerMs() const
{
  return m_settings->value("timerSettings/freezeTimerMs", 10).toInt();
}

int SConfig::getScannerUpdateTimerMs() const
{
  return m_settings->value("timerSettings/scannerUpdateTimerMs", 10).toInt();
}

int SConfig::getViewerUpdateTimerMs() const
{
  return m_settings->value("timerSettings/viewerUpdateTimerMs", 100).toInt();
}

int SConfig::getViewerNbrBytesSeparator() const
{
  return m_settings->value("viewerSettings/nbrBytesSeparator", 1).toInt();
}

void SConfig::setMainWindowGeometry(QByteArray const& geometry)
{
  m_settings->setValue("mainWindowSettings/mainWindowGeometry", geometry);
}

void SConfig::setMainWindowState(QByteArray const& state)
{
  m_settings->setValue("mainWindowSettings/mainWindowState", state);
}

void SConfig::setSplitterState(QByteArray const& state)
{
  m_settings->setValue("mainWindowSettings/splitterState", state);
}

void SConfig::setWatcherUpdateTimerMs(const int updateTimerMs)
{
  m_settings->setValue("timerSettings/watcherUpdateTimerMs", updateTimerMs);
}

void SConfig::setFreezeTimerMs(const int freezeTimerMs)
{
  m_settings->setValue("timerSettings/freezeTimerMs", freezeTimerMs);
}

void SConfig::setScannerUpdateTimerMs(const int scannerUpdateTimerMs)
{
  m_settings->setValue("timerSettings/scannerUpdateTimerMs", scannerUpdateTimerMs);
}

void SConfig::setViewerUpdateTimerMs(const int viewerUpdateTimerMs)
{
  m_settings->setValue("timerSettings/viewerUpdateTimerMs", viewerUpdateTimerMs);
}

void SConfig::setViewerNbrBytesSeparator(const int viewerNbrBytesSeparator)
{
  m_settings->setValue("viewerSettings/nbrBytesSeparator", viewerNbrBytesSeparator);
}
