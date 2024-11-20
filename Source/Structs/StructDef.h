#pragma once

#include <string>
#include <vector>

#include <QString>

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"
#include "../Structs/FieldDef.h"

class StructDef
{
public:
  StructDef();
  StructDef(QString label, size_t length, QVector<FieldDef*> entries, bool isPacked);

  ~StructDef();

  StructDef(const StructDef&) = delete;
  StructDef(StructDef&&) = delete;
  StructDef& operator=(const StructDef&) = delete;
  StructDef& operator=(StructDef&&) = delete;

  QString getLabel();
  size_t getLength();
  bool isValidStruct();
  void setLength(size_t length);
  void setLabel(const QString& label);
  void addFields(FieldDef* entry, size_t index = -1);
  void clearFields();
  void setFields(QVector<FieldDef*> entries);

  void readFromJson(const QJsonObject& json);
  void writeToJson(QJsonObject& json);

private:
  void calculateLength();
  QString m_label;
  size_t m_length;
  QVector<FieldDef*> m_fields;
  bool m_isPacked;
};
