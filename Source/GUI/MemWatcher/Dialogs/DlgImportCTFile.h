#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "../../../Common/CommonTypes.h"

class DlgImportCTFile : public QDialog
{
public:
  explicit DlgImportCTFile(QWidget* parent);

  void initialiseWidgets();
  void makeLayouts();
  QString getFileName() const;
  bool willUseDolphinPointers() const;
  u32 getCommonBase() const;
  void onAddressImportMethodChanged();
  void onBrowseFiles();
  void accept() override;

private:
  bool m_useDolphinPointers;
  u32 m_commonBase = 0;
  QString m_strFileName;
  QLineEdit* m_txbFileName;
  QPushButton* m_btnBrowseFiles;
  QLineEdit* m_txbCommonBase;
  QButtonGroup* m_btnGroupImportAddressMethod;
  QRadioButton* m_rdbUseDolphinPointers;
  QRadioButton* m_rdbUseCommonBase;
  QGroupBox* m_groupImportAddressMethod;
  QWidget* m_widgetAddressMethod;
};
