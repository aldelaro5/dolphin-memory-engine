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
    if (input.startsWith("0x"))
    {
      input.remove(0, 2);
      pos -= 2;
    }
    if (input.size() > 8)
    {
      return QValidator::Invalid;
    }
    return QValidator::Acceptable;
  }
};
}  // namespace

AddressInputWidget::AddressInputWidget(QWidget* const parent) : QLineEdit(parent)
{
  setPlaceholderText("00000000");
  setValidator(new AddressValidator);
  setMaxLength(10);  // 8 + 2, to allow the user to paste a value that includes  the "0x" prefix

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
