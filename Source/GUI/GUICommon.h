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

void changeApplicationStyle(int);

extern bool g_valueEditing;
}  // namespace GUICommon
