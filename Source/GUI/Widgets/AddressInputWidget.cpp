#include "AddressInputWidget.h"

#include <QEvent>
#include <QFontDatabase>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOptionFrame>

namespace
{
constexpr int horizontalMargin{2};  // QLineEditPrivate::horizontalMargin

class AddressValidator : public QValidator
{
public:
  QValidator::State validate(QString& input, int& pos) const override
  {
    // Strip invalid characters.
    {
      const QString copy{input};
      input.clear();
      for (qsizetype i{0}; i < copy.size(); ++i)
      {
        const QChar c{copy[i].toLower()};
        if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || c == 'x')
        {
          input.append(copy[i]);
          continue;
        }
        if (i < pos)
        {
          pos -= 1;
        }
      }
    }

    // Drop the optional prefix.
    if (input.startsWith("0x"))
    {
      input.remove(0, 2);
      pos = std::max(0, pos - 2);
    }

    // Drop excess.
    input.truncate(8);
    pos = std::min(8, pos);

    return QValidator::Acceptable;
  }
};
}  // namespace

AddressInputWidget::AddressInputWidget(QWidget* const parent) : QLineEdit(parent)
{
  setPlaceholderText("00000000");
  setValidator(new AddressValidator);

  const QFont fixedFont{QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont)};
  setFont(fixedFont);

  updateStyleSheet();
}

bool AddressInputWidget::event(QEvent* const event)
{
  if (event->type() == QEvent::FontChange)
  {
    updateStyleSheet();
  }
  return QLineEdit::event(event);
}

void AddressInputWidget::paintEvent(QPaintEvent* const event)
{
  QLineEdit::paintEvent(event);

  const QColor placeholderTextColor{palette().color(QPalette::PlaceholderText)};
  const QColor textColor{palette().color(QPalette::Text)};
  const QColor prefixColor{
      (placeholderTextColor.red() + textColor.red()) / 2,
      (placeholderTextColor.green() + textColor.green()) / 2,
      (placeholderTextColor.blue() + textColor.blue()) / 2,
  };

  QStyleOptionFrame panel{};
  initStyleOption(&panel);
  const QRect contentsRect{style()->subElementRect(QStyle::SE_LineEditContents, &panel, this)};
  const int x{contentsRect.x() + horizontalMargin - AddressInputWidget::calcPrefixWidth()};
  const int y{(rect().height() + fontMetrics().tightBoundingRect("0x").height()) / 2};

  QPainter painter(this);
  painter.setPen(prefixColor);
  painter.setFont(font());
  painter.drawText(x, y, "0x");
}

int AddressInputWidget::calcPrefixWidth()
{
  return fontMetrics().horizontalAdvance("0x");
}

void AddressInputWidget::updateStyleSheet()
{
  setStyleSheet(QStringLiteral("QLineEdit { padding-left: %1px }")
                    .arg(AddressInputWidget::calcPrefixWidth()));
}
