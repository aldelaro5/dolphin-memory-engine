#include "CheatEngineParser.h"

u32 CheatEngineParser::m_tableStartAddress = 0;
QString m_errorMessages = "";

void CheatEngineParser::setTableStartAddress(const u32 tableStartAddress)
{
  m_tableStartAddress = tableStartAddress;
}

MemWatchTreeNode* CheatEngineParser::parseCTFile(const QFile& CTFile, const bool useDolphinPointer)
{
  return nullptr;
}