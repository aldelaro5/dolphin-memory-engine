#pragma once

#include <QLineEdit>

class AddressInputWidget final : public QLineEdit
{
public:
  explicit AddressInputWidget(QWidget* parent = nullptr);
  ~AddressInputWidget() override = default;

  AddressInputWidget(const AddressInputWidget&) = delete;
  AddressInputWidget(AddressInputWidget&&) = delete;
  AddressInputWidget& operator=(const AddressInputWidget&) = delete;
  AddressInputWidget& operator=(AddressInputWidget&&) = delete;

  bool event(QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private:
  int calcPrefixWidth();
  void updateStyleSheet();
};
