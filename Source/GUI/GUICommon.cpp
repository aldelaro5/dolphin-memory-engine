#include "GUICommon.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFontMetrics>
#include <QStringList>
#include <QToolTip>

namespace GUICommon
{
QStringList g_memTypeNames =
    QStringList({QCoreApplication::translate("Common", "Byte"),
                 QCoreApplication::translate("Common", "2 bytes (Halfword)"),
                 QCoreApplication::translate("Common", "4 bytes (Word)"),
                 QCoreApplication::translate("Common", "Float"),
                 QCoreApplication::translate("Common", "Double"),
                 QCoreApplication::translate("Common", "String"),
                 QCoreApplication::translate("Common", "Array of bytes"),
                 QCoreApplication::translate("Common", "Struct"),
                 QCoreApplication::translate("Common", "Assembly (PowerPC)"),
                 QCoreApplication::translate("Common", "8 bytes (Doubleword)"),
                 QCoreApplication::translate("Common", "Array")});

QStringList g_memBaseNames = QStringList({QCoreApplication::translate("Common", "Decimal"),
                                          QCoreApplication::translate("Common", "Hexadecimal"),
                                          QCoreApplication::translate("Common", "Octal"),
                                          QCoreApplication::translate("Common", "Binary")});

QStringList g_memScanFilter =
    QStringList({QCoreApplication::translate("Common", "Exact value"),
                 QCoreApplication::translate("Common", "Increased by"),
                 QCoreApplication::translate("Common", "Decreased by"),
                 QCoreApplication::translate("Common", "Between"),
                 QCoreApplication::translate("Common", "Bigger than"),
                 QCoreApplication::translate("Common", "Smaller than"),
                 QCoreApplication::translate("Common", "Increased"),
                 QCoreApplication::translate("Common", "Decreased"),
                 QCoreApplication::translate("Common", "Changed"),
                 QCoreApplication::translate("Common", "Unchanged"),
                 QCoreApplication::translate("Common", "Unknown initial value")});

bool g_valueEditing = false;

QString getStringFromType(const Common::MemType type, const size_t length)
{
  switch (type)
  {
  case Common::MemType::type_byte:
  case Common::MemType::type_halfword:
  case Common::MemType::type_word:
  case Common::MemType::type_doubleword:
  case Common::MemType::type_float:
  case Common::MemType::type_double:
  case Common::MemType::type_struct:
  case Common::MemType::type_ppc:
  case Common::MemType::type_array:
    return GUICommon::g_memTypeNames.at(static_cast<int>(type));
  case Common::MemType::type_string:
    return QString::fromStdString("string[" + std::to_string(length) + "]");
  case Common::MemType::type_byteArray:
    return QString::fromStdString("array of bytes[" + std::to_string(length) + "]");
  default:
    return {};
  }
}

QString getNameFromBase(const Common::MemBase base)
{
  switch (base)
  {
  case Common::MemBase::base_binary:
    return QCoreApplication::translate("Common", "binary");
  case Common::MemBase::base_octal:
    return QCoreApplication::translate("Common", "octal");
  case Common::MemBase::base_decimal:
    return QCoreApplication::translate("Common", "decimal");
  case Common::MemBase::base_hexadecimal:
    return QCoreApplication::translate("Common", "hexadecimal");
  default:
    return {};
  }
}

bool isContainerType(const Common::MemType type)
{
  switch (type)
  {
  case Common::MemType::type_array:
  case Common::MemType::type_struct:
    return true;
  default:
    return false;
  }
}

void changeApplicationStyle(const ApplicationStyle style)
{
  QApplication::setStyle(QStringLiteral("fusion"));

  if (style == ApplicationStyle::System)
  {
    qApp->setPalette({});
    QToolTip::setPalette({});
    qApp->setStyleSheet({});
    return;
  }

  if (style == ApplicationStyle::Light)
  {
    QPalette palette;
    palette.setColor(QPalette::All, QPalette::Window, QColor(239, 239, 239));
    palette.setColor(QPalette::Disabled, QPalette::Window, QColor(239, 239, 239));
    palette.setColor(QPalette::All, QPalette::WindowText, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(190, 190, 190));
    palette.setColor(QPalette::All, QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(239, 239, 239));
    palette.setColor(QPalette::All, QPalette::AlternateBase, QColor(247, 247, 247));
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(247, 247, 247));
    palette.setColor(QPalette::All, QPalette::ToolTipBase, QColor(255, 255, 220));
    palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(255, 255, 220));
    palette.setColor(QPalette::All, QPalette::ToolTipText, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(0, 0, 0));
    palette.setColor(QPalette::All, QPalette::PlaceholderText, QColor(119, 119, 119));
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(119, 119, 119));
    palette.setColor(QPalette::All, QPalette::Text, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(190, 190, 190));
    palette.setColor(QPalette::All, QPalette::Button, QColor(239, 239, 239));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(239, 239, 239));
    palette.setColor(QPalette::All, QPalette::ButtonText, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(190, 190, 190));
    palette.setColor(QPalette::All, QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::All, QPalette::Light, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor(255, 255, 255));
    palette.setColor(QPalette::All, QPalette::Midlight, QColor(202, 202, 202));
    palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor(202, 202, 202));
    palette.setColor(QPalette::All, QPalette::Dark, QColor(159, 159, 159));
    palette.setColor(QPalette::Disabled, QPalette::Dark, QColor(190, 190, 190));
    palette.setColor(QPalette::All, QPalette::Mid, QColor(184, 184, 184));
    palette.setColor(QPalette::Disabled, QPalette::Mid, QColor(184, 184, 184));
    palette.setColor(QPalette::All, QPalette::Shadow, QColor(118, 118, 118));
    palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor(177, 177, 177));
    palette.setColor(QPalette::All, QPalette::Highlight, QColor(48, 140, 198));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(145, 145, 145));
    palette.setColor(QPalette::All, QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::All, QPalette::Link, QColor(0, 0, 255));
    palette.setColor(QPalette::Disabled, QPalette::Link, QColor(0, 0, 255));
    palette.setColor(QPalette::All, QPalette::LinkVisited, QColor(255, 0, 255));
    palette.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(255, 0, 255));

    qApp->setPalette(palette);
    QToolTip::setPalette(palette);

    const int padding{QFontMetrics(QFont()).height() / 2};
    qApp->setStyleSheet(QString(R"(
        QToolTip {
            padding: %1px;
            border: 1px solid rgb(239, 239, 239);
            background: rgb(255, 255, 255);
        }
    )")
                            .arg(padding));
  }
  else if (style == ApplicationStyle::DarkGray)
  {
    QPalette palette;
    palette.setColor(QPalette::All, QPalette::Window, QColor(50, 50, 50));
    palette.setColor(QPalette::Disabled, QPalette::Window, QColor(55, 55, 55));
    palette.setColor(QPalette::All, QPalette::WindowText, QColor(200, 200, 200));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(108, 108, 108));
    palette.setColor(QPalette::All, QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(30, 30, 30));
    palette.setColor(QPalette::All, QPalette::AlternateBase, QColor(38, 38, 38));
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(42, 42, 42));
    palette.setColor(QPalette::All, QPalette::ToolTipBase, QColor(45, 45, 45));
    palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(45, 45, 45));
    palette.setColor(QPalette::All, QPalette::ToolTipText, QColor(200, 200, 200));
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(200, 200, 200));
    palette.setColor(QPalette::All, QPalette::PlaceholderText, QColor(90, 90, 90));
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(90, 90, 90));
    palette.setColor(QPalette::All, QPalette::Text, QColor(200, 200, 200));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(108, 108, 108));
    palette.setColor(QPalette::All, QPalette::Button, QColor(54, 54, 54));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(54, 54, 54));
    palette.setColor(QPalette::All, QPalette::ButtonText, QColor(200, 200, 200));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(108, 108, 108));
    palette.setColor(QPalette::All, QPalette::BrightText, QColor(75, 75, 75));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::All, QPalette::Light, QColor(26, 26, 26));
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor(26, 26, 26));
    palette.setColor(QPalette::All, QPalette::Midlight, QColor(40, 40, 40));
    palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor(40, 40, 40));
    palette.setColor(QPalette::All, QPalette::Dark, QColor(108, 108, 108));
    palette.setColor(QPalette::Disabled, QPalette::Dark, QColor(108, 108, 108));
    palette.setColor(QPalette::All, QPalette::Mid, QColor(71, 71, 71));
    palette.setColor(QPalette::Disabled, QPalette::Mid, QColor(71, 71, 71));
    palette.setColor(QPalette::All, QPalette::Shadow, QColor(25, 25, 25));
    palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor(37, 37, 37));
    palette.setColor(QPalette::All, QPalette::Highlight, QColor(45, 140, 225));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(45, 140, 225).darker());
    palette.setColor(QPalette::All, QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(40, 40, 40));
    palette.setColor(QPalette::All, QPalette::Link, QColor(40, 130, 220));
    palette.setColor(QPalette::Disabled, QPalette::Link, QColor(40, 130, 220).darker());
    palette.setColor(QPalette::All, QPalette::LinkVisited, QColor(110, 70, 150));
    palette.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(110, 70, 150).darker());

    qApp->setPalette(palette);
    QToolTip::setPalette(palette);

    const int padding{QFontMetrics(QFont()).height() / 2};
    qApp->setStyleSheet(QString(R"(
        QToolTip {
            padding: %1px;
            border: 1px solid rgb(30, 30, 30);
            background: rgb(45, 45, 45);
        }
    )")
                            .arg(padding));
  }
  else if (style == ApplicationStyle::Dark)
  {
    QPalette palette;
    palette.setColor(QPalette::All, QPalette::Window, QColor(22, 22, 22));
    palette.setColor(QPalette::Disabled, QPalette::Window, QColor(30, 30, 30));
    palette.setColor(QPalette::All, QPalette::WindowText, QColor(180, 180, 180));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(90, 90, 90));
    palette.setColor(QPalette::All, QPalette::Base, QColor(35, 35, 35));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(30, 30, 30));
    palette.setColor(QPalette::All, QPalette::AlternateBase, QColor(40, 40, 40));
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(35, 35, 35));
    palette.setColor(QPalette::All, QPalette::ToolTipBase, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(0, 0, 0));
    palette.setColor(QPalette::All, QPalette::ToolTipText, QColor(170, 170, 170));
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(170, 170, 170));
    palette.setColor(QPalette::All, QPalette::PlaceholderText, QColor(100, 100, 100));
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(100, 100, 100));
    palette.setColor(QPalette::All, QPalette::Text, QColor(200, 200, 200));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(90, 90, 90));
    palette.setColor(QPalette::All, QPalette::Button, QColor(30, 30, 30));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(20, 20, 20));
    palette.setColor(QPalette::All, QPalette::ButtonText, QColor(180, 180, 180));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
    palette.setColor(QPalette::All, QPalette::BrightText, QColor(75, 75, 75));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::All, QPalette::Light, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor(0, 0, 0));
    palette.setColor(QPalette::All, QPalette::Midlight, QColor(40, 40, 40));
    palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor(40, 40, 40));
    palette.setColor(QPalette::All, QPalette::Dark, QColor(90, 90, 90));
    palette.setColor(QPalette::Disabled, QPalette::Dark, QColor(90, 90, 90));
    palette.setColor(QPalette::All, QPalette::Mid, QColor(60, 60, 60));
    palette.setColor(QPalette::Disabled, QPalette::Mid, QColor(60, 60, 60));
    palette.setColor(QPalette::All, QPalette::Shadow, QColor(10, 10, 10));
    palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor(20, 20, 20));
    palette.setColor(QPalette::All, QPalette::Highlight, QColor(35, 130, 200));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(35, 130, 200).darker());
    palette.setColor(QPalette::All, QPalette::HighlightedText, QColor(240, 240, 240));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(35, 35, 35));
    palette.setColor(QPalette::All, QPalette::Link, QColor(40, 130, 220));
    palette.setColor(QPalette::Disabled, QPalette::Link, QColor(40, 130, 220).darker());
    palette.setColor(QPalette::All, QPalette::LinkVisited, QColor(110, 70, 150));
    palette.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(110, 70, 150).darker());

    qApp->setPalette(palette);
    QToolTip::setPalette(palette);

    const int padding{QFontMetrics(QFont()).height() / 2};
    qApp->setStyleSheet(QString(R"(
        QToolTip {
            padding: %1px;
            border: 1px solid rgb(50, 50, 50);
            background: rgb(10, 10, 10);
        }
    )")
                            .arg(padding));
  }
}

}  // namespace GUICommon
