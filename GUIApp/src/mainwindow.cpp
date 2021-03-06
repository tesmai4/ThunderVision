#include <iostream>
#include <QProgressDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include "camerasviewdialog.hpp"
#include "rectificationviewdialog.hpp"
#include "selectinputdialog.hpp"
#include "disparitydialog.hpp"
#include "thanksdialog.hpp"
#include "mainwindow.hpp"

MainWindow::MainWindow(tdv::TDVContext *ctx)
{
    m_ctx = ctx;
    m_reconst = NULL;

    m_camsDialog = NULL;
    m_rectDialog = NULL;
    m_dispDialog = NULL;
    
    setupUi(this);
    connect(pbCamerasView, SIGNAL(clicked()),
            this, SLOT(showCamerasViews()));
    connect(pbReconstFrame, SIGNAL(clicked()),
            this, SLOT(stepReconstruction()));
    connect(pbReconstStream, SIGNAL(clicked()),
            this, SLOT(playReconstruction()));
    connect(pbDisparityMap, SIGNAL(clicked()),
            this, SLOT(showDisparity()));
    connect(pbRectification, SIGNAL(clicked()),
            this, SLOT(showRectification()));        
    
    pbExport->setEnabled(true);
    pbRectification->setEnabled(false);
    pbDisparityMap->setEnabled(false);
    
    m_reprView = new ReprojectionView(this);
    layReproj->addWidget(m_reprView, 0, 0);

    connect(pbExport, SIGNAL(clicked()),
            m_reprView, SLOT(exportMesh()));

}

MainWindow::~MainWindow()
{

}

void MainWindow::start(tdv::StereoInputSource *inputSrc)
{
    if ( inputSrc == NULL )
    {
        SelectInputDialog inputDlg;
        if ( inputDlg.exec() == QDialog::Accepted )
        {
            QProgressDialog loadDlg("Loading", "", 0, 0,
                                    NULL, Qt::FramelessWindowHint);
            loadDlg.show();
            try
            {
                inputSrc = inputDlg.createInputSource();
            }
            catch (const tdv::Exception &ex)
            {
                QMessageBox::critical(this,
                                      tr("Error while creating input sources"),
                                      tr(ex.what()));
            }

            loadDlg.close();
        }
        else
        {
            close();
        }
    }

    if ( inputSrc != NULL )
    {
        m_ctx->start(inputSrc);
    }
}

void MainWindow::playReconstruction()
{
    initReconstruction();

    if ( m_reconst != NULL )
    {
        m_reconst->continuous();
    }
}

void MainWindow::stepReconstruction()
{
    initReconstruction();

    if ( m_reconst != NULL )
    {
        m_reconst->step();
    }
}

void MainWindow::pauseReconstruction()
{
    initReconstruction();
    if ( m_reconst != NULL )
    {
        m_reconst->pause();
    }
}

void MainWindow::showCamerasViews()
{
    if ( m_camsDialog == NULL )
    {
        m_camsDialog = new CamerasViewDialog(m_ctx, this);
        m_camsDialog->init();
        m_camsDialog->show();
        connect(m_camsDialog, SIGNAL(finished(int)),
                this, SLOT(doneCamerasViews()));
        pbCamerasView->setEnabled(false);
     }
}

void MainWindow::doneCamerasViews()
{
    if( m_camsDialog != NULL )
    {
        m_camsDialog->dispose();
        m_camsDialog = NULL;
        pbCamerasView->setEnabled(true);
    }
}

void MainWindow::showRectification()
{
    if ( m_reconst != NULL && m_rectDialog == NULL )
    {
        m_rectDialog = new RectificationViewDialog(m_reconst, this);
        m_rectDialog->init();
        m_rectDialog->show();
        connect(m_rectDialog, SIGNAL(finished(int)),
                this, SLOT(doneRectification()));
        pbRectification->setEnabled(false);        
    }
}

void MainWindow::doneRectification()
{
    if ( m_rectDialog != NULL )
    {
        m_rectDialog->dispose();
        m_rectDialog = NULL;        
        pbRectification->setEnabled(true);
    }
}

void MainWindow::showDisparity()
{
    if ( m_reconst != NULL && m_dispDialog == NULL )
    {
        m_dispDialog = new DisparityDialog(m_reconst, this);
        m_dispDialog->init();
        m_dispDialog->show();
        connect(m_dispDialog, SIGNAL(finished(int)),
                this, SLOT(doneDisparity()));
        pbDisparityMap->setEnabled(false);
    }
}

void MainWindow::doneDisparity()
{
    if ( m_dispDialog != NULL )
    {
        m_dispDialog->dispose();
        m_dispDialog = NULL;
        pbDisparityMap->setEnabled(true);
    }
}

void MainWindow::initReconstruction()
{
    if ( m_reconst == NULL )
    {        
        m_reconst = m_ctx->runReconstruction(
            "Device", m_reprView->reprojection());
        pbRectification->setEnabled(true);
        pbDisparityMap->setEnabled(true);
    }
}

void MainWindow::dispose()
{
    if ( m_camsDialog != NULL )
    {
        m_camsDialog->close();
    }

    if ( m_rectDialog != NULL )
    {
        m_rectDialog->close();
    }

    if ( m_reconst != NULL )
    {
        m_ctx->releaseReconstruction(m_reconst);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{    
    if ( m_rectDialog != NULL 
         || m_camsDialog != NULL )
        event->ignore();
    
    dispose();
#ifdef TDV_THANKS    
    ThanksDialog thanks(this);    
    thanks.exec();
#endif
}
