#include "GUICommon.h"

#include <QStringList>

namespace GUICommon
{
QStringList g_memTypeNames = QStringList({"Byte", "2 bytes (Halfword)", "4 bytes (Word)", "Float",
                                          "Double", "String", "Array of bytes"});

QStringList g_memBaseNames = QStringList({"Decimal", "Hexadecimal", "Octal", "Binary"});

QStringList g_memScanFilter = QStringList({"Exact value", "Increased by", "Decreased by", "Between",
                                           "Bigger than", "Smaller than", "Increased", "Decreased",
                                           "Changed", "Unchanged", "Unknown initial value"});

QString getStringFromType(const Common::MemType type, const size_t length)
{
  switch (type)
  {
  case Common::MemType::type_byte:
  case Common::MemType::type_halfword:
  case Common::MemType::type_word:
  case Common::MemType::type_float:
  case Common::MemType::type_double:
    return GUICommon::g_memTypeNames.at(static_cast<int>(type));
  case Common::MemType::type_string:
    return QString::fromStdString("string[" + std::to_string(length) + "]");
  case Common::MemType::type_byteArray:
    return QString::fromStdString("array of bytes[" + std::to_string(length) + "]");
  default:
    return QString("");
  }
}

QString getNameFromBase(const Common::MemBase base)
{
  switch (base)
  {
  case Common::MemBase::base_binary:
    return QString("binary");
  case Common::MemBase::base_octal:
    return QString("octal");
  case Common::MemBase::base_decimal:
    return QString("decimal");
  case Common::MemBase::base_hexadecimal:
    return QString("hexadecimal");
  default:
    return QString("");
  }
}
}