#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>

#include "../../../Common/CommonTypes.h"

class DlgImportCTFile : public QDialog
{
public:
  DlgImportCTFile(QWidget* parent);

  void initialiseWidgets();
  void makeLayouts();
  QString getFileName() const;
  bool willUseDolphinPointers() const;
  u64 getCommonBase() const;
  void onAddressImportMethodChanged();
  void onBrowseFiles();
  void accept();

private:
  bool m_useDolphinPointers;
  u64 m_commonBase = 0;
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
