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
  StructDef(QString label);
  StructDef(QString label, u32 length, QVector<FieldDef*> entries);
  explicit StructDef(StructDef* structDef);

  ~StructDef();

  StructDef(const StructDef&) = delete;
  StructDef(StructDef&&) = delete;
  StructDef& operator=(const StructDef&) = delete;
  StructDef& operator=(StructDef&&) = delete;

  QString getLabel();
  u32 getLength();
  QVector<FieldDef*> getFields();
  bool isValidFieldLayout(u32 length, QVector<FieldDef*> fields);
  bool isValidStruct();
  void setLength(u32 length);
  void setLabel(const QString& label);
  void addFields(FieldDef* entry, int index = -1);
  void clearFields();
  void setFields(QVector<FieldDef*> entries);
  void updateStructTypeLabel(const QString& oldLabel, QString newLabel);
  void updateStructFieldSize(QString structName, u32 newLength);

  void readFromJson(const QJsonObject& json);
  void writeToJson(QJsonObject& json);

  bool isSame(const StructDef* other) const;
  QString getDiffString(const StructDef* other) const;

private:
  void recalculateOffsets();
  void calculateLength();
  QString m_label;
  u32 m_length;
  QVector<FieldDef*> m_fields;
  bool m_isValid;
};
