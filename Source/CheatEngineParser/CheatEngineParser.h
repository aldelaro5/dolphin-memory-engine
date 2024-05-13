#pragma once

#include <QFile>
#include <QString>
#include <QXmlStreamReader>

#include "../MemoryWatch/MemWatchTreeNode.h"

class CheatEngineParser
{
public:
  CheatEngineParser();
  ~CheatEngineParser();

  CheatEngineParser(const CheatEngineParser&) = delete;
  CheatEngineParser(CheatEngineParser&&) = delete;
  CheatEngineParser& operator=(const CheatEngineParser&) = delete;
  CheatEngineParser& operator=(CheatEngineParser&&) = delete;

  QString getErrorMessages() const;
  bool hasACriticalErrorOccured() const;
  void setTableStartAddress(u32 tableStartAddress);
  MemWatchTreeNode* parseCTFile(QIODevice* CTFileIODevice, bool useDolphinPointer);

private:
  struct cheatEntryParsingState
  {
    qint64 lineNumber = 0;

    bool labelFound = false;
    bool consoleAddressFound = false;
    bool typeFound = false;
    bool lengthForStrFound = false;

    bool validConsoleAddressHex = true;
    bool validConsoleAddress = true;
    bool validType = true;
    bool validLengthForStr = true;
  };

  MemWatchTreeNode* parseCheatTable(MemWatchTreeNode* rootNode, bool useDolphinPointer);
  MemWatchTreeNode* parseCheatEntries(MemWatchTreeNode* node, bool useDolphinPointer);
  void parseCheatEntry(MemWatchTreeNode* node, bool useDolphinPointer);
  void verifyCheatEntryParsingErrors(cheatEntryParsingState state, MemWatchEntry* entry,
                                     bool isGroup, bool useDolphinPointer);
  static QString formatImportedEntryBasicInfo(const MemWatchEntry* entry);

  u32 m_tableStartAddress = 0;
  QString m_errorMessages = "";
  bool m_criticalErrorOccured = false;
  QXmlStreamReader* m_xmlReader;
};
