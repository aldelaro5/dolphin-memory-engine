#include "StructDefMap.h"

#include <QJsonArray>
#include <QFileDialog>


StructDefMap::StructDefMap()
{
  m_structs = QMap<QString, StructDef*>();
}

StructDefMap::~StructDefMap()
{
  qDeleteAll(m_structs);
  m_structs.clear();
}

void StructDefMap::addStructDef(QString label, StructDef* structDef)
{
  m_structs.insert(label, structDef);
}

void StructDefMap::removeStructDef(QString label)
{
  if (!m_structs.contains(label))
  {
    return;
  }
  delete m_structs[label];
  m_structs.remove(label);
}

void StructDefMap::changeStructKey(QString oldLabel, QString newLabel)
{
  if (!m_structs.contains(oldLabel) || oldLabel == newLabel) {
    return;
  }
  StructDef* s = m_structs[oldLabel];
  m_structs.insert(newLabel, s);
  m_structs.remove(oldLabel);
}

StructTreeNode* StructDefMap::getRootNode()
{
  return m_rootNode;
}

void StructDefMap::readFromJson(QJsonObject& json)
{
  QJsonArray map = json["map"].toArray();
  for (auto i : map)
  {
    QJsonObject node = i.toObject();
    QString key = node["key"].toString();

    //For now any collisions are skipped
    if (m_structs.contains(key))
    {
      continue;
    }

    QJsonObject nodeJson = node["value"].toObject();
    StructDef* value = new StructDef();
    value->readFromJson(nodeJson);

    m_structs.insert(key, value);
  }

  QJsonObject tree = json["structTree"].toObject();
  m_rootNode->readFromJson(tree, nullptr, m_structs.keys());
}

void StructDefMap::writeToJson(QJsonObject& json)
{
  QJsonArray structArr;
  for (QString key : m_structs.keys())
  {
    QJsonObject structJson;
    structJson["key"] = key;
    StructDef* structDef = m_structs[key];

    if (!structDef)
    {
      structJson["value"] = QString("None");
    }
    else
    {
      QJsonObject valueJson;
      structDef->writeToJson(valueJson);
      structJson["value"] = valueJson;
    }
  }
  json["structMap"] = structArr;

  QJsonObject structTree;
  m_rootNode->writeToJson(structTree, m_structs.keys());
  json["structTree"] = structTree;
}
