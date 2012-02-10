#include "AMLoopActionEditor.h"

#include <QSpinBox>
#include <QBoxLayout>
#include <QLabel>

AMLoopActionEditor::AMLoopActionEditor(AMLoopActionInfo *info, QWidget *parent)
    : QFrame(parent) {

    info_ = info;
    setFrameStyle(QFrame::StyledPanel);

    spinBox_ = new QSpinBox();
    spinBox_->setRange(1, 9999);

    QHBoxLayout* hl = new QHBoxLayout(this);
    hl->addWidget(new QLabel("Looping"));
    hl->addWidget(spinBox_);
    hl->addWidget(new QLabel("times"));
    hl->addStretch();

    spinBox_->setValue(info_->loopCount());
    connect(spinBox_, SIGNAL(editingFinished()), this, SLOT(onSpinBoxEditingFinished()));
}

void AMLoopActionEditor::onSpinBoxEditingFinished()
{
    info_->setLoopCount(spinBox_->value());
}