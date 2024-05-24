#pragma once

#include <QString>
#include <QStringList>

#include "../Common/MemoryCommon.h"

namespace GUICommon
{
extern QStringList g_memTypeNames;
extern QStringList g_memScanFilter;
extern QStringList g_memBaseNames;

QString getStringFromType(Common::MemType type, size_t length = 0);
QString getNameFromBase(Common::MemBase base);

enum class ApplicationStyle
{
  System = 0,
  Light,
  DarkGray,
  Dark,
};

void changeApplicationStyle(ApplicationStyle style);

extern bool g_valueEditing;
}  // namespace GUICommon
