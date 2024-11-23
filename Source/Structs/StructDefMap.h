#pragma once

#include <string>
#include <QString>
#include <QJsonObject>

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"
#include "../Structs/StructDef.h"
#include "StructTreeNode.h"

class StructDefMap
{
  StructDefMap();

  ~StructDefMap();

  StructDefMap(const StructDefMap&) = delete;
  StructDefMap(StructDefMap&&) = delete;
  StructDefMap& operator=(const StructDefMap&) = delete;
  StructDefMap& operator=(StructDefMap&&) = delete;

  QMap<QString, StructDef*> m_structs;

  void addStructDef(QString label, StructDef* structDef);
  void removeStructDef(QString label);
  void changeStructKey(QString oldLabel, QString newLabel);

  StructTreeNode* getRootNode();

  void readFromJson(QJsonObject& json);
  void writeToJson(QJsonObject& json);
  StructTreeNode* m_rootNode;
};
