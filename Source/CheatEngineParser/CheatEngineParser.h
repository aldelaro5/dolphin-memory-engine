#pragma once

#include <QFile>
#include <QString>

#include "../MemoryWatch/MemWatchTreeNode.h"

class CheatEngineParser
{
public:
  static void setTableStartAddress(const u32 tableStartAddress);
  static MemWatchTreeNode* parseCTFile(const QFile& CTFile, const bool useDolphinPointer);

private:
  static u32 m_tableStartAddress;
  static QString m_errorMessages;
};