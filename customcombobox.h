#ifndef CUSTOMCOMBOBOX_H
#define CUSTOMCOMBOBOX_H

#include <QComboBox>
#include <QDebug>

class CustomComboBox : public QComboBox
{
public:
    CustomComboBox(QWidget *parent = nullptr);
    void showPopup() override;
};

#endif // CUSTOMCOMBOBOX_H
