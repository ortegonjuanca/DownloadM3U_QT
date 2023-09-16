#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "customcombobox.h"

#include <QLabel>
#include <QMainWindow>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void LoadFile();
    void CheckAll();
    void UncheckAll();
    void SaveFile();
    void FilterList(const QString &filter);
    void CleanUrls();
    void CleanScreen();

private:
    Ui::MainWindow *ui;
    QString header, fileContent;
    QList<QString> fileContentList;
    QList<QString> countriesList;
    QSet<QString> selectedCountries;
    QLabel * loadingLabel;
    CustomComboBox * customComboBox;
};
#endif // MAINWINDOW_H
