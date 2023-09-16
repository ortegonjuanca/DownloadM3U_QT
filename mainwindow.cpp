#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QEventLoop>
#include <QRegularExpression>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMovie>
#include <QCompleter>
#include <QSettings>
#include <QStringListModel>
#include "customcombobox.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());

    QSettings settings("./assets/config.ini", QSettings::IniFormat);
    this->customComboBox = new CustomComboBox(this);

    QList<QString> optionsList;
    settings.beginGroup("CustomComboBoxOptions");
    QStringList optionKeys = settings.childKeys();
    this->customComboBox->clear();
    foreach (const QString& optionKey, optionKeys)
    {
        QString optionText = settings.value(optionKey).toString();
        optionsList.append(optionText);
        this->customComboBox->addItem(optionText);
    }
    this->customComboBox->addItem("");
    this->customComboBox->setCurrentIndex(-1);
    settings.endGroup();

    this->customComboBox->setEditable(true);
    QFont font;
    font.setPointSize(14);
    this->customComboBox->setFont(font);
    QCompleter * completer = new QCompleter(optionsList, this);
    this->customComboBox->setCompleter(completer);

    ui->horizontalLayout->insertWidget(1, this->customComboBox, 1);

    connect(ui->loadFileButton, SIGNAL(clicked()), this, SLOT(LoadFile()));
    connect(ui->checkAllButton, SIGNAL(clicked()), this, SLOT(CheckAll()));
    connect(ui->uncheckAllButton, SIGNAL(clicked()), this, SLOT(UncheckAll()));
    connect(ui->saveFileButton, SIGNAL(clicked()), this, SLOT(SaveFile()));
    connect(ui->countryFilterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(FilterList(const QString &)));
    connect(ui->cleanUrlsAction, SIGNAL(triggered()), this, SLOT(CleanUrls()));
    connect(ui->cleanScreenButton, SIGNAL(clicked()), this, SLOT(CleanScreen()));

    QVBoxLayout * mainLayout = new QVBoxLayout(this);

    this->loadingLabel = new QLabel(this);
    this->loadingLabel->setAlignment(Qt::AlignCenter);
    this->loadingLabel->setFixedSize(400, 400);
    QMovie *loadingMovie = new QMovie("./assets/loadingSpinner.gif");
    this->loadingLabel->setMovie(loadingMovie);
    this->loadingLabel->hide();
    loadingMovie->start();

    mainLayout->addWidget(this->loadingLabel, 0, Qt::AlignCenter);
    ui->centralwidget->setLayout(mainLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::LoadFile()
{
    QString m3uUrl = customComboBox->currentText();
    if (m3uUrl.isEmpty())
    {
        QMessageBox::critical(nullptr, "Error", "Debe introducir una URL.");
        return;
    }

    this->setEnabled(false);
    this->loadingLabel->show();

    QSettings settings("assets/config.ini", QSettings::IniFormat);

    settings.remove("CustomComboBoxOptions");
    settings.beginGroup("CustomComboBoxOptions");

    QCompleter * completer = customComboBox->completer();

    if (this->customComboBox->findText(m3uUrl) == -1)
    {
        this->customComboBox->insertItem(0, m3uUrl);

        if (completer)
        {
            QStringListModel *model = qobject_cast<QStringListModel *>(completer->model());
            if (model) {
                QStringList opciones = model->stringList();
                opciones << m3uUrl;
                model->setStringList(opciones);
            }
        }
    }

    for (int i = 0; i < customComboBox->count()-1; i++)
    {
        QString optionText = customComboBox->itemText(i);
        settings.setValue("Option"+QString::number(i), optionText);
    }

    settings.endGroup();

    QUrl url(m3uUrl);
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray fileData = reply->readAll();
        this->fileContent = QString(fileData);
        this->fileContentList = this->fileContent.split("\n");
        this->header = fileContentList[0];

        QRegularExpression regex("group-title=\"([^\"]+)\"");
        QSet<QString> countriesSet;

        for (int i = 1; i < fileContentList.length(); i += 2)
        {
            QRegularExpressionMatch match = regex.match(fileContentList[i]);
            if (match.hasMatch())
            {
                QString groupTitle = match.captured(1);
                countriesSet.insert(groupTitle);
            }
        }

        this->countriesList = countriesSet.values();

        std::sort(countriesList.begin(), countriesList.end(), [](const QString &left, const QString &right)
        {
            return left < right;
        });

        ui->countriesListWidget->clear();
        QStringListIterator it(countriesList);
        while (it.hasNext())
        {
              QListWidgetItem * listItem = new QListWidgetItem(it.next(), ui->countriesListWidget);
              listItem->setCheckState(Qt::Unchecked);
              ui->countriesListWidget->addItem(listItem);
        }

        ui->checkAllButton->setEnabled(true);
        ui->uncheckAllButton->setEnabled(true);
        ui->countryFilterLineEdit->setEnabled(true);
        ui->countriesListWidget->setEnabled(true);
        ui->saveFileButton->setEnabled(true);
        ui->cleanScreenButton->setEnabled(true);
        this->loadingLabel->hide();
    }
    else
    {
        this->loadingLabel->hide();
        QMessageBox::critical(nullptr, "Error", "Error durante la carga. Inténtelo de nuevo más tarde.");
    }

    reply->deleteLater();
    this->setEnabled(true);
}

void MainWindow::CheckAll()
{
    int itemCount = ui->countriesListWidget->count();
    for (int i = 0; i < itemCount; i++)
    {
        QListWidgetItem * item = ui->countriesListWidget->item(i);
        if (item)
        {
            item->setCheckState(Qt::Checked);
        }
    }
}

void MainWindow::UncheckAll()
{
    int itemCount = ui->countriesListWidget->count();
    for (int i = 0; i < itemCount; i++)
    {
        QListWidgetItem * item = ui->countriesListWidget->item(i);
        if (item)
        {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

void MainWindow::SaveFile()
{
    this->selectedCountries.clear();
    int itemCount = ui->countriesListWidget->count();
    for (int i = 0; i < itemCount; i++)
    {
        QListWidgetItem * item = ui->countriesListWidget->item(i);
        if (item->checkState() == Qt::Checked)
        {
            this->selectedCountries.insert(item->text());
        }
    }

    if (selectedCountries.empty())
    {
        QMessageBox::critical(nullptr, "Error", "Debe haber al menos una elección de la lista.");
        return;
    }

    QString selectedFilePath = QFileDialog::getSaveFileName(nullptr, "Guardar archivo", "", "Archivos M3U (*.m3u)");

    if (!selectedFilePath.isEmpty())
    {
        QFile file(selectedFilePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            this->setEnabled(false);
            QString finalFileContent = this->header + "\n";
            for (int i = 1; i < this->fileContentList.length(); i += 2)
            {
                foreach (const QString &country, this->selectedCountries)
                {
                    if (fileContentList[i].contains("group-title=\""+country+"\"")) {
                        finalFileContent += fileContentList[i] + "\n";
                        finalFileContent += fileContentList[i+1] + "\n";
                    }
                }
            }
            QTextStream stream(&file);
            stream << finalFileContent;
            file.close();

            this->setEnabled(true);;
            QMessageBox::information(nullptr, "Éxito", "Archivo creado correctamente en " + selectedFilePath);
        }
        else
        {
            QMessageBox::critical(nullptr, "Error", "No se pudo abrir el archivo para escritura.");
        }
    }
}

void MainWindow::FilterList(const QString &filter)
{
    int itemCount = ui->countriesListWidget->count();
    for (int i = 0; i < itemCount; ++i) {
        QListWidgetItem * item = ui->countriesListWidget->item(i);
        if (item)
        {
            bool hide = !item->text().contains(filter, Qt::CaseInsensitive);
            item->setHidden(hide);
        };
    }
}

void MainWindow::CleanUrls()
{
    this->customComboBox->clear();
    this->customComboBox->addItem("");

    QSettings settings("assets/config.ini", QSettings::IniFormat);
    settings.remove("CustomComboBoxOptions");
}

void MainWindow::CleanScreen()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmación", "¿Seguro que quieres limpiar la pantalla?",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        ui->countriesListWidget->clear();
        this->customComboBox->clearEditText();
        ui->checkAllButton->setEnabled(false);
        ui->uncheckAllButton->setEnabled(false);
        ui->countryFilterLineEdit->clear();
        ui->countryFilterLineEdit->setEnabled(false);
        ui->countriesListWidget->setEnabled(false);
        ui->saveFileButton->setEnabled(false);
        ui->cleanScreenButton->setEnabled(false);
    }
}
