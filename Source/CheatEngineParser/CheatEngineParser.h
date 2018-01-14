#pragma once

#include <QFile>
#include <QString>
#include <QXmlStreamReader>

#include "../MemoryWatch/MemWatchTreeNode.h"

class CheatEngineParser
{
public:
  CheatEngineParser();

  void setTableStartAddress(const u64 tableStartAddress);
  MemWatchTreeNode* parseCTFile(QIODevice* CTFileIODevice, const bool useDolphinPointer);

private:
  MemWatchTreeNode* parseCheatTable(MemWatchTreeNode* rootNode, const bool useDolphinPointer);
  MemWatchTreeNode* parseCheatEntries(MemWatchTreeNode* node, const bool useDolphinPointer);
  void parseCheatEntry(MemWatchTreeNode* node, const bool useDolphinPointer);

  u64 m_tableStartAddress = 0;
  QString m_errorMessages = "";
  QXmlStreamReader* m_xmlReader;
};