/* $Id$ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
 */

/*
 * Copyright (C) 2010-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QHBoxLayout>
# include <QHeaderView>
# include <QLabel>
# include <QProgressBar>
# include <QTableWidget>

/* GUI includes: */
# include "QIToolButton.h"
# include "UIIconPool.h"
# include "UIGuestControlFileManager.h"
# include "UIGuestControlFileManagerOperationsPanel.h"
# include "UIProgressEventHandler.h"

/* COM includes: */
# include "CProgress.h"


#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/*********************************************************************************************************************************
*   UIFileOperationProgressWidget definition.                                                                                    *
*********************************************************************************************************************************/

class UIFileOperationProgressWidget : public QIWithRetranslateUI<QWidget>
{

    Q_OBJECT;

public:

    UIFileOperationProgressWidget(const CProgress &comProgress, QWidget *pParent = 0);
    ~UIFileOperationProgressWidget();

signals:

    void sigProgressTaskComplete(QUuid progressId);

protected:

    void retranslateUi() /* override */;

private slots:

    void sltHandleProgressPercentageChange(const QUuid &uProgressId, const int iPercent);
    void sltHandleProgressTaskComplete(const QUuid &uProgressId);
    void sltCancelProgress();

private:

    void prepare();
    void prepareWidgets();
    void prepareEventHandler();
    void cleanupEventHandler();

    CProgress               m_comProgress;
    UIProgressEventHandler *m_pEventHandler;
    QHBoxLayout            *m_pMainLayout;
    QProgressBar           *m_pProgressBar;
    QIToolButton           *m_pCancelButton;
};


/*********************************************************************************************************************************
*   UIFileOperationProgressWidget implementation.                                                                                *
*********************************************************************************************************************************/

UIFileOperationProgressWidget::UIFileOperationProgressWidget(const CProgress &comProgress, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_comProgress(comProgress)
    , m_pEventHandler(0)
    , m_pMainLayout(0)
    , m_pProgressBar(0)
    , m_pCancelButton(0)
{
    prepare();
}

UIFileOperationProgressWidget::~UIFileOperationProgressWidget()
{
    cleanupEventHandler();
}

void UIFileOperationProgressWidget::retranslateUi()
{
    if (m_pCancelButton)
        m_pCancelButton->setToolTip(UIGuestControlFileManager::tr("Close the pane"));
}

void UIFileOperationProgressWidget::prepare()
{
    prepareWidgets();
    prepareEventHandler();
}

void UIFileOperationProgressWidget::prepareWidgets()
{
    m_pMainLayout = new QHBoxLayout;
    if (!m_pMainLayout)
        return;

    m_pCancelButton = new QIToolButton;
    if (m_pCancelButton)
    {
        //m_pCancelButton->setIcon(UIIconPool::iconSet(":/close_16px.png"));
        m_pCancelButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
        //m_pCancelButton->setStyleSheet("QToolButton { border: 0px none black; margin: 0px 0px 0px 0px; } QToolButton::menu-indicator {image: none;}");
        m_pCancelButton->setStyleSheet("QToolButton { border: 0px none black; margin: 0px 0px 0px 0px; } ");
        const QSize sh = m_pCancelButton->sizeHint();
        m_pCancelButton->setFixedSize(sh);

        connect(m_pCancelButton, &QIToolButton::clicked, this, &UIFileOperationProgressWidget::sltCancelProgress);
        if (!m_comProgress.isNull() && !m_comProgress.GetCancelable())
            m_pCancelButton->setEnabled(false);
        m_pMainLayout->addWidget(m_pCancelButton);
    }

    m_pProgressBar = new QProgressBar;
    if (m_pProgressBar)
    {
        m_pProgressBar->setMinimum(0);
        m_pProgressBar->setMaximum(100);
        m_pProgressBar->setTextVisible(true);
        m_pProgressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_pMainLayout->addWidget(m_pProgressBar);
    }

    setLayout(m_pMainLayout);
}

void UIFileOperationProgressWidget::prepareEventHandler()
{
    if (m_comProgress.isNull())
        return;
    m_pEventHandler = new UIProgressEventHandler(this, m_comProgress);
    connect(m_pEventHandler, &UIProgressEventHandler::sigProgressPercentageChange,
            this, &UIFileOperationProgressWidget::sltHandleProgressPercentageChange);
    connect(m_pEventHandler, &UIProgressEventHandler::sigProgressTaskComplete,
            this, &UIFileOperationProgressWidget::sltHandleProgressTaskComplete);
}

void UIFileOperationProgressWidget::cleanupEventHandler()
{
    delete m_pEventHandler;
    m_pEventHandler = 0;
}

void UIFileOperationProgressWidget::sltHandleProgressPercentageChange(const QUuid &uProgressId, const int iPercent)
{
    Q_UNUSED(uProgressId);
    Q_UNUSED(iPercent);
    m_pProgressBar->setValue(iPercent);
}

void UIFileOperationProgressWidget::sltHandleProgressTaskComplete(const QUuid &uProgressId)
{
    Q_UNUSED(uProgressId);
    if (m_pCancelButton)
        m_pCancelButton->setEnabled(false);
    emit sigProgressTaskComplete(m_comProgress.GetId());
}

void UIFileOperationProgressWidget::sltCancelProgress()
{
    m_comProgress.Cancel();
    /* Since we dont have a "progress canceled" event we have to do this here: */
    if (m_pCancelButton)
        m_pCancelButton->setEnabled(false);
}


/*********************************************************************************************************************************
*   UIGuestControlFileManagerOperationsPanel implementation.                                                                     *
*********************************************************************************************************************************/

UIGuestControlFileManagerOperationsPanel::UIGuestControlFileManagerOperationsPanel(UIGuestControlFileManager *pManagerWidget,
                                                                                   QWidget *pParent)
    : UIGuestControlFileManagerPanel(pManagerWidget, pParent)
    , m_pTableWidget(0)
{
    prepare();
}

void UIGuestControlFileManagerOperationsPanel::addNewProgress(const CProgress &comProgress)
{
    if (!m_pTableWidget)
        return;

    m_pTableWidget->setRowCount(m_pTableWidget->rowCount() + 1);
    UIFileOperationProgressWidget *pOperationsWidget = new UIFileOperationProgressWidget(comProgress);
    m_pTableWidget->setCellWidget(m_pTableWidget->rowCount() - 1, 0, pOperationsWidget);
    connect(pOperationsWidget, &UIFileOperationProgressWidget::sigProgressTaskComplete,
            this, &UIGuestControlFileManagerOperationsPanel::sigFileOperationComplete);

    m_pTableWidget->resizeColumnsToContents();
}

QString UIGuestControlFileManagerOperationsPanel::panelName() const
{
    return "OperationsPanel";
}

void UIGuestControlFileManagerOperationsPanel::prepareWidgets()
{
    if (!mainLayout())
        return;
    m_pTableWidget = new QTableWidget();

    if (m_pTableWidget)
    {
        m_pTableWidget->setColumnCount(TableColumn_Max);
        m_pTableWidget->verticalHeader()->hide();
        QStringList headers;
        headers << "Progress" << "Information";
        m_pTableWidget->setHorizontalHeaderLabels(headers);
        m_pTableWidget->setShowGrid(false);
        mainLayout()->addWidget(m_pTableWidget);
    }
}

void UIGuestControlFileManagerOperationsPanel::prepareConnections()
{

}

void UIGuestControlFileManagerOperationsPanel::retranslateUi()
{
    UIGuestControlFileManagerPanel::retranslateUi();

}

#include "UIGuestControlFileManagerOperationsPanel.moc"
