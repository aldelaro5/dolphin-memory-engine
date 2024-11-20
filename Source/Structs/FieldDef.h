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
  FieldDef(size_t offset, bool isPropagated, MemWatchEntry* entry);

  ~FieldDef();

  FieldDef(const FieldDef&) = delete;
  FieldDef(FieldDef&&) = delete;
  FieldDef& operator=(const FieldDef&) = delete;
  FieldDef& operator=(FieldDef&&) = delete;

  size_t getOffset() const;
  bool getIsPropagated() const;
  void setOffset() const;
  void setIsPropagated() const;
  MemWatchEntry* getEntry() const;

  void readFromJSON(const QJsonObject& json);
  void writeToJson(QJsonObject& json);

private:
  size_t m_structOffset;
  bool m_isPropagated;
  MemWatchEntry* m_entry;
};
