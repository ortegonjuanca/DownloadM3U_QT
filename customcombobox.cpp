#include "customcombobox.h"

CustomComboBox::CustomComboBox(QWidget *parent)
    : QComboBox (parent) { }

void CustomComboBox::showPopup()
{
    QComboBox::showPopup();
}
