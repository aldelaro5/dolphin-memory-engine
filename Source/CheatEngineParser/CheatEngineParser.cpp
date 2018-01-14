#include "CheatEngineParser.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "../DolphinProcess/DolphinAccessor.h"

CheatEngineParser::CheatEngineParser()
{
  m_xmlReader = new QXmlStreamReader();
}

void CheatEngineParser::setTableStartAddress(const u64 tableStartAddress)
{
  m_tableStartAddress = tableStartAddress;
}

MemWatchTreeNode* CheatEngineParser::parseCTFile(QIODevice* CTFileIODevice,
                                                 const bool useDolphinPointer)
{
  m_xmlReader->setDevice(CTFileIODevice);

  if (m_xmlReader->readNextStartElement())
  {
    std::string test = m_xmlReader->name().toString().toStdString();
    if (m_xmlReader->name() == QString("CheatTable"))
    {
      MemWatchTreeNode* rootNode = new MemWatchTreeNode(nullptr);
      parseCheatTable(rootNode, useDolphinPointer);
      return rootNode;
    }
  }

  return nullptr;
}

MemWatchTreeNode* CheatEngineParser::parseCheatTable(MemWatchTreeNode* rootNode,
                                                     const bool useDolphinPointer)
{
  while (!(m_xmlReader->isEndElement() && m_xmlReader->name() == QString("CheatTable")))
  {
    if (m_xmlReader->readNextStartElement())
    {
      std::string test = m_xmlReader->name().toString().toStdString();
      if (m_xmlReader->name() == QString("CheatEntries"))
      {
        parseCheatEntries(rootNode, useDolphinPointer);
      }
    }
  }
  return rootNode;
}

MemWatchTreeNode* CheatEngineParser::parseCheatEntries(MemWatchTreeNode* node,
                                                       const bool useDolphinPointer)
{
  while (!(m_xmlReader->isEndElement() && m_xmlReader->name() == QString("CheatEntries")))
  {
    if (m_xmlReader->readNextStartElement())
    {
      std::string test = m_xmlReader->name().toString().toStdString();
      if (m_xmlReader->name() == QString("CheatEntry"))
      {
        parseCheatEntry(node, useDolphinPointer);
      }
    }
  }
  return node;
}

void CheatEngineParser::parseCheatEntry(MemWatchTreeNode* node, const bool useDolphinPointer)
{
  std::string label = "No label";
  u32 consoleAddress = Common::MEM1_START;
  Common::MemType type = Common::MemType::type_byte;
  Common::MemBase base = Common::MemBase::base_decimal;
  bool isUnsigned = true;
  size_t length = 1;
  bool isGroup = false;

  while (!(m_xmlReader->isEndElement() && m_xmlReader->name() == QString("CheatEntry")))
  {
    if (m_xmlReader->readNextStartElement())
    {
      if (m_xmlReader->name() == QString("Description"))
      {
        label = m_xmlReader->readElementText().toStdString();
        if (label.at(0) == '"')
          label.erase(0, 1);
        if (label.at(label.length() - 1) == '"')
          label.erase(label.length() - 1, 1);
      }
      else if (m_xmlReader->name() == QString("VariableType"))
      {
        std::string strVarType = m_xmlReader->readElementText().toStdString();
        if (strVarType == "Custom")
        {
          continue;
        }
        else
        {
          if (strVarType == "Byte" || strVarType == "Binary")
            type = Common::MemType::type_byte;
          else if (strVarType == "String")
            type = Common::MemType::type_string;
          else if (strVarType == "Array of byte")
            type = Common::MemType::type_byteArray;
        }
      }
      else if (m_xmlReader->name() == QString("CustomType"))
      {
        std::string strVarType = m_xmlReader->readElementText().toStdString();
        if (strVarType == "2 Byte Big Endian")
          type = Common::MemType::type_halfword;
        else if (strVarType == "4 Byte Big Endian")
          type = Common::MemType::type_word;
        else if (strVarType == "Float Big Endian")
          type = Common::MemType::type_float;
      }
      else if (m_xmlReader->name() == QString("Length") ||
               m_xmlReader->name() == QString("ByteLength"))
      {
        std::string strLength = m_xmlReader->readElementText().toStdString();
        std::stringstream ss(strLength);
        ss >> length;
      }
      else if (m_xmlReader->name() == QString("Address"))
      {
        if (useDolphinPointer)
        {
          m_xmlReader->skipCurrentElement();
        }
        else
        {
          u64 consoleAddressCandidate = 0;
          std::string strCEAddress = m_xmlReader->readElementText().toStdString();
          std::stringstream ss(strCEAddress);
          ss >> std::hex;
          ss >> consoleAddressCandidate;
          consoleAddressCandidate -= m_tableStartAddress;
          consoleAddressCandidate += Common::MEM1_START;
          if (DolphinComm::DolphinAccessor::isValidConsoleAddress(consoleAddressCandidate))
            consoleAddress = consoleAddressCandidate;
        }
      }
      else if (m_xmlReader->name() == QString("Offsets"))
      {
        if (useDolphinPointer)
        {
          if (m_xmlReader->readNextStartElement())
          {
            if (m_xmlReader->name() == QString("Offset"))
            {
              u32 consoleAddressCandidate = 0;
              std::string strOffset = m_xmlReader->readElementText().toStdString();
              std::stringstream ss(strOffset);
              ss >> std::hex;
              ss >> consoleAddressCandidate;
              consoleAddressCandidate += Common::MEM1_START;
              if (DolphinComm::DolphinAccessor::isValidConsoleAddress(consoleAddressCandidate))
                consoleAddress = consoleAddressCandidate;
            }
          }
        }
        else
        {
          m_xmlReader->skipCurrentElement();
        }
      }
      else if (m_xmlReader->name() == QString("ShowAsHex"))
      {
        if (m_xmlReader->readElementText().toStdString() == "1")
          base = Common::MemBase::base_hexadecimal;
      }
      else if (m_xmlReader->name() == QString("ShowAsBinary"))
      {
        if (m_xmlReader->readElementText().toStdString() == "1")
          base = Common::MemBase::base_binary;
      }
      else if (m_xmlReader->name() == QString("ShowAsSigned"))
      {
        if (m_xmlReader->readElementText().toStdString() == "1")
          isUnsigned = false;
      }
      else if (m_xmlReader->name() == QString("CheatEntries"))
      {
        isGroup = true;
        MemWatchTreeNode* newNode =
            new MemWatchTreeNode(nullptr, node, true, QString::fromStdString(label));
        node->appendChild(newNode);
        parseCheatEntries(newNode, useDolphinPointer);
      }
      else
      {
        m_xmlReader->skipCurrentElement();
      }
    }
  }

  if (!isGroup)
  {
    MemWatchEntry* entry =
        new MemWatchEntry(label, consoleAddress, type, base, isUnsigned, length, false);
    MemWatchTreeNode* newNode = new MemWatchTreeNode(entry, node, false, "");
    node->appendChild(newNode);
  }
}