#pragma once

#include <string>
#include <vector>

#include <QJsonObject>
#include <QString>

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"
#include "../MemoryWatch/MemWatchEntry.h"

class FieldDef
{
public:
  FieldDef();
  FieldDef(u32 offset, u32 size = 0, bool is_padding = false);
  FieldDef(u32 offset, MemWatchEntry* entry);
  explicit FieldDef(FieldDef* field);

  ~FieldDef();

  FieldDef(const FieldDef&) = delete;
  FieldDef(FieldDef&&) = delete;
  FieldDef& operator=(const FieldDef&) = delete;
  FieldDef& operator=(FieldDef&&) = delete;

  u32 getOffset() const;
  void setOffset(u32 offset);
  MemWatchEntry* getEntry() const;
  void setEntry(MemWatchEntry* entry);
  u32 getFieldSize() const;
  void setFieldSize(u32 size);
  QString getLabel() const;
  void setLabel(QString label);
  bool isPadding() const;
  void convertToPadding();

  bool isSame(FieldDef* const other) const;
  QStringList diffList(FieldDef* const other) const;
  QStringList getFieldDescLines() const;

  void readFromJSON(const QJsonObject& json);
  void writeToJson(QJsonObject& json);

private:
  u32 m_structOffset;
  u32 m_size;
  MemWatchEntry* m_entry;
};
