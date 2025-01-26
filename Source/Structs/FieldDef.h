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
  FieldDef(size_t offset, size_t size = -1, bool is_padding = false);
  FieldDef(size_t offset, MemWatchEntry* entry);
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
  size_t getFieldSize() const;
  void setFieldSize(size_t size);
  QString getLabel() const;
  void setLabel(QString label);
  bool isPadding() const;
  void convertToPadding();

  void readFromJSON(const QJsonObject& json);
  void writeToJson(QJsonObject& json);

private:
  u32 m_structOffset;
  size_t m_size;
  MemWatchEntry* m_entry;
};
